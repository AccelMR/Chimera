/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2025/04/09
 */
/************************************************************************/

#include "chRenderer.h"

#include "chEventSystem.h"
#include "chEventDispatcherManager.h"
#include "chFileSystem.h"
#include "chICommandBuffer.h"
#include "chICommandPool.h"
#include "chICommandQueue.h"
#include "chIShader.h"
#include "chISynchronization.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"

namespace chEngineSDK {

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

  m_commandPool = graphicsAPI.createCommandPool(QueueType::Graphics);
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
    commandBuffer = m_commandPool->allocateCommandBuffer();
  }

  createSyncObjects();

  VertexPosColor triangleVerices[] = {
    { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // Red
    { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // Green
    { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }  // Blue
  };

  BufferCreateInfo bufferCreateInfo{
    .size = sizeof(triangleVerices),
    .usage = BufferUsage::VertexBuffer,
    .memoryUsage = MemoryUsage::CpuToGpu,
    .initialData = static_cast<void*>(&triangleVerices),
    .initialDataSize = sizeof(triangleVerices),
  };

  m_vertexBuffer = graphicsAPI.createBuffer(bufferCreateInfo);

  ShaderCreateInfo shaderCreateInfo{
    .stage = ShaderStage::Vertex,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead("resources/shaders/triangleVertex.spv"),
    .filePath = "resources/shaders/triangleVertex.spv",
    .defines = { /* Preprocessor defines */ }
  };

  ShaderCreateInfo fragmentShaderCreateInfo{
    .stage = ShaderStage::Fragment,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead("resources/shaders/triangleFragment.spv"),
    .filePath = "resources/shaders/triangleFragment.spv",
    .defines = { /* Preprocessor defines */ }
  };

  m_vertexShader  = graphicsAPI.createShader(shaderCreateInfo);
  m_fragmentShader= graphicsAPI.createShader(fragmentShaderCreateInfo);

  PipelineCreateInfo pipelineCreateInfo{
    .shaders = {
      { ShaderStage::Vertex, m_vertexShader },
      { ShaderStage::Fragment, m_fragmentShader }
    },
    .vertexLayout = VertexPosColor::getLayout(),
    .topology = PrimitiveTopology::TriangleList,
    .renderPass = m_swapChain->getRenderPass()
  };

  m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();
  HEvent listenResize = eventDispatcher.OnResize.connect(
    [&](uint32 width,uint32 height) {
      m_width = width;
      m_height = height;
      resizeSwapChain();
  });
}

/*
*/
void
Renderer::render(){
  auto& graphicsAPI = IGraphicsAPI::instance();
  
  m_inFlightFences[m_currentFrame]->wait(2);
  m_inFlightFences[m_currentFrame]->reset();
  
  if (!m_swapChain->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame])){
    resizeSwapChain();
    return;
  }
  uint32 imageIndex = m_swapChain->getCurrentImageIndex();
  if (imageIndex >= m_commandBuffers.size()) {
    return;
  }
  
  auto& cmdBuffer = m_commandBuffers[imageIndex];
  cmdBuffer->begin();
  
  RenderPassBeginInfo renderPassInfo{
    .renderPass = m_swapChain->getRenderPass(),
    .framebuffer = m_swapChain->getFramebuffer(imageIndex),
    .clearValues = { LinearColor::Purple }
  };
  
  cmdBuffer->beginRenderPass(renderPassInfo);
  
  cmdBuffer->setViewport(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  cmdBuffer->setScissor(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  
  cmdBuffer->bindPipeline(m_pipeline);
  cmdBuffer->bindVertexBuffer(m_vertexBuffer);
  
  cmdBuffer->draw(3);
  
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
  m_swapChain->resize(m_width, m_height);
    
  IGraphicsAPI::instance().getQueue(QueueType::Graphics)->waitIdle();

  m_commandBuffers.clear();
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
    commandBuffer = m_commandPool->allocateCommandBuffer();
  }
}
} // namespace chEngineSDK
