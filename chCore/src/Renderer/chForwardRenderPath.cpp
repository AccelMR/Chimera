/************************************************************************/
/**
 * @file chForwardRenderPath.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws the scene items textured, in one pass, straight into the output.
 */
/************************************************************************/

#include "chForwardRenderPath.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chMaterial.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chRenderItem.h"

namespace chEngineSDK {

namespace {
// Must match CameraData in cube.hlsl.
struct CameraData
{
  Matrix4 view;
  Matrix4 projection;
};

// Must match PushConstants in cube.hlsl.
struct DrawPushConstants
{
  Matrix4 model;
  float baseColorFactor[4];
  uint32 cameraIndex;
  uint32 textureIndex;
  uint32 samplerIndex;
  uint32 padding;
};
static_assert(sizeof(DrawPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr Format kTextureFormat = Format::R8G8B8A8_UNORM;
constexpr Format kDepthFormat = Format::D32_SFLOAT;
} // namespace

/*
 */
ForwardRenderPath::ForwardRenderPath()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();

  for (SPtr<IBuffer>& cameraBuffer : m_cameraBuffers) {
    cameraBuffer = graphicsAPI.createBuffer({.size = sizeof(CameraData),
                                             .usage = BufferUsage::UniformBuffer,
                                             .memoryUsage = MemoryUsage::CpuToGpu});
  }

  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});

  constexpr uint32 kWhitePixel = 0xFFFFFFFF;
  m_defaultTexture = graphicsAPI.createTexture({.format = kTextureFormat,
                                                .initialData = &kWhitePixel,
                                                .initialDataSize = sizeof(kWhitePixel)});

  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "cube");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "cube");
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

  graph.addPass("Forward")
      .writeColor(output, LoadOp::Clear, view.clearColor)
      .writeDepth(depth)
      .setExecute([this, camera = &view.camera, items = view.items](RenderPassContext& context) {
        recordDraws(context, *camera, items);
      });
  return depth;
}

/*
 */
void
ForwardRenderPath::recordDraws(RenderPassContext& context,
                               const Camera& camera,
                               Span<const RenderItem* const> items)
{
  ICommandList& commandList = context.getCommandList();

  IBuffer& cameraBuffer = *m_cameraBuffers[IGraphicsAPI::instance().getFrameIndex()];
  const CameraData cameraData{.view = camera.getViewMatrix(),
                              .projection = camera.getProjectionMatrix()};
  cameraBuffer.update(&cameraData, sizeof(cameraData));

  commandList.bindPipeline(*m_pipeline);

  DrawPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                  .baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f},
                                  .cameraIndex = cameraBuffer.getBindlessIndex(),
                                  .textureIndex = 0,
                                  .samplerIndex = m_sampler->getBindlessIndex(),
                                  .padding = 0};
  for (const RenderItem* item : items) {
    const IBuffer* vertexBuffer = item->mesh->getVertexBuffer();
    const IBuffer* indexBuffer = item->mesh->getIndexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      continue;
    }

    const Material* material = item->material;
    const ITexture* texture = material ? material->getBaseColorGpuTexture() : nullptr;
    const LinearColor& baseColor =
        material ? material->getBaseColorFactor() : LinearColor::White;
    pushConstants.model = item->worldMatrix;
    pushConstants.baseColorFactor[0] = baseColor.r;
    pushConstants.baseColorFactor[1] = baseColor.g;
    pushConstants.baseColorFactor[2] = baseColor.b;
    pushConstants.baseColorFactor[3] = baseColor.a;
    pushConstants.textureIndex = (texture ? *texture : *m_defaultTexture).getBindlessIndex();
    commandList.pushConstants(&pushConstants, sizeof(pushConstants));

    commandList.bindVertexBuffer(*vertexBuffer);
    commandList.bindIndexBuffer(*indexBuffer, item->mesh->getIndexType());
    commandList.drawIndexed(item->mesh->getIndexCount());
  }
}

/*
 */
void
ForwardRenderPath::updatePipeline(Format colorFormat)
{
  if (m_pipeline != nullptr && m_pipelineFormat == colorFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexNormalTexCoord::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = colorFormat;
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  m_pipelineFormat = colorFormat;
}

} // namespace chEngineSDK
