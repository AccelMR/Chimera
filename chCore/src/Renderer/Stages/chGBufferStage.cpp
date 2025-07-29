/************************************************************************/
/**
 * @file chGBufferStage.cpp
 * @author AccelMR
 * @date 2025/07/21
 * @brief G-Buffer generation stage implementation
 */
/************************************************************************/

#include "chGBufferStage.h"

#include "chLogger.h"
#include "chIGraphicsAPI.h"

// Graphics includes
#include "chIBuffer.h"
#include "chICommandBuffer.h"
#include "chICommandPool.h"
#include "chICommandQueue.h"
#include "chIFrameBuffer.h"
#include "chIPipeline.h"
#include "chIRenderPass.h"
#include "chISynchronization.h"
#include "chITexture.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_GBUFFER_STAGE_LOG_LEVEL All
#else
#define CH_GBUFFER_STAGE_LOG_LEVEL Info
#endif

CH_LOG_DECLARE_STATIC(GBufferStageLog, CH_GBUFFER_STAGE_LOG_LEVEL);

/*
 */
GBufferStage::GBufferStage() {
  CH_LOG_INFO(GBufferStageLog, "GBufferStage created");

  // Setup clear colors for G-Buffer
  m_clearColors = {
    LinearColor(0.0f, 0.0f, 0.0f, 0.0f),  // Albedo (black)
    LinearColor(0.5f, 0.5f, 1.0f, 0.0f),  // Normal (encoded as 0.5,0.5,1.0 = up)
    LinearColor(0.0f, 0.0f, 0.0f, 0.0f),  // Metallic (non-metallic)
    LinearColor(0.5f, 0.0f, 0.0f, 0.0f),  // Roughness (medium roughness)
    LinearColor(0.0f, 0.0f, 0.0f, 0.0f),  // Motion (no motion)
  };
}

/*
 */
GBufferStage::~GBufferStage() {
  CH_LOG_INFO(GBufferStageLog, "GBufferStage destroyed");
  cleanup();
}

/*
 */
void
GBufferStage::initialize(uint32 width, uint32 height) {
  CH_LOG_INFO(GBufferStageLog, "Initializing GBufferStage with dimensions: {0}x{1}",
              width, height);

  if (!IGraphicsAPI::instancePtr()) {
    CH_LOG_ERROR(GBufferStageLog, "Graphics API not initialized");
    return;
  }

  m_width = width;
  m_height = height;

  createGBufferTargets();
  createGBufferRenderPass();
  createGBufferPipeline();
  createCommandResources();

  CH_LOG_INFO(GBufferStageLog, "GBufferStage initialized successfully");
}

/*
 */
bool
GBufferStage::execute(const RenderStageIO& inputs, RenderStageIO& outputs, float deltaTime) {
  // Early return if not enabled
  if (!m_enabled) {
    return false;
  }

  // Camera is required
  if (!inputs.hasValidInput<CameraData>()) {
    CH_LOG_WARNING(GBufferStageLog, "Missing or invalid camera data");
    return false;
  }

  // Model is optional - get it if available
  auto cameraData = inputs.getInput<CameraData>();
  auto modelData = inputs.getInput<ModelData>(); // Can be nullptr

  // Render geometry to G-Buffer
  if (!renderGeometry(cameraData->camera,
                     modelData ? modelData->model : nullptr,
                     deltaTime)) {
    CH_LOG_ERROR(GBufferStageLog, "Failed to render geometry");
    return false;
  }

  // Create and populate output data (same as before)
  auto albedoOutput = chMakeShared<AlbedoData>();
  albedoOutput->texture = m_albedoView;
  outputs.setOutput<AlbedoData>(albedoOutput);

  auto normalOutput = chMakeShared<NormalData>();
  normalOutput->texture = m_normalView;
  outputs.setOutput<NormalData>(normalOutput);

  auto metallicOutput = chMakeShared<MetallicData>();
  metallicOutput->texture = m_metallicView;
  outputs.setOutput<MetallicData>(metallicOutput);

  auto roughnessOutput = chMakeShared<RoughnessData>();
  roughnessOutput->texture = m_roughnessView;
  outputs.setOutput<RoughnessData>(roughnessOutput);

  auto depthOutput = chMakeShared<DepthData>();
  depthOutput->texture = m_depthView;
  outputs.setOutput<DepthData>(depthOutput);

  auto motionOutput = chMakeShared<MotionVectorData>();
  motionOutput->texture = m_motionView;
  outputs.setOutput<MotionVectorData>(motionOutput);

  return true;
}

/*
 */
void
GBufferStage::resize(uint32 width, uint32 height) {
  CH_LOG_INFO(GBufferStageLog, "Resizing GBufferStage to {0}x{1}", width, height);

  auto& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  if (m_fence) {
    m_fence->wait();
  }

  m_width = width;
  m_height = height;

  // Recreate targets and render pass with new dimensions
  createGBufferTargets();
  createGBufferRenderPass();

  // Pipeline doesn't need recreation for resize

  CH_LOG_INFO(GBufferStageLog, "GBufferStage resized successfully");
}

/*
 */
void
GBufferStage::cleanup() {
  CH_LOG_INFO(GBufferStageLog, "Cleaning up GBufferStage");

  if (IGraphicsAPI::instancePtr()) {
    auto& graphicsAPI = IGraphicsAPI::instance();
    graphicsAPI.waitIdle();

    if (m_fence) {
      m_fence->wait();
    }
  }

  // Reset all resources
  m_commandBuffer.reset();
  m_commandPool.reset();
  m_fence.reset();
  m_pipeline.reset();
  m_framebuffer.reset();
  m_renderPass.reset();

  // Reset texture views
  m_albedoView.reset();
  m_normalView.reset();
  m_metallicView.reset();
  m_roughnessView.reset();
  m_depthView.reset();
  m_motionView.reset();

  // Reset textures
  m_albedoTarget.reset();
  m_normalTarget.reset();
  m_metallicTarget.reset();
  m_roughnessTarget.reset();
  m_depthTarget.reset();
  m_motionTarget.reset();

  CH_LOG_INFO(GBufferStageLog, "GBufferStage cleanup completed");
}

/*
 */
void
GBufferStage::createGBufferTargets() {
   if (m_width == 0 || m_height == 0) {
    CH_LOG_ERROR(GBufferStageLog, "Invalid dimensions for G-Buffer targets");
    return;
  }

  auto& graphicsAPI = IGraphicsAPI::instance();

  // Albedo Target (RGBA8 - RGB: albedo, A: unused)
  TextureCreateInfo albedoInfo{
    .type = TextureType::Texture2D,
    .format = Format::R8G8B8A8_UNORM,
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled
  };
  m_albedoTarget = graphicsAPI.createTexture(albedoInfo);

  TextureViewCreateInfo albedoViewInfo{
    .format = Format::R8G8B8A8_UNORM,
    .viewType = TextureViewType::View2D
  };
  m_albedoView = m_albedoTarget->createView(albedoViewInfo);

  // Normal Target (RGBA16F - RGB: world normal, A: unused)
  // Using 16F for better precision on normals
  TextureCreateInfo normalInfo{
    .type = TextureType::Texture2D,
    .format = Format::R16G16B16A16_SFLOAT,
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled
  };
  m_normalTarget = graphicsAPI.createTexture(normalInfo);

  TextureViewCreateInfo normalViewInfo{
    .format = Format::R16G16B16A16_SFLOAT,
    .viewType = TextureViewType::View2D
  };
  m_normalView = m_normalTarget->createView(normalViewInfo);

  // Metallic Target (R8 - R: metallic, GBA: unused)
  TextureCreateInfo metallicInfo{
    .type = TextureType::Texture2D,
    .format = Format::R8G8B8A8_UNORM,  // Using RGBA8 for simplicity, could optimize to R8
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled
  };
  m_metallicTarget = graphicsAPI.createTexture(metallicInfo);

  TextureViewCreateInfo metallicViewInfo{
    .format = Format::R8G8B8A8_UNORM,
    .viewType = TextureViewType::View2D
  };
  m_metallicView = m_metallicTarget->createView(metallicViewInfo);

  // Roughness Target (R8 - R: roughness, GBA: unused)
  TextureCreateInfo roughnessInfo{
    .type = TextureType::Texture2D,
    .format = Format::R8G8B8A8_UNORM,  // Using RGBA8 for simplicity, could optimize to R8
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled
  };
  m_roughnessTarget = graphicsAPI.createTexture(roughnessInfo);

  TextureViewCreateInfo roughnessViewInfo{
    .format = Format::R8G8B8A8_UNORM,
    .viewType = TextureViewType::View2D
  };
  m_roughnessView = m_roughnessTarget->createView(roughnessViewInfo);

  // Motion Vector Target (RG16F - RG: motion vectors, BA: unused)
  TextureCreateInfo motionInfo{
    .type = TextureType::Texture2D,
    .format = Format::R16G16B16A16_SFLOAT,
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::ColorAttachment | TextureUsage::Sampled
  };
  m_motionTarget = graphicsAPI.createTexture(motionInfo);

  TextureViewCreateInfo motionViewInfo{
    .format = Format::R16G16B16A16_SFLOAT,
    .viewType = TextureViewType::View2D
  };
  m_motionView = m_motionTarget->createView(motionViewInfo);

  // Depth Target
  TextureCreateInfo depthInfo{
    .type = TextureType::Texture2D,
    .format = Format::D32_SFLOAT,
    .width = m_width,
    .height = m_height,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::DepthStencil
  };
  m_depthTarget = graphicsAPI.createTexture(depthInfo);

  TextureViewCreateInfo depthViewInfo{
    .format = Format::D32_SFLOAT,
    .viewType = TextureViewType::View2D,
    .bIsDepthStencil = true
  };
  m_depthView = m_depthTarget->createView(depthViewInfo);

  CH_LOG_INFO(GBufferStageLog, "G-Buffer targets created: {0}x{1}", m_width, m_height);
}

/*
 */
void
GBufferStage::createGBufferRenderPass() {
 auto& graphicsAPI = IGraphicsAPI::instance();

  // Define attachments for G-Buffer (5 color + 1 depth)
  Vector<AttachmentDescription> attachments{
    // Attachment 0: Albedo
    {
      .format = Format::R8G8B8A8_UNORM,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::ShaderReadOnly
    },
    // Attachment 1: Normal
    {
      .format = Format::R16G16B16A16_SFLOAT,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::ShaderReadOnly
    },
    // Attachment 2: Metallic
    {
      .format = Format::R8G8B8A8_UNORM,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::ShaderReadOnly
    },
    // Attachment 3: Roughness
    {
      .format = Format::R8G8B8A8_UNORM,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::ShaderReadOnly
    },
    // Attachment 4: Motion Vectors
    {
      .format = Format::R16G16B16A16_SFLOAT,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::ShaderReadOnly
    },
    // Attachment 5: Depth
    {
      .format = Format::D32_SFLOAT,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::DontCare,
      .stencilLoadOp = LoadOp::DontCare,
      .stencilStoreOp = StoreOp::DontCare,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::DepthStencilAttachment
    }
  };

  // Color attachment references (5 color targets)
  Vector<AttachmentReference> colorRefs{
    {.attachment = 0, .layout = TextureLayout::ColorAttachment},  // Albedo
    {.attachment = 1, .layout = TextureLayout::ColorAttachment},  // Normal
    {.attachment = 2, .layout = TextureLayout::ColorAttachment},  // Metallic
    {.attachment = 3, .layout = TextureLayout::ColorAttachment},  // Roughness
    {.attachment = 4, .layout = TextureLayout::ColorAttachment}   // Motion
  };

  // Depth attachment reference
  AttachmentReference depthRef{
    .attachment = 5,
    .layout = TextureLayout::DepthStencilAttachment
  };

  // Subpass description
  SubpassDescription subpass{
    .pipelineBindPoint = PipelineBindPoint::Graphics,
    .colorAttachments = colorRefs,
    .depthStencilAttachment = depthRef
  };

  // Subpass dependency
  SubpassDependency dependency{
    .srcSubpass = SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = PipelineStage::ColorAttachmentOutput,
    .dstStageMask = PipelineStage::ColorAttachmentOutput,
    .srcAccessMask = Access::NoAccess,
    .dstAccessMask = Access::ColorAttachmentWrite
  };

  // Create render pass
  RenderPassCreateInfo renderPassInfo{
    .attachments = attachments,
    .subpasses = {subpass},
    .dependencies = {dependency}
  };

  m_renderPass = graphicsAPI.createRenderPass(renderPassInfo);

  // Create framebuffer
  FrameBufferCreateInfo fbInfo{
    .renderPass = m_renderPass,
    .attachments = {
      m_albedoView, m_normalView, m_metallicView,
      m_roughnessView, m_motionView, m_depthView
    },
    .width = m_width,
    .height = m_height,
    .layers = 1
  };
  m_framebuffer = graphicsAPI.createFrameBuffer(fbInfo);

  CH_LOG_INFO(GBufferStageLog, "G-Buffer render pass and framebuffer created");
}

/*
 */
void
GBufferStage::createGBufferPipeline() {
  // For now, we'll create a basic pipeline
  // In a full implementation, you'd load G-Buffer shaders here
  //auto& graphicsAPI = IGraphicsAPI::instance();

  // TODO: Load actual G-Buffer shaders
  // For now, we'll skip pipeline creation and just log
  // This will be implemented when we have proper G-Buffer shaders

  CH_LOG_INFO(GBufferStageLog, "G-Buffer pipeline creation skipped (no shaders yet)");

  // The pipeline will be created later when we add proper shaders
  // m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);
}

/*
 */
void
GBufferStage::createCommandResources() {
    auto& graphicsAPI = IGraphicsAPI::instance();

  // Create command pool for graphics operations
  m_commandPool = graphicsAPI.createCommandPool(QueueType::Graphics);
  if (!m_commandPool) {
    CH_LOG_ERROR(GBufferStageLog, "Failed to create command pool");
    return;
  }

  // Allocate command buffer
  m_commandBuffer = m_commandPool->allocateCommandBuffer();
  if (!m_commandBuffer) {
    CH_LOG_ERROR(GBufferStageLog, "Failed to allocate command buffer");
    return;
  }

  // Create fence for synchronization
  m_fence = graphicsAPI.createFence(true); // Start signaled
  if (!m_fence) {
    CH_LOG_ERROR(GBufferStageLog, "Failed to create fence");
    return;
  }

  CH_LOG_INFO(GBufferStageLog, "Command resources created successfully");
}

/*
 */
bool
GBufferStage::renderGeometry(SPtr<Camera> camera, SPtr<Model> model, float ) {
  if (!camera) {
    CH_LOG_ERROR(GBufferStageLog, "Invalid camera for rendering");
    return false;
  }

  if (!m_commandBuffer || !m_renderPass || !m_framebuffer) {
    CH_LOG_ERROR(GBufferStageLog, "Render resources not initialized");
    return false;
  }

  auto& graphicsAPI = IGraphicsAPI::instance();

  // Wait for previous frame
  if (!m_fence->wait(1000000000)) { // 1 second timeout
    CH_LOG_WARNING(GBufferStageLog, "Fence timeout");
    return false;
  }
  m_fence->reset();

  // Begin command buffer recording
  m_commandBuffer->begin();

  // Begin render pass
  RenderPassBeginInfo renderPassInfo{
    .renderPass = m_renderPass,
    .framebuffer = m_framebuffer,
    .clearValues = m_clearColors,
    .depthStencilClearValue = {{1.0f, 0}}
  };

  m_commandBuffer->beginRenderPass(renderPassInfo);
  m_commandBuffer->setViewport(0, 0, m_width, m_height);
  m_commandBuffer->setScissor(0, 0, m_width, m_height);

  // If we have a model, render it. Otherwise just clear the G-Buffer
  if (model) {
    // TODO: Bind G-Buffer pipeline and render model
    // For now, we'll just clear the render targets (same as before)
    CH_LOG_DEBUG(GBufferStageLog, "Rendering model to G-Buffer");

    // if (m_pipeline) {
    //   m_commandBuffer->bindPipeline(m_pipeline);
    //   // Render model here
    // }
  }
  else {
  }

  m_commandBuffer->endRenderPass();
  m_commandBuffer->end();

  // Submit command buffer
  SubmitInfo submitInfo{
    .commandBuffers = {m_commandBuffer},
    .waitSemaphores = {},
    .waitStages = {},
    .signalSemaphores = {}
  };

  graphicsAPI.getQueue(QueueType::Graphics)->submit(submitInfo, m_fence);
  return true;
}

} // namespace chEngineSDK
