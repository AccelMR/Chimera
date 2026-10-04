/************************************************************************/
/**
 * @file chWindowedApplication.cpp
 * @author AccelMR
 * @date 2025/07/05
 * Windowed application base class with DisplaySurface and graphics
 */
/************************************************************************/
#include "chWindowedApplication.h"

#include <chrono>
#include <thread>

#include "chCommandLine.h"
#include "chDisplayEventHandle.h"
#include "chDisplayManager.h"
#include "chDynamicLibManager.h"
#include "chEnginePaths.h"
#include "chEventDispatcherManager.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chStringUtils.h"

// Include graphics API
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"

CH_LOG_DECLARE_STATIC(WindowedApp, All);

namespace chEngineSDK {
using namespace std::chrono;

/*
 */
WindowedApplication::WindowedApplication() {}

/*
 */
WindowedApplication::~WindowedApplication() {}

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

  initializeDisplay(
      {.name = CommandLine::getValue("AppName", "Chimera Engine"),
       .title = CommandLine::getValue("WindowTitle", "Chimera Engine Windowed Application"),
       .width = static_cast<uint32>(CommandLine::getInt("Width", 2560)),
       .height = static_cast<uint32>(CommandLine::getInt("Height", 1440))});
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

  DynamicLibraryManager::startUp();
  DisplayManager::startUp();
  EventDispatcherManager::startUp();

  CH_LOG_INFO(WindowedApp, "WindowedApplication modules initialized.");
}

/*
 */
void
WindowedApplication::destroyModules()
{
  CH_LOG_INFO(WindowedApp, "WindowedApplication destroying modules.");
  m_closeEvent.disconnect();
  m_resizeEvent.disconnect();

  destroyRenderer();

  // Every GPU object keeps a copy of the device, so all of them must be gone by now.
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::shutDown();
  }

  // The window goes after the graphics API, which destroys the surface made on it.
  destroyDisplay();

  if (EventDispatcherManager::isStarted()) {
    EventDispatcherManager::shutDown();
  }
  if (DisplayManager::isStarted()) {
    DisplayManager::shutDown();
  }
  // Last, because the graphics API and the codecs run code from the libraries it loaded.
  if (DynamicLibraryManager::isStarted()) {
    DynamicLibraryManager::shutDown();
  }

  BaseApplication::destroyModules();
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
  // Chimera.exe -GraphicsAPI=chVulkan -scene=MyScene
  const String graphicsAPIName = CommandLine::getValue("GraphicsAPI", "chVulkan");

  const Path& pluginDirectory = EnginePaths::getPluginDirectory();
  CH_LOG_DEBUG(WindowedApp, "Loading graphics library: {0} from path: {1}", graphicsAPIName,
               pluginDirectory);

  WeakPtr<DynamicLibrary> graphicsLib =
      DynamicLibraryManager::instance().loadDynLibrary(graphicsAPIName, pluginDirectory);
  if (graphicsLib.expired()) {
    CH_EXCEPT(InternalErrorException,
              StringUtils::format("Failed to load graphics library: {0}", graphicsAPIName));
  }
  using GraphicsAPIInitFunc = void (*)();
  const auto initFunc = graphicsLib.lock()->getSymbol<GraphicsAPIInitFunc>("loadPlugin");

  if (!initFunc) {
    CH_EXCEPT(InternalErrorException,
              StringUtils::format("Failed to get symbol 'loadPlugin' from "
                               "graphics library: {0}",
                               graphicsAPIName));
  }
  initFunc();

  // Initialize the graphics API
  IGraphicsAPI* graphicsAPI = IGraphicsAPI::instancePtr();
  if (!graphicsAPI) {
    CH_EXCEPT(InternalErrorException, "Graphics API instance is null after initialization.");
  }

  CH_ASSERT(m_display && "Display must be initialized before graphics.");
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
WindowedApplication::initializeRenderComponents()
{
  CH_LOG_INFO(WindowedApp, "Initializing render components.");

  m_swapChain = IGraphicsAPI::instance().createSwapChain(m_display->getWidth(),
                                                         m_display->getHeight(), false);
  if (!m_swapChain) {
    CH_EXCEPT(InternalErrorException, "Failed to create SwapChain.");
  }

  CH_LOG_INFO(WindowedApp, "SwapChain images: {0}, Frames in flight: {1}",
              m_swapChain->getTextureCount(), GraphicsLimits::MAX_FRAMES_IN_FLIGHT);
}

/*
 */
void
WindowedApplication::destroyDisplay()
{
  m_eventhandler.reset();
  if (!m_display) {
    return;
  }

  CH_LOG_INFO(WindowedApp, "Destroying display.");
  m_display->close();
  m_display.reset();
}

/*
 */
void
WindowedApplication::destroyRenderer()
{
  CH_LOG_INFO(WindowedApp, "Destroying renderer.");

  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }
  m_swapChain.reset();
}

/*
 */
void
WindowedApplication::render(const float deltaTime)
{
  // The surface of a minimized window is 0x0, so there is nothing to draw on and the swap
  // chain cannot be recreated. Sleeping keeps the loop from spinning until it is restored.
  if (m_display->isMinimized()) {
    std::this_thread::sleep_for(milliseconds(10));
    return;
  }

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  ISwapChain& swapChain = *m_swapChain;

  ICommandList& commandList = graphicsAPI.beginFrame();

  const SwapChainStatus acquireStatus = swapChain.acquireNextImage();
  if (acquireStatus == SwapChainStatus::OutOfDate ||
      acquireStatus == SwapChainStatus::Failed) {
    graphicsAPI.endFrame();
    if (acquireStatus == SwapChainStatus::OutOfDate) {
      resize(m_display->getWidth(), m_display->getHeight());
    }
    return;
  }

  const RendererOutput sceneOutput = onRender(commandList, deltaTime);

  const uint32 width = swapChain.getWidth();
  const uint32 height = swapChain.getHeight();
  const ITexture& backBuffer = swapChain.getCurrentTexture();

  // The whole image is cleared, so its previous contents are not needed.
  const Array<TextureBarrier, 1> toRendering = {
      TextureBarrier{.texture = &backBuffer,
                     .before = ResourceState::Undefined,
                     .after = ResourceState::RenderTarget}};
  commandList.barrier(toRendering);

  RenderingDesc renderingDesc{.colorAttachmentCount = 1, .width = width, .height = height};
  renderingDesc.colorAttachments[0] = {.view = &swapChain.getCurrentTextureView(),
                                       .clearColor = getBackgroundColor()};
  commandList.beginRendering(renderingDesc);

  onPresent(sceneOutput, commandList, width, height);

  commandList.endRendering();

  const Array<TextureBarrier, 1> toPresent = {
      TextureBarrier{.texture = &backBuffer,
                     .before = ResourceState::RenderTarget,
                     .after = ResourceState::Present}};
  commandList.barrier(toPresent);

  graphicsAPI.endFrame();
  const SwapChainStatus presentStatus = swapChain.present();

  onPostPresent();

  // A suboptimal image is still drawn and presented, so the swap chain is only recreated
  // after the frame is done.
  if (acquireStatus == SwapChainStatus::Suboptimal ||
      presentStatus == SwapChainStatus::Suboptimal ||
      presentStatus == SwapChainStatus::OutOfDate) {
    resize(m_display->getWidth(), m_display->getHeight());
  }
}

/*
 */
void
WindowedApplication::resize(uint32 width, uint32 height)
{
  if (m_display->isMinimized()) {
    return;
  }

  m_swapChain->resize(width, height);

  CH_LOG_INFO(WindowedApp, "Swap chain resized to {0}x{1}.", m_swapChain->getWidth(),
              m_swapChain->getHeight());
}

/*
*/
void
WindowedApplication::bindEvents() {
  CH_LOG_INFO(WindowedApp, "Binding events for WindowedApplication.");

  auto& eventDispatcher = EventDispatcherManager::instance();

  m_closeEvent = eventDispatcher.OnClose.connect([&]() { m_running = false; });
  m_resizeEvent = eventDispatcher.OnResize.connect(
      [&](uint32 width, uint32 height) { resize(width, height); });
}

} // namespace chEngineSDK
