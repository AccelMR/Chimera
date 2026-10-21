/************************************************************************/
/**
 * @file chForwardRenderPath.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws the scene items lit by the scene lights, in one pass, into the output.
 */
/************************************************************************/

#include "chForwardRenderPath.h"

#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chMaterial.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chRenderItem.h"

namespace chEngineSDK {

namespace {
// Must match PushConstants in forward.hlsl.
struct DrawPushConstants
{
  Matrix4 model;
  uint32 viewIndex;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 padding;
};
static_assert(sizeof(DrawPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr Format kDepthFormat = Format::D32_SFLOAT;
} // namespace

/*
 */
ForwardRenderPath::ForwardRenderPath()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});

  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "forward");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "forward");
  m_maskedFragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "forward_masked");
}

/*
 */
ForwardRenderPath::~ForwardRenderPath() = default;

/*
 */
RGTextureHandle
ForwardRenderPath::addPasses(RenderGraph& graph,
                             const RenderView& view,
                             RGTextureHandle output)
{
  const RGTextureDesc outputDesc = graph.getTextureDesc(output);
  const RGTextureHandle depth = graph.createTexture(
      "Depth", {.format = kDepthFormat, .width = outputDesc.width, .height = outputDesc.height});

  // Looked up now, while building, so the pass code does not depend on the output format.
  updatePipeline(outputDesc.format);
  const uint32 viewIndex = m_viewBuffers.update(view);

  RenderPassBuilder pass = graph.addPass("Forward");
  pass.setOptional();
  pass.writeColor(output, LoadOp::Clear, view.clearColor).writeDepth(depth);
  for (const ShadowView& shadow : view.shadows) {
    pass.read(shadow.texture);
  }
  view.ibl.declareReads(pass);
  pass.setExecute([this, items = view.items, viewIndex](RenderPassContext& context) {
    recordDraws(context, items, viewIndex);
  });
  return depth;
}

/*
 */
void
ForwardRenderPath::recordDraws(RenderPassContext& context,
                               Span<const RenderItem* const> items,
                               uint32 viewIndex)
{
  ICommandList& commandList = context.getCommandList();
  DrawPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                  .viewIndex = viewIndex,
                                  .materialIndex = 0,
                                  .samplerIndex = m_sampler->getBindlessIndex(),
                                  .padding = 0};
  m_pipelines.recordDraws(
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
ForwardRenderPath::updatePipeline(Format colorFormat)
{
  if (m_pipelines.isBuilt() && m_pipelineFormat == colorFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexStatic::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = colorFormat;
  m_pipelines.build(m_pipelineCache, pipelineDesc, m_maskedFragmentShader);
  m_pipelineFormat = colorFormat;
}

} // namespace chEngineSDK
