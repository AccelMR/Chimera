/************************************************************************/
/**
 * @file chDeferredRenderPath.cpp
 * @author AccelMR
 * @date 2026/10/13
 * @brief Draws the scene surfaces into a GBuffer, then lights every pixel once.
 */
/************************************************************************/

#include "chDeferredRenderPath.h"

#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chITextureView.h"
#include "chMaterial.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chRenderItem.h"

namespace chEngineSDK {

namespace {
// Lit color, alpha unused. sRGB keeps the darks as precise as the textures they come from.
constexpr Format kBaseColorFormat = Format::R8G8B8A8_SRGB;
// World normal, ten bits per axis.
constexpr Format kNormalFormat = Format::A2B10G10R10_UNORM;
// Metallic, roughness, occlusion.
constexpr Format kSurfaceFormat = Format::R8G8B8A8_UNORM;
// Emissive goes above 1, so it needs floats; the alpha is not needed.
constexpr Format kEmissiveFormat = Format::B10G11R11_UFLOAT;
constexpr Format kDepthFormat = Format::D32_SFLOAT;

// Must match PushConstants in gbuffer.hlsl.
struct GBufferPushConstants
{
  Matrix4 model;
  uint32 viewIndex;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 padding;
};
static_assert(sizeof(GBufferPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

// Bindless indexes of the GBuffer textures, the first members of the push constants of
// every pass that reads the GBuffer.
struct GBufferTextures
{
  uint32 baseColorIndex;
  uint32 normalIndex;
  uint32 surfaceIndex;
  uint32 emissiveIndex;
  uint32 depthIndex;
};

// GBufferHandles is DeferredRenderPath::GBuffer, which is private.
template<typename GBufferHandles>
NODISCARD GBufferTextures
getGBufferTextures(const RenderPassContext& context, const GBufferHandles& gbuffer)
{
  return {.baseColorIndex = context.getView(gbuffer.baseColor).getBindlessIndex(),
          .normalIndex = context.getView(gbuffer.normal).getBindlessIndex(),
          .surfaceIndex = context.getView(gbuffer.surface).getBindlessIndex(),
          .emissiveIndex = context.getView(gbuffer.emissive).getBindlessIndex(),
          .depthIndex = context.getView(gbuffer.depth).getBindlessIndex()};
}

// Must match PushConstants in deferredlighting.hlsl.
struct LightingPushConstants
{
  GBufferTextures gbuffer;
  uint32 viewIndex;
  float inverseSize[2];
};
static_assert(sizeof(LightingPushConstants) == 32);

// Must match PushConstants in gbufferview.hlsl.
struct GBufferViewPushConstants
{
  GBufferTextures gbuffer;
  uint32 mode;
};
static_assert(sizeof(GBufferViewPushConstants) == 24);

// Must match the kMode constants in gbufferview.hlsl.
enum class GBufferViewMode : uint32
{
  BaseColor,
  Metallic,
  Roughness,
  Occlusion,
  Emissive
};

NODISCARD GBufferViewMode
toGBufferViewMode(ViewMode mode)
{
  switch (mode) {
    case ViewMode::Metallic:
      return GBufferViewMode::Metallic;
    case ViewMode::Roughness:
      return GBufferViewMode::Roughness;
    case ViewMode::AmbientOcclusion:
      return GBufferViewMode::Occlusion;
    case ViewMode::Emissive:
      return GBufferViewMode::Emissive;
    case ViewMode::BaseColor:
      break;
    default:
      CH_ASSERT(false && "Not a GBuffer view mode.");
      break;
  }
  return GBufferViewMode::BaseColor;
}
} // namespace

/*
 */
DeferredRenderPath::DeferredRenderPath()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});

  m_gbufferVertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "gbuffer");
  m_gbufferFragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "gbuffer");
  m_gbufferMaskedFragmentShader =
      graphicsAPI.loadShader(ShaderStage::Fragment, "gbuffer_masked");
  m_lighting.vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "deferredlighting");
  m_lighting.fragmentShader =
      graphicsAPI.loadShader(ShaderStage::Fragment, "deferredlighting");
  m_gbufferView.vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "gbufferview");
  m_gbufferView.fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "gbufferview");
}

/*
 */
DeferredRenderPath::~DeferredRenderPath() = default;

/*
 */
RGTextureHandle
DeferredRenderPath::addPasses(RenderGraph& graph,
                              const RenderView& view,
                              RGTextureHandle output)
{
  const RGTextureDesc outputDesc = graph.getTextureDesc(output);
  const uint32 viewIndex = m_viewBuffers.update(view);
  const GBuffer gbuffer =
      addGBufferPass(graph, view, viewIndex, outputDesc.width, outputDesc.height);
  updateFullscreenPipeline(m_lighting, outputDesc.format);

  graph.addPass("DeferredLighting")
      .read(gbuffer.baseColor)
      .read(gbuffer.normal)
      .read(gbuffer.surface)
      .read(gbuffer.emissive)
      .read(gbuffer.depth)
      .writeColor(output, LoadOp::Clear, view.clearColor)
      .setExecute([this, gbuffer, viewIndex](RenderPassContext& context) {
        const LightingPushConstants pushConstants{
            .gbuffer = getGBufferTextures(context, gbuffer),
            .viewIndex = viewIndex,
            .inverseSize = {1.0f / static_cast<float>(context.getWidth()),
                            1.0f / static_cast<float>(context.getHeight())}};
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_lighting.pipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.draw(3);
      });
  return gbuffer.depth;
}

/*
 */
RGTextureHandle
DeferredRenderPath::addGBufferViewPasses(RenderGraph& graph,
                                         const RenderView& view,
                                         RGTextureHandle output,
                                         ViewMode mode)
{
  const RGTextureDesc outputDesc = graph.getTextureDesc(output);
  const uint32 viewIndex = m_viewBuffers.update(view);
  const GBuffer gbuffer =
      addGBufferPass(graph, view, viewIndex, outputDesc.width, outputDesc.height);
  updateFullscreenPipeline(m_gbufferView, outputDesc.format);

  const uint32 shaderMode = static_cast<uint32>(toGBufferViewMode(mode));
  graph.addPass("GBufferView")
      .read(gbuffer.baseColor)
      .read(gbuffer.normal)
      .read(gbuffer.surface)
      .read(gbuffer.emissive)
      .read(gbuffer.depth)
      .writeColor(output, LoadOp::Clear, view.clearColor)
      .setExecute([this, gbuffer, shaderMode](RenderPassContext& context) {
        const GBufferViewPushConstants pushConstants{
            .gbuffer = getGBufferTextures(context, gbuffer),
            .mode = shaderMode};
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_gbufferView.pipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.draw(3);
      });
  return gbuffer.depth;
}

/*
 */
DeferredRenderPath::GBuffer
DeferredRenderPath::addGBufferPass(RenderGraph& graph,
                                   const RenderView& view,
                                   uint32 viewIndex,
                                   uint32 width,
                                   uint32 height)
{
  const GBuffer gbuffer{
      .baseColor = graph.createTexture(
          "GBufferBaseColor", {.format = kBaseColorFormat, .width = width, .height = height}),
      .normal = graph.createTexture(
          "GBufferNormal", {.format = kNormalFormat, .width = width, .height = height}),
      .surface = graph.createTexture(
          "GBufferSurface", {.format = kSurfaceFormat, .width = width, .height = height}),
      .emissive = graph.createTexture(
          "GBufferEmissive", {.format = kEmissiveFormat, .width = width, .height = height}),
      .depth = graph.createTexture(
          "Depth", {.format = kDepthFormat, .width = width, .height = height})};

  updateGBufferPipeline();

  // The color targets are not cleared: the passes that read them skip the pixels where the
  // depth is still clear, the only ones nothing wrote.
  graph.addPass("GBuffer")
      .writeColor(gbuffer.baseColor, LoadOp::DontCare)
      .writeColor(gbuffer.normal, LoadOp::DontCare)
      .writeColor(gbuffer.surface, LoadOp::DontCare)
      .writeColor(gbuffer.emissive, LoadOp::DontCare)
      .writeDepth(gbuffer.depth)
      .setExecute([this, items = view.items, viewIndex](RenderPassContext& context) {
        recordGBufferDraws(context, items, viewIndex);
      });
  return gbuffer;
}

/*
 */
void
DeferredRenderPath::recordGBufferDraws(RenderPassContext& context,
                                       Span<const RenderItem* const> items,
                                       uint32 viewIndex)
{
  ICommandList& commandList = context.getCommandList();
  GBufferPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                     .viewIndex = viewIndex,
                                     .materialIndex = 0,
                                     .samplerIndex = m_sampler->getBindlessIndex(),
                                     .padding = 0};
  m_gbufferPipelines.recordDraws(
      commandList, items, m_defaultMaterial,
      [&commandList, &pushConstants](const RenderItem& item, const Material& material) {
        pushConstants.model = item.worldMatrix;
        pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
      });
}

/*
 */
void
DeferredRenderPath::updateGBufferPipeline()
{
  if (m_gbufferPipelines.isBuilt()) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_gbufferVertexShader,
                                    .fragmentShader = m_gbufferFragmentShader,
                                    .vertexLayout = VertexStatic::getLayout(),
                                    .colorAttachmentCount = 4,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = kBaseColorFormat;
  pipelineDesc.colorFormats[1] = kNormalFormat;
  pipelineDesc.colorFormats[2] = kSurfaceFormat;
  pipelineDesc.colorFormats[3] = kEmissiveFormat;
  m_gbufferPipelines.build(m_pipelineCache, pipelineDesc, m_gbufferMaskedFragmentShader);
}

/*
 */
void
DeferredRenderPath::updateFullscreenPipeline(FullscreenPipeline& fullscreen,
                                             Format colorFormat)
{
  if (fullscreen.pipeline != nullptr && fullscreen.colorFormat == colorFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = fullscreen.vertexShader,
                                    .fragmentShader = fullscreen.fragmentShader,
                                    .raster = {.cullMode = CullMode::None},
                                    .depth = {.testEnable = false, .writeEnable = false},
                                    .colorAttachmentCount = 1};
  pipelineDesc.colorFormats[0] = colorFormat;
  fullscreen.pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  fullscreen.colorFormat = colorFormat;
}

} // namespace chEngineSDK
