/************************************************************************/
/**
 * @file chNastyRenderer.cpp
 * @author AccelMR
 * @date 2025/07/14
 * @brief Temporary forward renderer, replaced by the render graph.
 */
/************************************************************************/

#include "chNastyRenderer.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chLogger.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chScene.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(NastyRendererSystem, All);

namespace {
// Must match CameraData in cube.hlsl.
struct CameraData {
  Matrix4 view;
  Matrix4 projection;
};

// Must match PushConstants in cube.hlsl.
struct DrawPushConstants {
  Matrix4 model;
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
NastyRenderer::~NastyRenderer()
{
  cleanup();
}

/*
 */
void
NastyRenderer::initialize(uint32 width, uint32 height, Format colorFormat)
{
  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);
  m_renderWidth = width;
  m_renderHeight = height;
  m_colorFormat = colorFormat;

  createDepthTarget();
  initializeRenderResources();
}

/*
 */
void
NastyRenderer::onRender(ICommandList& commandList,
                        const ITextureView& colorTarget,
                        const Scene& scene,
                        const Camera& camera)
{
  IBuffer& cameraBuffer = *m_cameraBuffers[IGraphicsAPI::instance().getFrameIndex()];
  const CameraData cameraData{.view = camera.getViewMatrix(),
                              .projection = camera.getProjectionMatrix()};
  cameraBuffer.update(&cameraData, sizeof(cameraData));

  scene.gatherRenderItems(camera.getFrustum(), m_visibleItems);

  // The depth target is cleared, so its old contents (and layout) are not needed.
  const Array<TextureBarrier, 1> toRendering = {
      TextureBarrier{.texture = m_depthTarget.get(),
                     .before = ResourceState::Undefined,
                     .after = ResourceState::DepthWrite}};
  commandList.barrier(toRendering);

  RenderingDesc renderingDesc{.colorAttachmentCount = 1,
                              .depthAttachment = {.view = m_depthTargetView.get()},
                              .width = m_renderWidth,
                              .height = m_renderHeight};
  renderingDesc.colorAttachments[0] = {
      .view = &colorTarget,
      .clearColor = m_clearColors.empty() ? LinearColor::Black : m_clearColors[0]};

  commandList.beginRendering(renderingDesc);
  commandList.setViewport(0, 0, static_cast<float>(m_renderWidth),
                          static_cast<float>(m_renderHeight));
  commandList.setScissor(0, 0, m_renderWidth, m_renderHeight);
  commandList.bindPipeline(*m_pipeline);

  DrawPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                  .cameraIndex = cameraBuffer.getBindlessIndex(),
                                  .textureIndex = 0,
                                  .samplerIndex = m_sampler->getBindlessIndex(),
                                  .padding = 0};
  for (const RenderItem* item : m_visibleItems) {
    const IBuffer* vertexBuffer = item->mesh->getVertexBuffer();
    const IBuffer* indexBuffer = item->mesh->getIndexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      continue;
    }

    const ITexture& texture = item->texture ? *item->texture : *m_defaultTexture;
    pushConstants.model = item->worldMatrix;
    pushConstants.textureIndex = texture.getBindlessIndex();
    commandList.pushConstants(&pushConstants, sizeof(pushConstants));

    commandList.bindVertexBuffer(*vertexBuffer);
    commandList.bindIndexBuffer(*indexBuffer, item->mesh->getIndexType());
    commandList.drawIndexed(item->mesh->getIndexCount());
  }

  commandList.endRendering();
}

/*
 */
void
NastyRenderer::resize(uint32 width, uint32 height)
{
  m_renderWidth = width;
  m_renderHeight = height;

  // The old depth target goes through the deferred deletion, so the GPU can keep using it
  // until the frames in flight are done.
  createDepthTarget();
}

/*
 */
void
NastyRenderer::cleanup()
{
  if (!m_pipeline) {
    return;
  }
  IGraphicsAPI::instance().waitIdle();

  m_pipeline = nullptr;
  m_pipelineCache.clear();
  m_vertexShader.reset();
  m_fragmentShader.reset();
  for (SPtr<IBuffer>& cameraBuffer : m_cameraBuffers) {
    cameraBuffer.reset();
  }

  m_depthTargetView.reset();
  m_depthTarget.reset();

  m_defaultTexture.reset();
  m_sampler.reset();
}

/*
 */
void
NastyRenderer::createDepthTarget()
{
  m_depthTarget =
      IGraphicsAPI::instance().createTexture({.format = kDepthFormat,
                                              .width = m_renderWidth,
                                              .height = m_renderHeight,
                                              .usage = TextureUsage::DepthStencil});
  m_depthTargetView = m_depthTarget->createView();
}

/*
 */
void
NastyRenderer::initializeRenderResources()
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

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexNormalTexCoord::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = m_colorFormat;
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();

  CH_LOG_INFO(NastyRendererSystem, "Render resources initialized");
}

} // namespace chEngineSDK
