/************************************************************************/
/**
 * @file chScreenSpaceAO.cpp
 * @author AccelMR
 * @date 2026/10/16
 * @brief Measures how much nearby surfaces hide each pixel from the ambient light.
 */
/************************************************************************/

#include "chScreenSpaceAO.h"

#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chITextureView.h"
#include "chMath.h"
#include "chRenderSettings.h"

namespace chEngineSDK {

namespace {
// 1 is no occlusion; eight bits are more than the blur leaves visible.
constexpr Format kOcclusionFormat = Format::R8_UNORM;

// Must match numthreads in ssao.hlsl, ssaoblur.hlsl and ssaoupsample.hlsl.
constexpr uint32 kGroupSize = 8;

// Must match PushConstants in ssao.hlsl.
struct OcclusionPushConstants
{
  uint32 depthIndex;
  uint32 normalIndex;
  uint32 outputIndex;
  uint32 viewIndex;
  uint32 outputWidth;
  uint32 outputHeight;
  uint32 depthWidth;
  uint32 depthHeight;
  uint32 resolutionScale;
  uint32 sliceCount;
  uint32 stepCount;
  float radius;
  float falloffRange;
  float power;
  float maxPixelRadius;
};
static_assert(sizeof(OcclusionPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

// Must match PushConstants in ssaoblur.hlsl.
struct BlurPushConstants
{
  uint32 inputIndex;
  uint32 outputIndex;
  uint32 depthIndex;
  uint32 viewIndex;
  uint32 width;
  uint32 height;
  uint32 depthWidth;
  uint32 depthHeight;
  uint32 resolutionScale;
  int32 directionX;
  int32 directionY;
  uint32 radius;
  float depthTolerance;
};
static_assert(sizeof(BlurPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

// Must match PushConstants in ssaoupsample.hlsl.
struct UpsamplePushConstants
{
  uint32 inputIndex;
  uint32 outputIndex;
  uint32 depthIndex;
  uint32 viewIndex;
  uint32 inputWidth;
  uint32 inputHeight;
  uint32 width;
  uint32 height;
  float depthTolerance;
};
static_assert(sizeof(UpsamplePushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

NODISCARD uint32
getGroupCount(uint32 size)
{
  return (size + kGroupSize - 1) / kGroupSize;
}

NODISCARD SPtr<IPipeline>
createComputePipeline(IGraphicsAPI& graphicsAPI, StringView shader)
{
  return graphicsAPI.createComputePipeline(
      {.computeShader = graphicsAPI.loadShader(ShaderStage::Compute, shader)});
}

NODISCARD float
getDepthTolerance()
{
  return Math::clamp(g_cvarSSAODepthTolerance.get(), 0.001f, 1.0f);
}
} // namespace

/*
 */
ScreenSpaceAO::ScreenSpaceAO()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_occlusionPipeline = createComputePipeline(graphicsAPI, "ssao");
  m_blurPipeline = createComputePipeline(graphicsAPI, "ssaoblur");
  m_upsamplePipeline = createComputePipeline(graphicsAPI, "ssaoupsample");
}

/*
 */
ScreenSpaceAO::~ScreenSpaceAO() = default;

/*
 */
RGTextureHandle
ScreenSpaceAO::addPasses(RenderGraph& graph,
                         RGTextureHandle depth,
                         RGTextureHandle normal,
                         uint32 viewIndex)
{
  if (!g_cvarSSAO.get()) {
    return {};
  }

  const RGTextureDesc depthDesc = graph.getTextureDesc(depth);
  const uint32 resolutionScale = g_cvarSSAOHalfResolution.get() ? 2 : 1;
  const uint32 width = (depthDesc.width + resolutionScale - 1) / resolutionScale;
  const uint32 height = (depthDesc.height + resolutionScale - 1) / resolutionScale;

  const OcclusionPushConstants constants{
      .depthIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .normalIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .outputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .viewIndex = viewIndex,
      .outputWidth = width,
      .outputHeight = height,
      .depthWidth = depthDesc.width,
      .depthHeight = depthDesc.height,
      .resolutionScale = resolutionScale,
      .sliceCount = static_cast<uint32>(Math::clamp(g_cvarSSAOSlices.get(), 1, 8)),
      .stepCount = static_cast<uint32>(Math::clamp(g_cvarSSAOSteps.get(), 1, 16)),
      .radius = Math::max(g_cvarSSAORadius.get(), 0.0001f),
      .falloffRange = Math::clamp(g_cvarSSAOFalloff.get(), 0.01f, 1.0f),
      .power = Math::clamp(g_cvarSSAOPower.get(), 0.5f, 4.0f),
      .maxPixelRadius = Math::clamp(g_cvarSSAOMaxPixelRadius.get(), 16.0f, 1024.0f)};

  const RGTextureHandle occlusion =
      graph.createTexture("SSAO", {.format = kOcclusionFormat, .width = width, .height = height});
  graph.addPass("SSAO")
      .read(depth)
      .read(normal)
      .writeStorage(occlusion)
      .setExecute([this, constants, depth, normal, occlusion](RenderPassContext& context) {
        OcclusionPushConstants pushConstants = constants;
        pushConstants.depthIndex = context.getView(depth).getBindlessIndex();
        pushConstants.normalIndex = context.getView(normal).getBindlessIndex();
        pushConstants.outputIndex = context.getView(occlusion).getStorageBindlessIndex();
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_occlusionPipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.dispatch(getGroupCount(pushConstants.outputWidth),
                             getGroupCount(pushConstants.outputHeight));
      });

  RGTextureHandle result = occlusion;
  if (g_cvarSSAOBlurRadius.get() > 0) {
    result = addBlurPass(graph, "SSAOBlurX", result, depth, viewIndex, resolutionScale, 1, 0);
    result = addBlurPass(graph, "SSAOBlurY", result, depth, viewIndex, resolutionScale, 0, 1);
  }
  if (resolutionScale > 1) {
    result = addUpsamplePass(graph, result, depth, viewIndex);
  }
  return result;
}

/*
 */
RGTextureHandle
ScreenSpaceAO::addBlurPass(RenderGraph& graph,
                           StringView name,
                           RGTextureHandle input,
                           RGTextureHandle depth,
                           uint32 viewIndex,
                           uint32 resolutionScale,
                           int32 directionX,
                           int32 directionY)
{
  const RGTextureDesc inputDesc = graph.getTextureDesc(input);
  const RGTextureDesc depthDesc = graph.getTextureDesc(depth);
  const BlurPushConstants constants{
      .inputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .outputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .depthIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .viewIndex = viewIndex,
      .width = inputDesc.width,
      .height = inputDesc.height,
      .depthWidth = depthDesc.width,
      .depthHeight = depthDesc.height,
      .resolutionScale = resolutionScale,
      .directionX = directionX,
      .directionY = directionY,
      .radius = static_cast<uint32>(Math::clamp(g_cvarSSAOBlurRadius.get(), 0, 6)),
      .depthTolerance = getDepthTolerance()};

  const RGTextureHandle output = graph.createTexture(name, inputDesc);
  graph.addPass(name)
      .read(input)
      .read(depth)
      .writeStorage(output)
      .setExecute([this, constants, input, depth, output](RenderPassContext& context) {
        BlurPushConstants pushConstants = constants;
        pushConstants.inputIndex = context.getView(input).getBindlessIndex();
        pushConstants.outputIndex = context.getView(output).getStorageBindlessIndex();
        pushConstants.depthIndex = context.getView(depth).getBindlessIndex();
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_blurPipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.dispatch(getGroupCount(pushConstants.width),
                             getGroupCount(pushConstants.height));
      });
  return output;
}

/*
 */
RGTextureHandle
ScreenSpaceAO::addUpsamplePass(RenderGraph& graph,
                               RGTextureHandle input,
                               RGTextureHandle depth,
                               uint32 viewIndex)
{
  const RGTextureDesc inputDesc = graph.getTextureDesc(input);
  const RGTextureDesc depthDesc = graph.getTextureDesc(depth);
  const UpsamplePushConstants constants{
      .inputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .outputIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .depthIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .viewIndex = viewIndex,
      .inputWidth = inputDesc.width,
      .inputHeight = inputDesc.height,
      .width = depthDesc.width,
      .height = depthDesc.height,
      .depthTolerance = getDepthTolerance()};

  const RGTextureHandle output = graph.createTexture(
      "SSAOUpsample",
      {.format = kOcclusionFormat, .width = depthDesc.width, .height = depthDesc.height});
  graph.addPass("SSAOUpsample")
      .read(input)
      .read(depth)
      .writeStorage(output)
      .setExecute([this, constants, input, depth, output](RenderPassContext& context) {
        UpsamplePushConstants pushConstants = constants;
        pushConstants.inputIndex = context.getView(input).getBindlessIndex();
        pushConstants.outputIndex = context.getView(output).getStorageBindlessIndex();
        pushConstants.depthIndex = context.getView(depth).getBindlessIndex();
        ICommandList& commandList = context.getCommandList();
        commandList.bindPipeline(*m_upsamplePipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.dispatch(getGroupCount(pushConstants.width),
                             getGroupCount(pushConstants.height));
      });
  return output;
}

} // namespace chEngineSDK
