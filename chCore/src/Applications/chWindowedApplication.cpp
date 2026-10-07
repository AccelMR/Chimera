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

#include "chConsoleVariable.h"
#include "chDisplayEventHandle.h"
#include "chDisplayManager.h"
#include "chDynamicLibManager.h"
#include "chEnginePaths.h"
#include "chEventDispatcherManager.h"
#include "chGraphicsSettings.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chMath.h"
#include "chStringUtils.h"

// Include graphics API
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"

CH_LOG_DECLARE_STATIC(WindowedApp, All);

namespace chEngineSDK {
using namespace std::chrono;

namespace {
ConsoleVariable<String> g_cvarApplicationName("Application.Name",
                                              "Chimera Engine",
                                              "Name the platform shows for the program.",
                                              "AppName");

ConsoleVariable<String> g_cvarWindowTitle("Window.Title",
                                          "Chimera Engine Windowed Application",
                                          "Title of the main window.",
                                          "WindowTitle");

ConsoleVariable<int32> g_cvarWindowWidth("Window.Width",
                                         2560,
                                         "Width of the main window when it opens.",
                                         "Width");

ConsoleVariable<int32> g_cvarWindowHeight("Window.Height",
                                          1440,
                                          "Height of the main window when it opens.",
                                          "Height");
} // namespace

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

  loadGraphicsAPI();
  initializeDisplay(
      {.name = g_cvarApplicationName.get(),
       .title = g_cvarWindowTitle.get(),
       .width = static_cast<uint32>(Math::max(1, g_cvarWindowWidth.get())),
       .height = static_cast<uint32>(Math::max(1, g_cvarWindowHeight.get())),
       .resizable = true,
       .platformFlags = IGraphicsAPI::instance().getPlatformWindowFlags()});
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

  // The window goes after every swap chain, so no surface made on it outlives it.
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
WindowedApplication::loadGraphicsAPI()
{
  const String& graphicsAPIName = g_cvarGraphicsAPI.get();

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

  if (!IGraphicsAPI::isStarted()) {
    CH_EXCEPT(InternalErrorException,
              StringUtils::format("{0} did not start a graphics API.", graphicsAPIName));
  }
}

/*
 */
void
WindowedApplication::initializeGraphics()
{
  CH_LOG_INFO(WindowedApp, "Initializing graphics subsystem.");
  IGraphicsAPI::instance().initialize(
      {.enableValidationLayer = g_cvarValidationLayer.get()});
  CH_LOG_INFO(WindowedApp, "Graphics subsystem initialized successfully.");
}

/*
 */
void
WindowedApplication::initializeRenderComponents()
{
  CH_LOG_INFO(WindowedApp, "Initializing render components.");

  CH_ASSERT(m_display && "Display must be initialized before the swap chain.");
  m_swapChain = IGraphicsAPI::instance().createSwapChain(
      {.window = m_display->getPlatformHandler(),
       .width = m_display->getWidth(),
       .height = m_display->getHeight(),
       .vsync = g_cvarVSync.get(),
       .debugName = "Main SwapChain"});
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

  onRender(commandList, swapChain, deltaTime);

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
