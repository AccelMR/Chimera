/************************************************************************/
/**
 * @file chImageBasedLighting.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Precomputes the data the lit paths read to light surfaces with the environment.
 */
/************************************************************************/

#include "chImageBasedLighting.h"

#include "chConsoleVariable.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chMath.h"

namespace chEngineSDK {

namespace {
ConsoleVariable<int32> g_cvarBrdfLutSize(
    "Renderer.IBLBrdfLutSize",
    128,
    "Width and height of the split sum lookup table of the image based lighting (32 to "
    "1024). The table is smooth, so a small one is enough.");

ConsoleVariable<int32> g_cvarBrdfLutSamples(
    "Renderer.IBLBrdfLutSamples",
    512,
    "GGX samples per texel of the split sum lookup table (16 to 8192). Built once, so its "
    "cost does not matter per frame.");

constexpr Format kBrdfLutFormat = Format::R16G16_SFLOAT;

// Must match numthreads in brdflut.hlsl.
constexpr uint32 kBrdfLutGroupSize = 8;

// Must match PushConstants in brdflut.hlsl.
struct BrdfLutPushConstants
{
  uint32 outputIndex;
  uint32 size;
  uint32 sampleCount;
};
} // namespace

/*
 */
ImageBasedLighting::ImageBasedLighting()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_brdfLutPipeline = graphicsAPI.createComputePipeline(
      {.computeShader = graphicsAPI.loadShader(ShaderStage::Compute, "brdflut")});
}

/*
 */
ImageBasedLighting::~ImageBasedLighting() = default;

/*
 */
ImageBasedLightingView
ImageBasedLighting::addPasses(RenderGraph& graph)
{
  ImageBasedLightingView view;
  addBrdfLutPass(graph, view);
  return view;
}

/*
 */
void
ImageBasedLighting::addBrdfLutPass(RenderGraph& graph, ImageBasedLightingView& view)
{
  const uint32 size = static_cast<uint32>(Math::clamp(g_cvarBrdfLutSize.get(), 32, 1024));
  const uint32 samples =
      static_cast<uint32>(Math::clamp(g_cvarBrdfLutSamples.get(), 16, 8192));
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
        const uint32 groupCount = (size + kBrdfLutGroupSize - 1) / kBrdfLutGroupSize;
        commandList.dispatch(groupCount, groupCount);
      });
}

} // namespace chEngineSDK
