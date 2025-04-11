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
void
Renderer::initialize(WeakPtr<DisplaySurface> displaySurface,
                       uint32 width, 
                       uint32 height, 
                       bool vsync) {

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();
  CH_ASSERT(displaySurface.expired());
  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);

  m_swapChain = IGraphicsAPI::instance().createSwapChain(width, height, vsync);
  
  HEvent listenResize = eventDispatcher.OnResize.connect(
    [&](uint32 width,uint32 height) {
      m_swapChain->resize(width, height);
  });

  initializeRenderResources();
}

/*
*/
void
Renderer::render(){
  auto& graphicsAPI = IGraphicsAPI::instance();
    
  // Esperar a que el frame anterior termine
  m_inFlightFences[m_currentFrame]->wait();
  m_inFlightFences[m_currentFrame]->reset();
  
  // Adquirir la siguiente imagen del swapchain
  m_swapChain->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame]);
  
  uint32 imageIndex = m_swapChain->getCurrentImageIndex();
  
  // Reestablecer y grabar el command buffer
  auto& cmdBuffer = m_commandBuffers[imageIndex];
  cmdBuffer->begin();
  
  // Comenzar render pass
  RenderPassBeginInfo renderPassInfo{
      .renderPass = m_swapChain->getRenderPass(),
      .framebuffer = m_swapChain->getFramebuffer(imageIndex),
      .clearValues = { {0.0f, 0.0f, 0.0f, 1.0f} } // Negro transparente
  };
  
  cmdBuffer->beginRenderPass(renderPassInfo);
  
  // Configurar viewport y scissor
  cmdBuffer->setViewport(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  cmdBuffer->setScissor(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  
  // Vincular pipeline y recursos
  cmdBuffer->bindPipeline(m_pipeline);
  cmdBuffer->bindVertexBuffer(m_vertexBuffer);
  
  // Dibujar triángulo
  cmdBuffer->draw(3);
  
  cmdBuffer->endRenderPass();
  cmdBuffer->end();
  
  // Enviar el command buffer
  Vector<SPtr<ICommandBuffer>> submitBuffers = { cmdBuffer };
  
  SubmitInfo submitInfo{
      .waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] },
      .signalSemaphores = { m_renderFinishedSemaphores[m_currentFrame] },
      .commandBuffers = submitBuffers
  };
  
  graphicsAPI.getQueue(QueueType::Graphics)->submit(submitBuffers, m_inFlightFences[m_currentFrame]);
  
  // Presentar
  m_swapChain->present({ m_renderFinishedSemaphores[m_currentFrame] });
  
  // Avanzar al siguiente frame
  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

  auto vertexShader = graphicsAPI.createShader(shaderCreateInfo);
  auto fragmentShader = graphicsAPI.createShader(fragmentShaderCreateInfo);

  PipelineCreateInfo pipelineCreateInfo{
    .shaders = {
      { ShaderStage::Vertex, vertexShader },
      { ShaderStage::Fragment, fragmentShader }
    },
    .vertexLayout = VertexPosColor::getLayout(),
    .topology = PrimitiveTopology::TriangleList,
    .renderPass = m_swapChain->getRenderPass()
  };

  m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);
}
} // namespace chEngineSDK
