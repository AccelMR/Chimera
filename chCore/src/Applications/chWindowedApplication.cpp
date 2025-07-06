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
#include "chLogger.h"
#include "chStringUtils.h"

// Include graphics API
#include "chIGraphicsAPI.h"


CH_LOG_DECLARE_STATIC(WindowedApp, All);

namespace chEngineSDK {
using namespace std::chrono;

/*
*/
WindowedApplication::WindowedApplication() {}

/*
*/
WindowedApplication::~WindowedApplication() {
  CH_LOG_INFO(WindowedApp, "Destroying WindowedApplication.");
}

/*
*/
void
WindowedApplication::run() {
  CH_LOG_INFO(WindowedApp, "Running WindowedApplication.");

  auto& eventDispatcher = EventDispatcherManager::instance();

  const double fixedTimeStamp = 1.0 / 60.0;
  double accumulator = 0.0;
  auto previousTime = high_resolution_clock::now();

  HEvent listenKeyEscapeDown = eventDispatcher.listenKeyDown(Key::Escape,
  [&](const KeyBoardData&) {
    m_running = false;
  });
  HEvent listenCloseEvent = eventDispatcher.OnClose.connect(
  [&]() {
    m_running = false;
  });

  while (m_running) {
    //Calculate delta time
    auto currentTime = high_resolution_clock::now();
    auto deltaTime = duration_cast<duration<double>>(currentTime - previousTime).count();
    previousTime = currentTime;

    m_eventhandler->update();
    eventDispatcher.dispatchEvents(m_eventhandler);
    if (!m_running) { break; }

    // Update the accumulator
    accumulator += deltaTime;
    while (accumulator >= fixedTimeStamp) {
      // Update the application logic
      update(static_cast<float>(fixedTimeStamp));
      accumulator -= fixedTimeStamp;
    }

    // Render the application
    render(static_cast<float>(deltaTime));

    // Dispatch events
    eventDispatcher.updateStates();
  }
}

/*
*/
void
WindowedApplication::initializeModules() {
  CH_LOG_INFO(WindowedApp, "WindowedApplication initializing modules.");
  BaseApplication::initializeModules();

  if (!CommandParser::isStarted()) {
    CH_LOG_WARNING(WindowedApp,
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
WindowedApplication::onPostInitialize() {
  CH_LOG_INFO(WindowedApp, "WindowedApplication post-initialization.");

  CommandParser& commandParser = CommandParser::instance();

  initializeDisplay({
      .name = commandParser.getParam("AppName", "Chimera Engine"),
      .title = commandParser.getParam("WindowTitle", "Chimera Engine Windowed Application"),
      .width = static_cast<uint32>(commandParser.getParamAsInt("Width", 1280)),
      .height = static_cast<uint32>(commandParser.getParamAsInt("Height", 720))});
  initializeGraphics();
  initializeRenderComponents();
  CH_LOG_INFO(WindowedApp, "WindowedApplication post-initialization completed.");
}

void
WindowedApplication::onPostDestoyModules() {
  CH_LOG_INFO(WindowedApp, "WindowedApplication pre-shutdown.");
  destroyRenderer();
  destroyGraphics();
  destroyDisplay();
  CH_LOG_INFO(WindowedApp, "WindowedApplication pre-shutdown completed.");
}

void
WindowedApplication::initializeDisplay(const ScreenDescriptor& desc) {
  m_eventhandler = chMakeShared<DisplayEventHandle>(desc.width, desc.height);
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
  const String& graphicsAPIName = CommandParser::instance().getParam("GraphicsAPI", "chVulkan");

#if USING(CH_DEBUG_MODE)
  const Path dllAbsolutePath = FileSystem::absolutePath(std::move(Path("build/debug-x64/lib")));
  CH_LOG_DEBUG(WindowedApp, "Loading graphics library: {0} from path: {1}",
               graphicsAPIName, dllAbsolutePath.toString());
#endif //CH_DEBUG_MODE

  WeakPtr<DynamicLibrary> graphicsLib =
    DynamicLibraryManager::instance().loadDynLibrary(graphicsAPIName
#if USING(CH_DEBUG_MODE)
                                                    ,dllAbsolutePath
#endif //CH_DEBUG_MODE
                                                    );
  if (graphicsLib.expired()) {
    CH_EXCEPT(InternalErrorException, chString::format("Failed to load graphics library: {0}",
                                                        graphicsAPIName));
  }
  SPtr<DynamicLibrary> graphicsLibrary = graphicsLib.lock();

  // Load graphics api function pointer C style
  typedef void (*GraphicsAPIInitFunc)();
  auto initFunc = reinterpret_cast<GraphicsAPIInitFunc>(graphicsLibrary->getSymbol("loadPlugin"));

  if (!initFunc) {
    CH_EXCEPT(InternalErrorException, chString::format("Failed to get symbol 'loadPlugin' from "
                                                        "graphics library: {0}",
                                                        graphicsAPIName));
  }
  initFunc();

  // Initialize the graphics API
  IGraphicsAPI* graphicsAPI = IGraphicsAPI::instancePtr();
  if (!graphicsAPI) {
    CH_EXCEPT(InternalErrorException, "Graphics API instance is null after initialization.");
  }

  graphicsAPI->initialize({
    .weakDisplaySurface = m_display,
    //TODO: probably want to change this or make the display return sidth of the drawable area
    .width = m_display->getWidth(),
    .height = m_display->getHeight(),
    .enableValidationLayer = true
  });

  CH_LOG_INFO(WindowedApp, "Graphics subsystem initialized successfully.");
}

/*
*/
void
WindowedApplication::initializeRenderComponents() {
}

/*
*/
void
WindowedApplication::destroyGraphics() {
  CH_LOG_INFO(WindowedApp, "Destroying graphics subsystem.");
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
  }
  else {
    CH_LOG_WARNING(WindowedApp, "Display was not initialized, nothing to destroy.");
  }
}

/*
*/
void
WindowedApplication::destroyRenderer() {
  CH_LOG_INFO(WindowedApp, "Destroying renderer.");
}

/*
*/
void
WindowedApplication::render(const float deltaTime) {
  //CH_LOG_DEBUG(WindowedApp, "Rendering frame with delta time: {0}", deltaTime);
  // Call the onRender method to allow derived classes to implement their rendering logic
  onRender(deltaTime);
}

} // namespace chEngineSDK
