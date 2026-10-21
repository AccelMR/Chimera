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
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chEventDispatcherManager.h"
#include "chGraphicsSettings.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chProfiler.h"
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

ConsoleVariable<String> g_cvarApplicationId("Application.Id",
                                            "",
                                            "Id the desktop matches the windows by (Wayland "
                                            "app_id, X11 class), e.g. in window rules. Empty "
                                            "uses the application's own.",
                                            "AppId");

ConsoleVariable<String> g_cvarWindowTitle("Window.Title",
                                          "Chimera Engine Windowed Application",
                                          "Title of the main window.",
                                          "WindowTitle");

ConsoleVariable<int32> g_cvarWindowWidth("Window.Width",
                                         0,
                                         "Width of the main window when it opens, in screen "
                                         "units. 0 opens it maximized.",
                                         "Width");

ConsoleVariable<int32> g_cvarWindowHeight("Window.Height",
                                          0,
                                          "Height of the main window when it opens, in "
                                          "screen units. 0 opens it maximized.",
                                          "Height");

ConsoleVariable<bool> g_cvarWindowMaximized("Window.Maximized",
                                            false,
                                            "Opens the main window maximized.",
                                            "Maximized");

ConsoleVariable<int32> g_cvarWindowResizeDelay("Window.ResizeDelay",
                                               150,
                                               "Milliseconds without a size change before "
                                               "the swap chain follows the window; until "
                                               "then the old size is stretched. 0 follows "
                                               "every change.",
                                               "ResizeDelay");

NODISCARD int64
getSteadyNanoseconds()
{
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}
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
    Profiler::beginFrame();

    // Calculate delta time
    auto currentTime = high_resolution_clock::now();
    auto deltaTime = duration_cast<duration<double>>(currentTime - previousTime).count();
    previousTime = currentTime;

    {
      CH_PROFILE_SCOPE("Events");
      m_eventhandler->update();
      eventDispatcher.dispatchEvents(m_eventhandler);
    }
    if (!m_running) {
      break;
    }

    {
      CH_PROFILE_SCOPE("FixedUpdate");
      accumulator += deltaTime;
      while (accumulator >= fixedTimeStamp) {
        fixedUpdate(static_cast<float>(fixedTimeStamp));
        accumulator -= fixedTimeStamp;
      }
    }
    {
      CH_PROFILE_SCOPE("Update");
      update(static_cast<float>(deltaTime));
    }
    if (!m_running) {
      break;
    }

    render();
    Profiler::endFrame();
  }
}

/*
*/
void
WindowedApplication::initialize() {
  CH_LOG_INFO(WindowedApp, "Initializing WindowedApplication.");
  initializeModules();

  loadGraphicsAPI();
  // Without a saved size the window takes the whole display, and its size before being
  // restored is the display's, so a restore does not shrink it to a small default.
  const bool bHasSize = g_cvarWindowWidth.get() > 0 && g_cvarWindowHeight.get() > 0;
  // The plugin is loaded by now, so the title can say which API draws.
  initializeDisplay(
      {.name = g_cvarApplicationName.get(),
       .appId = g_cvarApplicationId.get().empty() ? String(getDefaultApplicationId())
                                                  : g_cvarApplicationId.get(),
       .title = StringUtils::format("{0} - {1}", g_cvarWindowTitle.get(),
                                    IGraphicsAPI::instance().getAPIName()),
       .width = bHasSize ? static_cast<uint32>(g_cvarWindowWidth.get()) : 0,
       .height = bHasSize ? static_cast<uint32>(g_cvarWindowHeight.get()) : 0,
       .resizable = true,
       .maximized = !bHasSize || g_cvarWindowMaximized.get(),
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
WindowedApplication::saveWindowPlacement(StringView configFile) const
{
  // A minimized window may report a size it never had on screen, so the last good one stays.
  if (!m_display || m_display->isMinimized()) {
    return;
  }

  uint32 width = 0;
  uint32 height = 0;
  m_display->getWindowSize(width, height);
  if (width == 0 || height == 0) {
    return;
  }

  ANSICHAR buffer[StringUtils::MAX_INTEGER_CHARS];
  EngineConfig::setUserValue(configFile, "Window", "Width", StringUtils::toChars(buffer, width));
  EngineConfig::setUserValue(configFile, "Window", "Height",
                             StringUtils::toChars(buffer, height));
  EngineConfig::setUserValue(configFile, "Window", "Maximized",
                             m_display->isMaximized() ? "true" : "false");
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
WindowedApplication::render()
{
  // The surface of a minimized window is 0x0, so there is nothing to draw on and the swap
  // chain cannot be recreated. Sleeping keeps the loop from spinning until it is restored.
  if (m_display->isMinimized()) {
    std::this_thread::sleep_for(milliseconds(10));
    return;
  }

  applyPendingResize();
  // An out of date swap chain cannot present, so while a drag still changes the size the
  // frames are skipped instead of making the swap chain again for every step.
  if (m_bSwapChainOutOfDate) {
    std::this_thread::sleep_for(milliseconds(1));
    return;
  }

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  ISwapChain& swapChain = *m_swapChain;
  // Graphics.VSync may change at run time (menu, console); outside the frame, because an
  // API may make the swap chain again for it.
  if (swapChain.isVSync() != g_cvarVSync.get()) {
    swapChain.setVSync(g_cvarVSync.get());
    CH_LOG_INFO(WindowedApp, "VSync {0}.", swapChain.isVSync() ? "on" : "off");
  }

  CH_PROFILE_SCOPE("Render");
  ICommandList* commandList = nullptr;
  {
    // Waits until the GPU is done with the last frame that used this slot.
    CH_PROFILE_SCOPE("WaitForGPU");
    commandList = &graphicsAPI.beginFrame();
  }

  SwapChainStatus acquireStatus = SwapChainStatus::Failed;
  {
    CH_PROFILE_SCOPE("AcquireImage");
    acquireStatus = swapChain.acquireNextImage();
  }
  if (acquireStatus == SwapChainStatus::OutOfDate ||
      acquireStatus == SwapChainStatus::Failed) {
    graphicsAPI.endFrame();
    if (acquireStatus == SwapChainStatus::OutOfDate) {
      onSwapChainOutOfDate();
    }
    return;
  }

  {
    CH_PROFILE_SCOPE("OnRender");
    onRender(*commandList, swapChain);
  }

  SwapChainStatus presentStatus = SwapChainStatus::Failed;
  {
    CH_PROFILE_SCOPE("Submit");
    graphicsAPI.endFrame();
  }
  {
    CH_PROFILE_SCOPE("Present");
    presentStatus = swapChain.present();
    onPostPresent();
  }

  // A suboptimal image is still drawn and presented, so the swap chain is only recreated
  // after the frame is done, and not at all while a window drag is still changing the size.
  if (presentStatus == SwapChainStatus::OutOfDate) {
    onSwapChainOutOfDate();
  }
  else if (!m_bResizePending && (acquireStatus == SwapChainStatus::Suboptimal ||
                                 presentStatus == SwapChainStatus::Suboptimal)) {
    resize(m_display->getWidth(), m_display->getHeight());
  }
}

/*
 */
void
WindowedApplication::onSwapChainOutOfDate()
{
  if (m_bResizePending) {
    m_bSwapChainOutOfDate = true;
    return;
  }

  resize(m_display->getWidth(), m_display->getHeight());
}

/*
 */
void
WindowedApplication::requestResize()
{
  if (g_cvarWindowResizeDelay.get() <= 0) {
    resize(m_display->getWidth(), m_display->getHeight());
    return;
  }

  m_bResizePending = true;
  m_lastResizeRequestNs = getSteadyNanoseconds();
}

/*
 */
void
WindowedApplication::applyPendingResize()
{
  if (!m_bResizePending) {
    return;
  }

  const int64 delayNs = static_cast<int64>(g_cvarWindowResizeDelay.get()) * 1'000'000;
  if (getSteadyNanoseconds() - m_lastResizeRequestNs < delayNs) {
    return;
  }

  // The size event of a drag may arrive after an out of date present already resized to it.
  const uint32 width = m_display->getWidth();
  const uint32 height = m_display->getHeight();
  if (!m_bSwapChainOutOfDate && m_swapChain->getWidth() == width &&
      m_swapChain->getHeight() == height) {
    m_bResizePending = false;
    return;
  }

  resize(width, height);
}

/*
 */
void
WindowedApplication::resize(uint32 width, uint32 height)
{
  if (m_display->isMinimized()) {
    return;
  }

  m_bResizePending = false;
  m_bSwapChainOutOfDate = false;
  const uint32 oldWidth = m_swapChain->getWidth();
  const uint32 oldHeight = m_swapChain->getHeight();
  m_swapChain->resize(width, height);

  // An API may keep the swap chain (a window being minimized reports a 0x0 surface first).
  if (m_swapChain->getWidth() != oldWidth || m_swapChain->getHeight() != oldHeight) {
    CH_LOG_INFO(WindowedApp, "Swap chain resized to {0}x{1}.", m_swapChain->getWidth(),
                m_swapChain->getHeight());
  }
}

/*
*/
void
WindowedApplication::bindEvents() {
  CH_LOG_INFO(WindowedApp, "Binding events for WindowedApplication.");

  auto& eventDispatcher = EventDispatcherManager::instance();

  m_closeEvent = eventDispatcher.OnClose.connect([&]() {
    if (onCloseRequested()) {
      m_running = false;
    }
  });
  m_resizeEvent = eventDispatcher.OnResize.connect(
      [&](uint32 width, uint32 height) {
        CH_PARAMETER_UNUSED(width);
        CH_PARAMETER_UNUSED(height);
        requestResize();
      });
}

} // namespace chEngineSDK
