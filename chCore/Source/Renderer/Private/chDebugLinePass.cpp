/************************************************************************/
/**
 * @file chDebugLinePass.cpp
 * @author AccelMR
 * @date 2026/10/18
 * @brief Draws the lines of DebugDraw over the image.
 */
/************************************************************************/

#include "chDebugLinePass.h"

#include "chCamera.h"
#include "chDebugDraw.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chMath.h"

#if USING(CH_DEBUG_DRAW)
namespace chEngineSDK {

namespace {
// Must match PushConstants in debuglines.hlsl.
struct DebugLinePushConstants
{
  Matrix4 viewProjection;
  uint32 vertexBufferIndex;
  uint32 firstVertex;
};

// Enough for the bounds of a few hundred objects, so most frames never make it again.
constexpr uint32 kInitialVertexCapacity = 8192;

NODISCARD SPtr<IBuffer>
createVertexBuffer(uint32 capacity)
{
  return IGraphicsAPI::instance().createBuffer(
      {.size = static_cast<uint32>(capacity * sizeof(DebugLineVertex)),
       .usage = BufferUsage::StorageBuffer,
       .memoryUsage = MemoryUsage::CpuToGpu});
}
} // namespace

/*
 */
DebugLinePass::DebugLinePass()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "debuglines");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "debuglines");
}

/*
 */
DebugLinePass::~DebugLinePass() = default;

/*
 */
void
DebugLinePass::addPass(RenderGraph& graph,
                       const Camera& camera,
                       Span<const DebugDraw* const> lists,
                       RGTextureHandle output,
                       RGTextureHandle depth)
{
  uint32 counts[static_cast<uint32>(DebugDepth::COUNT)] = {};
  for (const DebugDraw* list : lists) {
    if (list) {
      counts[0] += static_cast<uint32>(list->getVertices(DebugDepth::Tested).size());
      counts[1] += static_cast<uint32>(list->getVertices(DebugDepth::OnTop).size());
    }
  }
  const uint32 total = counts[0] + counts[1];
  if (total == 0) {
    return;
  }

  const uint32 frameIndex = IGraphicsAPI::instance().getFrameIndex();
  uint32& capacity = m_capacities[frameIndex];
  if (total > capacity) {
    capacity = Math::max(total, Math::max(capacity * 2, kInitialVertexCapacity));
    m_vertexBuffers[frameIndex] = createVertexBuffer(capacity);
  }
  IBuffer& buffer = *m_vertexBuffers[frameIndex];

  // Tested lines of every list first, then the ones on top, so each kind is one draw.
  uint32 written = 0;
  for (const DebugDepth kind : {DebugDepth::Tested, DebugDepth::OnTop}) {
    for (const DebugDraw* list : lists) {
      if (!list) {
        continue;
      }
      const Span<const DebugLineVertex> vertices = list->getVertices(kind);
      if (!vertices.empty()) {
        buffer.update(vertices.data(), vertices.size_bytes(),
                      static_cast<uint32>(written * sizeof(DebugLineVertex)));
        written += static_cast<uint32>(vertices.size());
      }
    }
  }

  const bool bHasDepth = depth.isValid();
  const Format colorFormat = graph.getTextureDesc(output).format;
  const Format depthFormat = bHasDepth ? graph.getTextureDesc(depth).format : Format::Unknown;
  // Without depth nothing hides the tested lines, so they draw with the others.
  const IPipeline* testedPipeline =
      bHasDepth ? &getPipeline(true, colorFormat, depthFormat) : nullptr;
  const IPipeline* onTopPipeline = &getPipeline(false, colorFormat, depthFormat);
  const DebugLinePushConstants pushConstants{
      .viewProjection = camera.getViewProjectionMatrix(),
      .vertexBufferIndex = buffer.getBindlessIndex(),
      .firstVertex = 0};

  RenderPassBuilder pass = graph.addPass("DebugLines");
  pass.setOptional().writeColor(output, LoadOp::Load);
  if (bHasDepth) {
    pass.writeDepth(depth, LoadOp::Load);
  }
  pass.setExecute([testedPipeline, onTopPipeline, pushConstants, testedCount = counts[0],
                   total](RenderPassContext& context) {
    ICommandList& commandList = context.getCommandList();
    DebugLinePushConstants constants = pushConstants;
    uint32 first = 0;
    if (testedPipeline && testedCount > 0) {
      commandList.bindPipeline(*testedPipeline);
      commandList.pushConstants(&constants, sizeof(constants));
      commandList.draw(testedCount);
      first = testedCount;
    }
    if (first < total) {
      constants.firstVertex = first;
      commandList.bindPipeline(*onTopPipeline);
      commandList.pushConstants(&constants, sizeof(constants));
      commandList.draw(total - first);
    }
  });
}

/*
 */
const IPipeline&
DebugLinePass::getPipeline(bool bTested, Format colorFormat, Format depthFormat)
{
  PipelineEntry& entry = bTested ? m_testedPipeline : m_onTopPipeline;
  if (entry.pipeline != nullptr && entry.colorFormat == colorFormat &&
      entry.depthFormat == depthFormat) {
    return *entry.pipeline;
  }

  GraphicsPipelineDesc pipelineDesc{
      .vertexShader = m_vertexShader,
      .fragmentShader = m_fragmentShader,
      .topology = PrimitiveTopology::LineList,
      .raster = {.cullMode = CullMode::None},
      // Lines read the depth but never write it, so one line does not hide the next.
      .depth = {.testEnable = bTested,
                .writeEnable = false,
                .compareOp = CompareOp::LessOrEqual},
      .colorAttachmentCount = 1,
      .depthFormat = depthFormat};
  pipelineDesc.colorFormats[0] = colorFormat;
  pipelineDesc.blendStates[0].enable = true;
  entry.pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  entry.colorFormat = colorFormat;
  entry.depthFormat = depthFormat;
  return *entry.pipeline;
}

} // namespace chEngineSDK
#endif // USING(CH_DEBUG_DRAW)
