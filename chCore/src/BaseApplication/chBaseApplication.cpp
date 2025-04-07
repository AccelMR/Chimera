/************************************************************************/
/**
 * @file chBaseApplication.cpp
 * @author AccelMR
 * @date 2025/12/09
 *   BaseApplication class that handles the main loop, and the initialization of the engine.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
#include "chBaseApplication.h"

#include "chCommandParser.h"
#include "chDebug.h"
#include "chDynamicLibManager.h"
#include "chDisplayManager.h"
#include "chEventSystem.h"
#include "chPath.h"
#include "chStringUtils.h"
#include "chEventDispatcherManager.h"
#include "chIGraphicsAPI.h"

#include <chrono>

#if USING(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif //USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {
using namespace std::chrono;

/*
*/
BaseApplication::~BaseApplication() {
  CH_LOG_INFO("Destroying BaseApplication");

  if (m_isInitialized) {
    destroyModules();
  }
}

/*
*/
void
BaseApplication::initialize(int argc, char** argv) {
  initializeModules();
  initPlatform(argc, argv);
  initializeGraphics();
  m_isInitialized = true;
}

/*
*/
void
BaseApplication::initPlatform(int argc, char** argv) {
  CommandParser& commandParser = CommandParser::getInstance();
  // Parse command line arguments.
  commandParser.parse(static_cast<int32>(argc), argv);

  SCREEN_DESC winDesc;
  winDesc.name = commandParser.getParam("AppName", "Chimera Engine");
  winDesc.title = commandParser.getParam("WindowTitle", "Chimera Engine Base Application");
  winDesc.width = std::stoi(commandParser.getParam("Width", "1280"));
  winDesc.height = std::stoi(commandParser.getParam("Height", "720"));

  m_eventhandler = chMakeShared<DisplayEventHandle>();
  WeakPtr<DisplaySurface> wptrDisplay = DisplayManager::instance().createDisplay(winDesc, m_eventhandler);
  if (wptrDisplay.expired()) {
    CH_EXCEPT(InternalErrorException, "Failed to create display.");
  }
  m_display = wptrDisplay.lock();
}

/*
*/
void
BaseApplication::initializeModules() {
  DynamicLibraryManager::startUp();
  EventDispatcherManager::startUp();
  DisplayManager::startUp();
}

/*
*/
void
BaseApplication::initializeGraphics() {

  DynamicLibraryManager& dynamicLibraryManager = DynamicLibraryManager::instance();

  const String graphicsAPIName = CommandParser::getInstance().getParam("GraphicsAPI", "chVulkan");

  WeakPtr<DynamicLibrary> graphicLibrary = dynamicLibraryManager.loadDynLibrary(graphicsAPIName);
  if (graphicLibrary.expired()) {
    CH_EXCEPT(InternalErrorException, "Failed to load graphics API library.");
  }
  SPtr<DynamicLibrary> graphicLibraryPtr = graphicLibrary.lock();

  typedef void (*LoadPluginFunc)();
  LoadPluginFunc loadPlugin = (LoadPluginFunc)graphicLibraryPtr->getSymbol("loadPlugin");

  if (loadPlugin == nullptr) {
    CH_EXCEPT(InternalErrorException, "Failed to load graphics API plugin.");
  }
  loadPlugin();

  // Initialize the graphics API.
  GraphicsAPIInfo graphicsAPIInfo;
  graphicsAPIInfo.weakDisplaySurface = m_display;
  graphicsAPIInfo.width = m_display->getWidth();
  graphicsAPIInfo.height = m_display->getHeight();
  
  IGraphicsAPI::instance().initialize(graphicsAPIInfo);
}

/*
*/
void
BaseApplication::destroyModules() {
  DisplayManager::shutDown();
  EventDispatcherManager::shutDown();
  DynamicLibraryManager::shutDown();
}

/*
*/
void
BaseApplication::destroyGraphics() {
}

/*
 */
void
BaseApplication::run() {
#if USING(CH_DEBUG_MODE)
  CH_LOG_DEBUG("Running BaseApplication in Debug mode.");
#endif //CH_DEBUG_MODE

 // Make sure the application is initialized.
  CH_ASSERT(m_isInitialized);

  auto& eventDispatcher = EventDispatcherManager::instance();
  
  bool running = true;
  HEvent OnClose = eventDispatcher.OnClose.connect([&]() 
    { m_display->close();  running = false; } );

  HEvent listenKeyEscapeDown = eventDispatcher.listenKeyDown(Key::Escape, 
    [&](const KeyBoardData& keyData)
      { m_display->close(); running = false; });

  HEvent listenWDown = eventDispatcher.listenKeyDown(Key::W, 
    [&](const KeyBoardData& keyData) {
      if (keyData.hasModifier(KEY_MODIFIER::LSHIFT)) {
        CH_LOG_INFO("Key W down with shift");
      }
    });

  const double fixedTimeStamp = 1.0 / 60.0;
  double accumulator = 0.0;
  auto previousTime = high_resolution_clock::now();

  while (running) {
    //Calculate delta time
    auto currentTime = high_resolution_clock::now();
    duration<double> frameDuration = currentTime - previousTime;
    double deltaTime = frameDuration.count();
    previousTime = currentTime;

    m_eventhandler->update();
    eventDispatcher.dispatchEvents(m_eventhandler);

    while (accumulator >= fixedTimeStamp) {
      //update();
      accumulator -= fixedTimeStamp;
    }

    eventDispatcher.updateKeyboardState(); 
  }
}
} // namespace chEngineSDK