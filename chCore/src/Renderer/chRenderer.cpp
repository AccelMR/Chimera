/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2025/04/09
 */
/************************************************************************/

#include "chRenderer.h"

#include "chBox.h"
#include "chDegree.h"
#include "chEventDispatcherManager.h"
#include "chEventSystem.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMatrix4.h"
#include "chRadian.h"
#include "chVector3.h"

// Graphics-related includes
#include "chIBuffer.h"
#include "chICommandBuffer.h"
#include "chICommandPool.h"
#include "chICommandQueue.h"
#include "chIDescriptorPool.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chISwapChain.h"
#include "chISynchronization.h"
#include "chITexture.h"
#include "chITextureView.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_RENDERER_LOG_LEVEL All
#else
#define CH_RENDERER_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)

CH_LOG_DECLARE_STATIC(RendererSystem, CH_RENDERER_LOG_LEVEL);

namespace RendererHelpers{
struct ProjectionViewMatrix{
  Matrix4 projectionMatrix;
  Matrix4 viewMatrix;
};

Vector<uint8> loadImage(const Path& path, 
                        int32* width, 
                        int32* height, 
                        int32* channels) {
  CH_ASSERT(FileSystem::isFile(path));

  Vector<uint8> imageData;
  uint8* data = stbi_load(path.toString().c_str(), width, height, channels, STBI_rgb_alpha);
  if (data) {
    imageData.resize((*width) * (*height) * (static_cast<SIZE_T>(STBI_rgb_alpha)));
    memcpy(imageData.data(), data, imageData.size());
    stbi_image_free(data);
  }
  return imageData;
}
} // namespace RendererHelpers

RendererHelpers::ProjectionViewMatrix projectionViewMatrix;
Vector3 cameraPosition(-5.0f, 0.0f, 0.0f);
Vector3 lookAtPos = Vector3::ZERO;
float g_farPlane = 10000.0f;
float g_nearPlane = 0.1f;
Radian g_halfFOV(Degree(45.0f) * .5f);
float g_cameraSpeed = 0.01f;
float g_cameraRotationSpeed = 0.01f;

/*
*/
Renderer::~Renderer() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  graphicsAPI.getQueue(QueueType::Graphics)->waitIdle();

  graphicsAPI.waitIdle();

  for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (m_inFlightFences[i]){
      m_inFlightFences[i]->wait();
    }
  }

  m_commandBuffers.clear();
  m_commandPool->reset();

  m_imageAvailableSemaphores.clear();
  m_renderFinishedSemaphores.clear();
  m_inFlightFences.clear();

  m_vertexBuffer.reset();
  m_vertexShader.reset();
  m_fragmentShader.reset();
  m_pipeline.reset();

  m_swapChain.reset();
}

/*
*/
void
Renderer::initialize(uint32 width, 
                     uint32 height, 
                     bool vsync) {
  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);

  m_swapChain = IGraphicsAPI::instance().createSwapChain(width, height, vsync);

  initializeRenderResources();
}

/*
*/
void
Renderer::createSyncObjects() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    m_imageAvailableSemaphores[i] = graphicsAPI.createSemaphore();
    m_renderFinishedSemaphores[i] = graphicsAPI.createSemaphore();
    m_inFlightFences[i] = graphicsAPI.createFence(true);
  }
}

/*
*/
void
Renderer::initializeRenderResources() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  m_width = m_swapChain->getWidth();
  m_height = m_swapChain->getHeight();

  m_commandPool = graphicsAPI.createCommandPool(QueueType::Graphics);
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
    commandBuffer = m_commandPool->allocateCommandBuffer();
  }

  createSyncObjects();

  int32 imageWidth = 0;
  int32 imageHeight = 0;
  int32 channel = 0;
  Vector<uint8> imageData = RendererHelpers::loadImage("resources/images/beto1.jpg", 
                                                       &imageWidth,
                                                       &imageHeight,
                                                       &channel);

  TextureCreateInfo textureCreateInfo{
    .type = TextureType::Texture2D,
    .format = Format::R8G8B8A8_UNORM,
    .width = static_cast<uint32>(imageWidth),
    .height = static_cast<uint32>(imageHeight),
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::Sampled | TextureUsage::TransferDst,
    .initialData = imageData.data(),
    .initialDataSize = imageData.size()
  };
  m_texture = graphicsAPI.createTexture(textureCreateInfo);

  TextureViewCreateInfo textureViewCreateInfo{
    .format = Format::R8G8B8A8_UNORM,
    .viewType = TextureViewType::View2D,
    .baseMipLevel = 0,
    .mipLevelCount = 1,
    .baseArrayLayer = 0,
    .arrayLayerCount = 1
  };
  m_textureView = m_texture->createView(textureViewCreateInfo);

  AABox box(-Vector3::UNIT, Vector3::UNIT);
  Array<Vector3, 8> boxVertices = box.generateVertices3();

  Vector3 unitX(1.0f, 0.0f, 0.0f);
  Vector3 unitY(0.0f, 1.0f, 0.0f);
  Vector3 unitZ(0.0f, 0.0f, 1.0f);
  Vector3 unitW(0.0f, 0.0f, 0.0f);

  VertexNormalTexCoord cubeVertices[] = {
    { boxVertices[0], unitX, Vector2(0.0f, 0.0f) },
    { boxVertices[1], unitY, Vector2(1.0f, 0.0f) },
    { boxVertices[2], unitZ, Vector2(1.0f, 1.0f) },
    { boxVertices[3], unitW, Vector2(0.0f, 1.0f) },
    { boxVertices[4], unitX, Vector2(0.0f, 1.0f) },
    { boxVertices[5], unitY, Vector2(1.0f, 1.0f) },
    { boxVertices[6], unitZ, Vector2(1.5f, 1.5f) },
    { boxVertices[7], unitW, Vector2(0.5f, 1.5f) }
  };

  BufferCreateInfo bufferCreateInfo{
    .size = sizeof(cubeVertices),
    .usage = BufferUsage::VertexBuffer,
    .memoryUsage = MemoryUsage::CpuToGpu,
    .initialData = static_cast<void*>(&cubeVertices),
    .initialDataSize = sizeof(cubeVertices),
  };
  m_vertexBuffer = graphicsAPI.createBuffer(bufferCreateInfo);

  Array<uint16, 36> boxIndices = box.getConstIndices();
  BufferCreateInfo indexBufferCreateInfo{
    .size = sizeof(boxIndices),
    .usage = BufferUsage::IndexBuffer,
    .memoryUsage = MemoryUsage::CpuToGpu,
    .initialData = static_cast<void*>(&boxIndices),
    .initialDataSize = sizeof(boxIndices),
  };
  m_indexBuffer = graphicsAPI.createBuffer(indexBufferCreateInfo);

  
  projectionViewMatrix = {
    .projectionMatrix = PerspectiveMatrix(g_halfFOV, 
                                          static_cast<float>(m_width), 
                                          static_cast<float>(m_height), 
                                          g_nearPlane, g_farPlane),
    .viewMatrix = LookAtMatrix(cameraPosition, 
                               lookAtPos,
                               Vector3::UP)
  };

  BufferCreateInfo projectionViewBufferCreateInfo{
    .size = sizeof(RendererHelpers::ProjectionViewMatrix),
    .usage = BufferUsage::UniformBuffer,
    .memoryUsage = MemoryUsage::CpuToGpu,
    .initialData = static_cast<void*>(&projectionViewMatrix),
    .initialDataSize = sizeof(RendererHelpers::ProjectionViewMatrix),
  };
  m_viewProjectionBuffer = graphicsAPI.createBuffer(projectionViewBufferCreateInfo);

  ShaderCreateInfo shaderCreateInfo{
    .stage = ShaderStage::Vertex,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead("resources/shaders/cubeVertex.spv"),
    .filePath = "resources/shaders/cubeVertex.spv",
    .defines = { /* Preprocessor defines */ }
  };

  ShaderCreateInfo fragmentShaderCreateInfo{
    .stage = ShaderStage::Fragment,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead("resources/shaders/cubeFragment.spv"),
    .filePath = "resources/shaders/cubeFragment.spv",
    .defines = { /* Preprocessor defines */ }
  };

  m_vertexShader  = graphicsAPI.createShader(shaderCreateInfo);
  m_fragmentShader= graphicsAPI.createShader(fragmentShaderCreateInfo);

  Vector<DescriptorSetLayoutBinding> bindings{
    {
      .binding = 0,
      .type = DescriptorType::UniformBuffer,
      .count = 1,
      .stageFlags = ShaderStage::Vertex
    },
    {
      .binding = 1,
      .type = DescriptorType::CombinedImageSampler,
      .count = 1,
      .stageFlags = ShaderStage::Fragment
    }
    
  };

  DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
    .bindings = bindings
  };
  m_descriptorSetLayout = graphicsAPI.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

  DescriptorPoolCreateInfo descriptorPoolCreateInfo{
    .maxSets = 1,
    .poolSizes = {
      { DescriptorType::UniformBuffer, 1 },
      { DescriptorType::CombinedImageSampler, 1 }
    }
  };
  m_descriptorPool = graphicsAPI.createDescriptorPool(descriptorPoolCreateInfo);

  SamplerCreateInfo samplerCreateInfo {
    .magFilter = SamplerFilter::Linear,
    .minFilter = SamplerFilter::Linear,
    .mipmapMode = SamplerMipmapMode::Linear,
    .addressModeU = SamplerAddressMode::Repeat,
    .addressModeV = SamplerAddressMode::Repeat,
    .addressModeW = SamplerAddressMode::Repeat,
    .anisotropyEnable = false,
    .maxAnisotropy = 16.0f
  };
  m_sampler = graphicsAPI.createSampler(samplerCreateInfo);

  DescriptorSetAllocateInfo descriptorSetAllocateInfo{
    .pool = m_descriptorPool,
    .layout = m_descriptorSetLayout 
  };
  m_descriptorSet = m_descriptorPool->allocateDescriptorSet(descriptorSetAllocateInfo);

  DescriptorBufferInfo descriptorBufferInfo{
    .buffer = m_viewProjectionBuffer,
    .offset = 0,
    .range = sizeof(RendererHelpers::ProjectionViewMatrix)
  };

  DescriptorImageInfo descriptorImageInfo{
    .sampler = m_sampler,
    .imageView = m_textureView,
    .imageLayout = TextureLayout::ShaderReadOnly
  };

  Vector<WriteDescriptorSet> writeDescriptorSets{
    {
      .dstSet = m_descriptorSet,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorType = DescriptorType::UniformBuffer,
      .bufferInfos = { descriptorBufferInfo }
    },
    {
      .dstSet = m_descriptorSet,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorType = DescriptorType::CombinedImageSampler,
      .imageInfos = { descriptorImageInfo }
    }
  };
  graphicsAPI.updateDescriptorSets(writeDescriptorSets);

  PipelineCreateInfo pipelineCreateInfo{
    .shaders = {
      { ShaderStage::Vertex, m_vertexShader },
      { ShaderStage::Fragment, m_fragmentShader }
    },
    .vertexLayout = VertexNormalTexCoord::getLayout(),
    .topology = PrimitiveTopology::TriangleList,
    .renderPass = m_swapChain->getRenderPass(),
    .setLayouts = { m_descriptorSetLayout },
  };
  m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();
  HEvent listenResize = eventDispatcher.OnResize.connect(
    [&](uint32 width,uint32 height) {
      m_width = width;
      m_height = height;
      resizeSwapChain();

      projectionViewMatrix.projectionMatrix = PerspectiveMatrix(g_halfFOV, 
                                                                static_cast<float>(m_width), 
                                                                static_cast<float>(m_height), 
                                                                g_nearPlane, g_farPlane);
  });

  HEvent listenKeyDown = eventDispatcher.OnKeyDown.connect([&](const KeyBoardData& keydata) {
    if (keydata.key == Key::P) {
      // Print camera position and lookAtPos
      CH_LOG_INFO(RendererSystem, "Camera Position: ({0}, {1}, {2})", cameraPosition.x, cameraPosition.y, cameraPosition.z);
      CH_LOG_INFO(RendererSystem, "LookAt Position: ({0}, {1}, {2})", lookAtPos.x, lookAtPos.y, lookAtPos.z);
    }
  });

  HEvent listenKeys = eventDispatcher.OnKeyPressed.connect([&](const KeyBoardData& keydata) {
    Vector3 cameraDir = (lookAtPos - cameraPosition).getNormalized(); // Adjusted to align with forward direction
    Vector3 right = Vector3::UP.cross(cameraDir).getNormalized(); // Corrected to align with right direction

    switch (keydata.key) {
      case Key::W: cameraPosition += cameraDir * g_cameraSpeed; lookAtPos += cameraDir * g_cameraSpeed; break; // Move forward
      case Key::S: cameraPosition -= cameraDir * g_cameraSpeed; lookAtPos -= cameraDir * g_cameraSpeed; break; // Move backward
      case Key::A: cameraPosition -= right * g_cameraSpeed; lookAtPos -= right * g_cameraSpeed; break; // Move left
      case Key::D: cameraPosition += right * g_cameraSpeed; lookAtPos += right * g_cameraSpeed; break; // Move right
      case Key::Q: cameraPosition += Vector3::UP * g_cameraSpeed; lookAtPos += Vector3::UP * g_cameraSpeed; break; // Move up
      case Key::E: cameraPosition -= Vector3::UP * g_cameraSpeed; lookAtPos -= Vector3::UP * g_cameraSpeed; break; // Move down
      case Key::R: cameraPosition = Vector3(-5.0f, 0.0f, 0.0f); lookAtPos = Vector3::ZERO; break;
      default: return;
    }

    lookAtPos = cameraPosition + cameraDir * 5.0f;
    projectionViewMatrix.viewMatrix = LookAtMatrix(cameraPosition, lookAtPos, Vector3::UP);
  });

  HEvent listenMouse = eventDispatcher.OnMouseMove.connect([&](const MouseMoveData& mouseData) {
    if (!eventDispatcher.isMouseButtonDown(MouseButton::Right)) {
      return;
    }

    if (mouseData.deltaX != 0 || mouseData.deltaY != 0) {
      //CH_LOG_INFO(RendererSystem, "Mouse moved: ({0}, {1})", mouseData.deltaX, mouseData.deltaY);
      RotationMatrix rotationMatrix(Rotator(
        Degree(-mouseData.deltaX * g_cameraRotationSpeed),  // Yaw (rotation around Z axis)
        Degree(0.0f),                     // Pitch (rotation around X axis - not used)
        Degree(mouseData.deltaY * g_cameraRotationSpeed)   // Roll (rotation around Y axis)
    ));
      projectionViewMatrix.viewMatrix *= rotationMatrix;
    }
  });
}

/*
*/
static constexpr uint64 MAX_WAIT_TIME = 1000000000; // 1 second
void
Renderer::render(const float deltaTime) {
  auto& graphicsAPI = IGraphicsAPI::instance();
  
  m_inFlightFences[m_currentFrame]->wait(MAX_WAIT_TIME);
  m_inFlightFences[m_currentFrame]->reset();
  
  if (!m_swapChain->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame])){
    resizeSwapChain();
    return;
  }
  uint32 imageIndex = m_swapChain->getCurrentImageIndex();

  //TODO: might need to fix this
  if (imageIndex >= m_commandBuffers.size()) {
    CH_LOG_ERROR(RendererSystem, "Image index out of range, swapchain may be resized. Deferring to next frame.");
    return;
  }
  
  auto& cmdBuffer = m_commandBuffers[imageIndex];
  cmdBuffer->begin();
  
  RenderPassBeginInfo renderPassInfo{
    .renderPass = m_swapChain->getRenderPass(),
    .framebuffer = m_swapChain->getFramebuffer(imageIndex),
    .clearValues = { LinearColor::Black }
  };
  
  cmdBuffer->beginRenderPass(renderPassInfo);
  
  cmdBuffer->setViewport(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  cmdBuffer->setScissor(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  
  cmdBuffer->bindPipeline(m_pipeline);
  cmdBuffer->bindVertexBuffer(m_vertexBuffer);
  cmdBuffer->bindIndexBuffer(m_indexBuffer, IndexType::UInt16);

  m_viewProjectionBuffer->update(&projectionViewMatrix,
                                 sizeof(RendererHelpers::ProjectionViewMatrix));

  cmdBuffer->bindDescriptorSets(PipelineBindPoint::Graphics,
                                m_pipeline->getLayout(),
                                0,
                                { m_descriptorSet });
  
  cmdBuffer->drawIndexed(36);
  
  cmdBuffer->endRenderPass();
  cmdBuffer->end();
  
  Vector<SPtr<ICommandBuffer>> submitBuffers = { cmdBuffer };
  
  SubmitInfo submitInfo{
    .commandBuffers = { m_commandBuffers[imageIndex] },
    .waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] },
    .waitStages = { PipelineStage::ColorAttachmentOutput },
    .signalSemaphores = { m_renderFinishedSemaphores[m_currentFrame] },
  };

  graphicsAPI.getQueue(QueueType::Graphics)->submit(submitInfo, m_inFlightFences[m_currentFrame]);
  
  m_swapChain->present({ m_renderFinishedSemaphores[m_currentFrame] });
  
  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
*/
void
Renderer::resizeSwapChain() {
  IGraphicsAPI::instance().getQueue(QueueType::Graphics)->waitIdle();

  m_imageAvailableSemaphores.clear();
  m_renderFinishedSemaphores.clear();
  m_swapChain->resize(m_width, m_height);

  createSyncObjects();

  m_commandBuffers.clear();
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
    commandBuffer = m_commandPool->allocateCommandBuffer();
  }
}
} // namespace chEngineSDK
