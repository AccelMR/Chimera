/************************************************************************/
/**
 * @file chImageBasedLighting.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Precomputes the data the lit paths read to light surfaces with the environment.
 */
/************************************************************************/

#include "chImageBasedLighting.h"

#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chMath.h"
#include "chRenderSettings.h"
#include "chSkyLightItem.h"
#include "chTextureMips.h"

namespace chEngineSDK {

namespace {
constexpr Format kBrdfLutFormat = Format::R16G16_SFLOAT;
// Light above 1 with enough precision in the darks, like the scene color.
constexpr Format kCubeFormat = Format::R16G16B16A16_SFLOAT;

// Must match numthreads in brdflut.hlsl and the ibl*.hlsl shaders.
constexpr uint32 kGroupSize = 8;

// Must match PushConstants in brdflut.hlsl.
struct BrdfLutPushConstants
{
  uint32 outputIndex;
  uint32 size;
  uint32 sampleCount;
};

// Must match PushConstants in iblenvironment.hlsl.
struct EnvironmentPushConstants
{
  float worldToSkyRows[3][4];
  uint32 skyIndex;
  uint32 samplerIndex;
  uint32 outputIndex;
  uint32 size;
  float skyMipLevel;
};

// Must match PushConstants in ibldownsample.hlsl.
struct DownsamplePushConstants
{
  uint32 sourceIndex;
  uint32 outputIndex;
  uint32 size;
};

// Must match PushConstants in iblirradiance.hlsl.
struct IrradiancePushConstants
{
  uint32 environmentIndex;
  uint32 samplerIndex;
  uint32 outputIndex;
  uint32 size;
  uint32 sampleCount;
  uint32 environmentSize;
  float environmentMaxMip;
};

// Must match PushConstants in iblprefilter.hlsl.
struct PrefilterPushConstants
{
  uint32 environmentIndex;
  uint32 samplerIndex;
  uint32 outputIndex;
  uint32 size;
  uint32 sampleCount;
  uint32 environmentSize;
  float environmentMaxMip;
  float roughness;
};

NODISCARD uint32
getGroupCount(uint32 size)
{
  return (size + kGroupSize - 1) / kGroupSize;
}

NODISCARD uint32
readSetting(const ConsoleVariable<int32>& cvar, int32 low, int32 high)
{
  return static_cast<uint32>(Math::clamp(cvar.get(), low, high));
}

NODISCARD SPtr<IPipeline>
createComputePipeline(IGraphicsAPI& graphicsAPI, StringView shader)
{
  return graphicsAPI.createComputePipeline(
      {.computeShader = graphicsAPI.loadShader(ShaderStage::Compute, shader)});
}
} // namespace

/*
 */
ImageBasedLighting::ImageBasedLighting()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_brdfLutPipeline = createComputePipeline(graphicsAPI, "brdflut");
  m_environmentPipeline = createComputePipeline(graphicsAPI, "iblenvironment");
  m_downsamplePipeline = createComputePipeline(graphicsAPI, "ibldownsample");
  m_irradiancePipeline = createComputePipeline(graphicsAPI, "iblirradiance");
  m_prefilterPipeline = createComputePipeline(graphicsAPI, "iblprefilter");
  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::ClampToEdge,
                                         .addressModeV = SamplerAddressMode::ClampToEdge,
                                         .addressModeW = SamplerAddressMode::ClampToEdge});
  m_skySampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                            .minFilter = SamplerFilter::Linear,
                                            .mipmapMode = SamplerMipmapMode::Linear,
                                            .addressModeU = SamplerAddressMode::Repeat,
                                            .addressModeV = SamplerAddressMode::ClampToEdge,
                                            .addressModeW = SamplerAddressMode::ClampToEdge});
}

/*
 */
ImageBasedLighting::~ImageBasedLighting() = default;

/*
 */
ImageBasedLightingView
ImageBasedLighting::addPasses(RenderGraph& graph, const SkyLightItem* skyLight)
{
  ImageBasedLightingView view;
  if (!g_cvarImageBasedLighting.get() || skyLight == nullptr || skyLight->texture == nullptr) {
    return view;
  }

  const uint32 prefilterSize = readSetting(g_cvarIBLPrefilterSize, 32, 1024);
  const BuildKey key{
      .sky = skyLight->texture,
      .worldToSky = skyLight->worldToSky,
      .cubeSize = readSetting(g_cvarIBLCubeSize, 64, 2048),
      .irradianceSize = readSetting(g_cvarIBLIrradianceSize, 8, 128),
      .irradianceSamples = readSetting(g_cvarIBLIrradianceSamples, 16, 4096),
      .prefilterSize = prefilterSize,
      .prefilterMips = Math::min(readSetting(g_cvarIBLPrefilterMips, 2, 10),
                                 TextureMips::getMipCount(prefilterSize, prefilterSize)),
      .prefilterSamples = readSetting(g_cvarIBLPrefilterSamples, 16, 4096)};

  addBrdfLutPass(graph, view);
  if (key != m_builtKey) {
    addEnvironmentPasses(graph, key, view);
    m_builtKey = key;
  }

  view.irradianceIndex = m_irradiance.view->getBindlessIndex();
  view.prefilteredIndex = m_prefiltered.view->getBindlessIndex();
  view.samplerIndex = m_sampler->getBindlessIndex();
  view.intensity = skyLight->intensity;
  view.prefilteredMaxMip = static_cast<float>(m_prefiltered.mipCount - 1);
  return view;
}

/*
 */
void
ImageBasedLighting::addBrdfLutPass(RenderGraph& graph, ImageBasedLightingView& view)
{
  const uint32 size = readSetting(g_cvarIBLBrdfLutSize, 32, 1024);
  const uint32 samples = readSetting(g_cvarIBLBrdfLutSamples, 16, 8192);
  if (m_brdfLut != nullptr && size == m_brdfLutSize && samples == m_brdfLutSamples) {
    view.brdfLutIndex = m_brdfLut->getBindlessIndex();
    return;
  }

  if (m_brdfLut == nullptr || size != m_brdfLutSize) {
    // The old table is freed by the deferred deletion once the GPU is done with it.
    m_brdfLut = IGraphicsAPI::instance().createTexture(
        {.format = kBrdfLutFormat,
         .width = size,
         .height = size,
         .usage = TextureUsageFlags(TextureUsage::Sampled) | TextureUsage::Storage});
    m_brdfLutView = m_brdfLut->createView();
  }
  m_brdfLutSize = size;
  m_brdfLutSamples = samples;

  view.brdfLutIndex = m_brdfLut->getBindlessIndex();
  view.brdfLut = graph.importTexture("BrdfLut", *m_brdfLut, *m_brdfLutView,
                                     ResourceState::Undefined, ResourceState::ShaderRead);
  graph.addPass("BrdfLut")
      .writeStorage(view.brdfLut)
      .setExecute([this, size, samples](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        const BrdfLutPushConstants pushConstants{
            .outputIndex = m_brdfLutView->getStorageBindlessIndex(),
            .size = size,
            .sampleCount = samples};
        commandList.bindPipeline(*m_brdfLutPipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.dispatch(getGroupCount(size), getGroupCount(size));
      });
}

/*
 */
void
ImageBasedLighting::addEnvironmentPasses(RenderGraph& graph,
                                         const BuildKey& key,
                                         ImageBasedLightingView& view)
{
  updateCube(m_environment, key.cubeSize, TextureMips::getMipCount(key.cubeSize, key.cubeSize));
  updateCube(m_irradiance, key.irradianceSize, 1);
  updateCube(m_prefiltered, key.prefilterSize, key.prefilterMips);

  const RGTextureHandle environment =
      graph.importTexture("IBLEnvironment", *m_environment.texture, *m_environment.view,
                          ResourceState::Undefined, ResourceState::ShaderRead);
  view.irradiance =
      graph.importTexture("IBLIrradiance", *m_irradiance.texture, *m_irradiance.view,
                          ResourceState::Undefined, ResourceState::ShaderRead);
  view.prefiltered =
      graph.importTexture("IBLPrefiltered", *m_prefiltered.texture, *m_prefiltered.view,
                          ResourceState::Undefined, ResourceState::ShaderRead);

  // The image's mip whose texels are about as large as the cube's: a face spans 90 degrees
  // in cubeSize texels, the image 360 degrees in its width.
  const ITexture& sky = *key.sky;
  const float skyMipLevel = Math::clamp(
      Math::log2(static_cast<float>(sky.getWidth()) / (Math::PI * key.cubeSize)), 0.0f,
      static_cast<float>(sky.getMipLevels() - 1));
  EnvironmentPushConstants environmentConstants{
      .worldToSkyRows = {},
      .skyIndex = sky.getBindlessIndex(),
      .samplerIndex = m_skySampler->getBindlessIndex(),
      .outputIndex = m_environment.mipViews[0]->getStorageBindlessIndex(),
      .size = key.cubeSize,
      .skyMipLevel = skyMipLevel};
  for (int32 row = 0; row < 3; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      environmentConstants.worldToSkyRows[row][column] = key.worldToSky[row][column];
    }
  }

  graph.addPass("IBLEnvironment")
      .writeStorage(environment)
      .setExecute([this, environmentConstants, environment](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_environmentPipeline);
        commandList.pushConstants(&environmentConstants, sizeof(environmentConstants));
        const uint32 groups = getGroupCount(environmentConstants.size);
        commandList.dispatch(groups, groups, 6);

        // Each mip averages the one above, so it waits for its writes.
        const TextureBarrier barrier{.texture = &context.getTexture(environment),
                                     .before = ResourceState::UnorderedAccess,
                                     .after = ResourceState::UnorderedAccess};
        commandList.bindPipeline(*m_downsamplePipeline);
        for (uint32 mip = 1; mip < m_environment.mipCount; ++mip) {
          commandList.barrier(Span<const TextureBarrier>(&barrier, 1));
          const uint32 mipSize = Math::max(m_environment.size >> mip, 1u);
          const DownsamplePushConstants downsampleConstants{
              .sourceIndex = m_environment.mipViews[mip - 1]->getStorageBindlessIndex(),
              .outputIndex = m_environment.mipViews[mip]->getStorageBindlessIndex(),
              .size = mipSize};
          commandList.pushConstants(&downsampleConstants, sizeof(downsampleConstants));
          commandList.dispatch(getGroupCount(mipSize), getGroupCount(mipSize), 6);
        }
      });

  const uint32 environmentIndex = m_environment.view->getBindlessIndex();
  const float environmentMaxMip = static_cast<float>(m_environment.mipCount - 1);
  const IrradiancePushConstants irradianceConstants{
      .environmentIndex = environmentIndex,
      .samplerIndex = m_sampler->getBindlessIndex(),
      .outputIndex = m_irradiance.mipViews[0]->getStorageBindlessIndex(),
      .size = key.irradianceSize,
      .sampleCount = key.irradianceSamples,
      .environmentSize = key.cubeSize,
      .environmentMaxMip = environmentMaxMip};
  graph.addPass("IBLIrradiance")
      .read(environment)
      .writeStorage(view.irradiance)
      .setExecute([this, irradianceConstants](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_irradiancePipeline);
        commandList.pushConstants(&irradianceConstants, sizeof(irradianceConstants));
        const uint32 groups = getGroupCount(irradianceConstants.size);
        commandList.dispatch(groups, groups, 6);
      });

  const PrefilterPushConstants prefilterConstants{
      .environmentIndex = environmentIndex,
      .samplerIndex = m_sampler->getBindlessIndex(),
      .outputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .size = 0,
      .sampleCount = key.prefilterSamples,
      .environmentSize = key.cubeSize,
      .environmentMaxMip = environmentMaxMip,
      .roughness = 0.0f};
  graph.addPass("IBLPrefilter")
      .read(environment)
      .writeStorage(view.prefiltered)
      .setExecute([this, prefilterConstants](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_prefilterPipeline);
        // Every mip reads only the environment, so they need no barriers between them.
        for (uint32 mip = 0; mip < m_prefiltered.mipCount; ++mip) {
          PrefilterPushConstants constants = prefilterConstants;
          constants.outputIndex = m_prefiltered.mipViews[mip]->getStorageBindlessIndex();
          constants.size = Math::max(m_prefiltered.size >> mip, 1u);
          constants.roughness =
              static_cast<float>(mip) / static_cast<float>(m_prefiltered.mipCount - 1);
          commandList.pushConstants(&constants, sizeof(constants));
          commandList.dispatch(getGroupCount(constants.size), getGroupCount(constants.size),
                               6);
        }
      });
}

/*
 */
void
ImageBasedLighting::updateCube(CubeTexture& cube, uint32 size, uint32 mipCount)
{
  if (cube.texture != nullptr && cube.size == size && cube.mipCount == mipCount) {
    return;
  }

  // The old cube is freed by the deferred deletion once the GPU is done with it.
  cube.texture = IGraphicsAPI::instance().createTexture(
      {.type = TextureType::TextureCube,
       .format = kCubeFormat,
       .width = size,
       .height = size,
       .mipLevels = mipCount,
       .arrayLayers = 6,
       .usage = TextureUsageFlags(TextureUsage::Sampled) | TextureUsage::Storage});
  cube.view = cube.texture->createView({.viewType = TextureViewType::ViewCube});
  cube.mipViews.clear();
  for (uint32 mip = 0; mip < mipCount; ++mip) {
    cube.mipViews.push_back(cube.texture->createView(
        {.viewType = TextureViewType::View2DArray, .baseMipLevel = mip, .mipLevelCount = 1}));
  }
  cube.size = size;
  cube.mipCount = mipCount;
}

} // namespace chEngineSDK
