/************************************************************************/
/**
 * @file chWindowedApplication.cpp
 * @author AccelMR
 * @date 2025/07/05
 * Windowed application base class with DisplaySurface and graphics
 */
/************************************************************************/
#include "chWindowedApplication.h"

#include "chCommandParser.h"
#include "chDisplayEventHandle.h"
#include "chDisplayManager.h"
#include "chDynamicLibManager.h"
#include "chEventDispatcherManager.h"
#include "chFileSystem.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chStringUtils.h"

// Include graphics API
#include "chICommandBuffer.h"
#include "chICommandPool.h"
#include "chICommandQueue.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chISynchronization.h"
#include "chITexture.h"
#include "chITextureView.h"

CH_LOG_DECLARE_STATIC(WindowedApp, All);

namespace chEngineSDK {
using namespace std::chrono;

constexpr uint64 MAX_WAIT_TIME = 100000000; // 1 seconds in nanoseconds
constexpr uint64 MAX_WAIT_TIME_RESIZE = 1000000000;
constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;

/*
 */
WindowedApplication::WindowedApplication() {}

/*
 */
WindowedApplication::~WindowedApplication() {
  CH_LOG_INFO(WindowedApp, "Destroying WindowedApplication.");

  for (auto& fence : m_renderComponents.inFlightFences) {
    if (fence && !fence->wait(MAX_WAIT_TIME)) {
      CH_LOG_WARNING(WindowedApp, "Fence wait timed out or failed to reset.");
    }
  }

  m_renderComponents.commandBuffers.clear();
  m_renderComponents.imageAvailableSemaphores.clear();
  m_renderComponents.renderFinishedSemaphores.clear();
  m_renderComponents.inFlightFences.clear();

  CH_LOG_INFO(WindowedApp, "WindowedApplication destroyed successfully.");
}

/*
 */
void
WindowedApplication::run() {
  CH_LOG_INFO(WindowedApp, "Running WindowedApplication.");

  const double fixedTimeStamp = 1.0 / 60.0;
  double accumulator = 0.0;
  auto previousTime = high_resolution_clock::now();

  auto& eventDispatcher = EventDispatcherManager::instance();

  while (m_running) {
    // Calculate delta time
    auto currentTime = high_resolution_clock::now();
    auto deltaTime = duration_cast<duration<double>>(currentTime - previousTime).count();
    previousTime = currentTime;

    m_eventhandler->update();
    eventDispatcher.dispatchEvents(m_eventhandler);
    if (!m_running) {
      break;
    }

    // Update the accumulator
    accumulator += deltaTime;
    while (accumulator >= fixedTimeStamp) {
      // Update the application logic
      update(static_cast<float>(fixedTimeStamp));
      accumulator -= fixedTimeStamp;
    }

    // Render the application
    render(static_cast<float>(deltaTime));
  }
}

/*
*/
void
WindowedApplication::initialize() {
  CH_LOG_INFO(WindowedApp, "Initializing WindowedApplication.");
  initializeModules();

  CommandParser& commandParser = CommandParser::instance();

  initializeDisplay(
      {.name = commandParser.getParam("AppName", "Chimera Engine"),
       .title = commandParser.getParam("WindowTitle", "Chimera Engine Windowed Application"),
       .width = static_cast<uint32>(commandParser.getParamAsInt("Width", 1920)),
       .height = static_cast<uint32>(commandParser.getParamAsInt("Height", 1080))});
  initializeGraphics();
  initializeRenderComponents();
  bindEvents();

  CH_LOG_INFO(WindowedApp, "WindowedApplication post-initialization completed.");

  onPostInitialize();
}

/*
 */
void
WindowedApplication::initializeModules() {
  CH_LOG_INFO(WindowedApp, "WindowedApplication initializing modules.");
  BaseApplication::initializeModules();

  if (!CommandParser::isStarted()) {
    CH_LOG_WARNING(
        WindowedApp,
        "CommandParser was not started before WindowedApplication, starting it now.");
    CommandParser::startUp();
  }

  DynamicLibraryManager::startUp();
  DisplayManager::startUp();
  EventDispatcherManager::startUp();

  CH_LOG_INFO(WindowedApp, "WindowedApplication modules initialized.");
}

/*
 */
void
WindowedApplication::destroyModules() {
  BaseApplication::destroyModules();
  CH_LOG_INFO(WindowedApp, "WindowedApplication destroying modules.");
  EventDispatcherManager::shutDown();
  DisplayManager::shutDown();
  DynamicLibraryManager::shutDown();
  CommandParser::shutDown();
}

/*
*/
void
WindowedApplication::onPostDestoyModules() {
  CH_LOG_INFO(WindowedApp, "WindowedApplication pre-shutdown.");
  destroyRenderer();
  destroyGraphics();
  destroyDisplay();
  CH_LOG_INFO(WindowedApp, "WindowedApplication pre-shutdown completed.");
}

/*
*/
void
WindowedApplication::initializeDisplay(const ScreenDescriptor& desc) {
  m_eventhandler = chMakeShared<DisplayEventHandle>();
  WeakPtr<DisplaySurface> wptrDisplay =
      DisplayManager::instance().createDisplay(desc, m_eventhandler);
  if (wptrDisplay.expired()) {
    CH_EXCEPT(InternalErrorException, "Failed to create display.");
  }
  m_display = wptrDisplay.lock();
  CH_LOG_INFO(WindowedApp, "Display initialized successfully.");
}

/*
 */
void
WindowedApplication::initializeGraphics() {
  CH_LOG_INFO(WindowedApp, "Initializing graphics subsystem.");
  // Initialize graphics subsystem here
  const String& graphicsAPIName =
      CommandParser::instance().getParam("GraphicsAPI", "chVulkan");

#if USING(CH_DEBUG_MODE)
  const Path dllAbsolutePath =
      FileSystem::absolutePath(std::move(Path("build/debug-x64/lib")));
  CH_LOG_DEBUG(WindowedApp, "Loading graphics library: {0} from path: {1}", graphicsAPIName,
               dllAbsolutePath.toString());
#endif // CH_DEBUG_MODE

  WeakPtr<DynamicLibrary> graphicsLib =
      DynamicLibraryManager::instance().loadDynLibrary(graphicsAPIName
#if USING(CH_DEBUG_MODE)
                                                       ,
                                                       dllAbsolutePath
#endif // CH_DEBUG_MODE
      );
  if (graphicsLib.expired()) {
    CH_EXCEPT(InternalErrorException,
              chString::format("Failed to load graphics library: {0}", graphicsAPIName));
  }
  SPtr<DynamicLibrary> graphicsLibrary = graphicsLib.lock();

  // Load graphics api function pointer C style
  typedef void (*GraphicsAPIInitFunc)();
  auto initFunc =
      reinterpret_cast<GraphicsAPIInitFunc>(graphicsLibrary->getSymbol("loadPlugin"));

  if (!initFunc) {
    CH_EXCEPT(InternalErrorException,
              chString::format("Failed to get symbol 'loadPlugin' from "
                               "graphics library: {0}",
                               graphicsAPIName));
  }
  initFunc();

  // Initialize the graphics API
  IGraphicsAPI* graphicsAPI = IGraphicsAPI::instancePtr();
  if (!graphicsAPI) {
    CH_EXCEPT(InternalErrorException, "Graphics API instance is null after initialization.");
  }

  graphicsAPI->initialize({.weakDisplaySurface = m_display,
                           // TODO: probably want to change this or make the display return
                           // sidth of the drawable area
                           .width = m_display->getWidth(),
                           .height = m_display->getHeight(),
                           .enableValidationLayer = true});

  CH_LOG_INFO(WindowedApp, "Graphics subsystem initialized successfully.");
}

/*
 */
void
WindowedApplication::initializeRenderComponents() {
  CH_LOG_INFO(WindowedApp, "Initializing render components.");

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();

  // Create SwapChain
  m_renderComponents.swapChain =
      graphicsAPI.createSwapChain(m_display->getWidth(), m_display->getHeight(),
                                  false // vsync
      );

  if (!m_renderComponents.swapChain) {
    CH_EXCEPT(InternalErrorException, "Failed to create SwapChain.");
  }

  // Create command pool for graphics operations
  m_renderComponents.commandPool = graphicsAPI.createCommandPool(QueueType::Graphics, false);

  if (!m_renderComponents.commandPool) {
    CH_EXCEPT(InternalErrorException, "Failed to create command pool.");
  }

  // Get SwapChain image count
  uint32 imageCount = m_renderComponents.swapChain->getTextureCount();

  // Create command buffers (one per swap chain image)
  m_renderComponents.commandBuffers.resize(imageCount);
  for (uint32 i = 0; i < imageCount; ++i) {
    m_renderComponents.commandBuffers[i] =
        m_renderComponents.commandPool->allocateCommandBuffer();
    if (!m_renderComponents.commandBuffers[i]) {
      CH_EXCEPT(InternalErrorException,
                chString::format("Failed to create command buffer {0}.", i));
    }
  }

  createSyncObjects();

  // Get graphics queue
  m_renderComponents.graphicsQueue = graphicsAPI.getQueue(QueueType::Graphics);

  if (!m_renderComponents.graphicsQueue) {
    CH_EXCEPT(InternalErrorException, "Failed to get graphics queue.");
  }

  // Initialize frame tracking
  m_renderComponents.currentFrame = 0;

  CH_LOG_INFO(WindowedApp, "Render components initialized successfully.");
  CH_LOG_INFO(WindowedApp, "SwapChain images: {0}, Frames in flight: {1}", imageCount,
              MAX_FRAMES_IN_FLIGHT);
}

/*
 */
void
WindowedApplication::destroyGraphics() {
  CH_LOG_INFO(WindowedApp, "Destroying graphics subsystem.");

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  CH_LOG_INFO(WindowedApp, "Graphics subsystem destroyed successfully.");
}

/*
 */
void
WindowedApplication::destroyDisplay() {
  CH_LOG_INFO(WindowedApp, "Destroying display.");
  if (m_display) {
    m_display->close();
    m_display.reset();
    m_eventhandler.reset();
    CH_LOG_INFO(WindowedApp, "Display destroyed successfully.");
  } else {
    CH_LOG_WARNING(WindowedApp, "Display was not initialized, nothing to destroy.");
  }
}

/*
 */
void
WindowedApplication::destroyRenderer() {
  CH_LOG_INFO(WindowedApp, "Destroying renderer.");

  // Wait for all operations to complete
  if (IGraphicsAPI::instancePtr()) {
    IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
    graphicsAPI.waitIdle();

    // Wait for all fences before destroying them
    for (auto& fence : m_renderComponents.inFlightFences) {
      if (fence) {
        fence->wait(); // Wait indefinitely for completion
      }
    }
  }

  // Clean up render components in correct order
  m_renderComponents.commandBuffers.clear();
  m_renderComponents.commandPool.reset();

  // Clear all synchronization objects
  m_renderComponents.inFlightFences.clear();
  m_renderComponents.renderFinishedSemaphores.clear();
  m_renderComponents.imageAvailableSemaphores.clear();

  m_renderComponents.graphicsQueue.reset();
  m_renderComponents.swapChain.reset();

  CH_LOG_INFO(WindowedApp, "Renderer destroyed successfully.");
}

/*
 */
void
WindowedApplication::render(const float deltaTime) {
  // Basic render implementation with proper Vulkan synchronization
  if (!m_renderComponents.swapChain || m_renderComponents.inFlightFences.empty()) {
    return;
  }

  constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;
  uint32& currentFrame = m_renderComponents.currentFrame;

  // Wait for previous frame to finish
  auto& currentFence = m_renderComponents.inFlightFences[currentFrame];
  if (!currentFence->wait(MAX_WAIT_TIME)) {
    CH_LOG_WARNING(WindowedApp, "Frame {0} fence wait timed out.", currentFrame);
    return;
  }
  currentFence->reset();

  // Acquire next image from swap chain
  uint32 semaphoreIndex = currentFrame % m_renderComponents.imageAvailableSemaphores.size();
  auto imageAvailableSem = m_renderComponents.imageAvailableSemaphores[semaphoreIndex];

  bool acquired = m_renderComponents.swapChain->acquireNextImage(imageAvailableSem, nullptr);

  if (!acquired) {
    CH_LOG_WARNING(WindowedApp, "Failed to acquire swap chain image.");
    return;
  }

  uint32 imageIndex = m_renderComponents.swapChain->getCurrentImageIndex();

  // Validate image index
  if (imageIndex >= m_renderComponents.commandBuffers.size()) {
    CH_LOG_WARNING(WindowedApp, "Image index {0} out of range, skipping frame.", imageIndex);
    return;
  }

  auto& commandBuffer = m_renderComponents.commandBuffers[imageIndex];
  auto renderPass = m_renderComponents.swapChain->getRenderPass();
  auto framebuffer = m_renderComponents.swapChain->getFramebuffer(imageIndex);

  // Record command buffer
  commandBuffer->begin();

  // Begin render pass with clear color
  RenderPassBeginInfo renderPassBegin{.renderPass = renderPass,
                                      .framebuffer = framebuffer,
                                      .clearValues = {getBackgroundColor()}};

  commandBuffer->beginRenderPass(renderPassBegin);

  // Call derived class render implementation
  onRender(deltaTime, commandBuffer);

  // End render pass
  commandBuffer->endRenderPass();
  commandBuffer->end();

  auto renderFinishedSem = m_renderComponents.renderFinishedSemaphores[imageIndex];

  // Submit command buffer
  SubmitInfo submitInfo{.commandBuffers = {commandBuffer},
                        .waitSemaphores = {imageAvailableSem},
                        .waitStages = {PipelineStage::ColorAttachmentOutput},
                        .signalSemaphores = {renderFinishedSem}};

  m_renderComponents.graphicsQueue->submit({submitInfo}, currentFence);

  // Present with image-specific semaphore
  m_renderComponents.swapChain->present({renderFinishedSem});

  // Advance to next frame
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
 */
void
WindowedApplication::resize(uint32 width, uint32 height) {
  CH_LOG_INFO(WindowedApp, "Resizing display and swap chain.");

  auto& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  for (auto& fence : m_renderComponents.inFlightFences) {
    if (fence && !fence->wait(MAX_WAIT_TIME_RESIZE)) {
      CH_LOG_WARNING(WindowedApp, "Fence wait timed out during resize.");
    }
    fence->reset();
  }

  m_renderComponents.imageAvailableSemaphores.clear();
  m_renderComponents.renderFinishedSemaphores.clear();

  m_renderComponents.swapChain->resize(width, height);

  const uint32 swapChainWidth = m_renderComponents.swapChain->getWidth();
  const uint32 swapChainHeight = m_renderComponents.swapChain->getHeight();

  TextureCreateInfo depthTextureInfo{.type = TextureType::Texture2D,
                                     .format = Format::D32_SFLOAT,
                                     .width = swapChainWidth,
                                     .height = swapChainHeight,
                                     .depth = 1,
                                     .mipLevels = 1,
                                     .arrayLayers = 1,
                                     .samples = SampleCount::Count1,
                                     .usage = TextureUsage::DepthStencil};

  auto newDepthTexture = graphicsAPI.createTexture(depthTextureInfo);
  TextureViewCreateInfo depthTextureViewInfo{.format = Format::D32_SFLOAT,
                                             .viewType = TextureViewType::View2D,
                                             .bIsDepthStencil = true};

  CH_ASSERT(newDepthTexture);
  m_renderComponents.depthTextureView = newDepthTexture->createView(depthTextureViewInfo);
  CH_ASSERT(m_renderComponents.depthTextureView);

  createSyncObjects();

  m_renderComponents.commandBuffers.clear();
  m_renderComponents.commandBuffers.resize(m_renderComponents.swapChain->getTextureCount());
  for (auto& commandBuffer : m_renderComponents.commandBuffers) {
    commandBuffer = m_renderComponents.commandPool->allocateCommandBuffer();
  }

  CH_LOG_INFO(WindowedApp, "Display and swap chain resized successfully.");
}

/*
*/
void
WindowedApplication::createSyncObjects() {
  CH_LOG_INFO(WindowedApp, "Creating synchronization objects for rendering.");
  const uint32 imageCount = m_renderComponents.swapChain->getTextureCount();

  m_renderComponents.imageAvailableSemaphores.resize(imageCount);
  m_renderComponents.renderFinishedSemaphores.resize(imageCount);
  m_renderComponents.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  auto& graphicsAPI = IGraphicsAPI::instance();
  for (uint32 i = 0; i < imageCount; ++i) {
    m_renderComponents.imageAvailableSemaphores[i] = graphicsAPI.createSemaphore();
    m_renderComponents.renderFinishedSemaphores[i] = graphicsAPI.createSemaphore();
  }

  for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    m_renderComponents.inFlightFences[i] = graphicsAPI.createFence(true);
  }
}

/*
*/
void
WindowedApplication::bindEvents() {
  CH_LOG_INFO(WindowedApp, "Binding events for WindowedApplication.");

  auto& eventDispatcher = EventDispatcherManager::instance();

  HEvent listenCloseEvent = eventDispatcher.OnClose.connect([&]() { m_running = false; });
  HEvent listenResize = eventDispatcher.OnResize.connect(
      [&](uint32 width, uint32 height) { resize(width, height); });
}

} // namespace chEngineSDK
