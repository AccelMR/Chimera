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
#include "chEventSystem.h"
#include "chGraphicsModule.h"
#include "chInputModule.h"
#include "chRenderer.h"
#include "chScreenModule.h"
#include "chStringUtils.h"
#include "chSystemEventDispatcherModule.h"

#include <chrono>

#if USING(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif //USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {
  using namespace std::chrono;

/*
*/
BaseApplication::~BaseApplication() {
  LOG_INFO("Destroying BaseApplication");

  if (m_isInitialized) {
    destroyModules();
  }
  
  g_Debug().saveLog(Path("log.txt"));
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
#if USING(CH_PLATFORM_LINUX)
  char exePath[1024];
  SIZE_T count = readlink("/proc/self/exe", exePath, 1024);

  if (count == -1) {
    std::cerr << "Error: No se pudo obtener la ruta del ejecutable." << std::endl;
  }

  const String exeDir = String(exePath).substr(0, String(exePath).find_last_of("/"));
  const String apiName = CommandParser::getInstance().getParam("GraphicsAPI", "Vulkan");
  const String fullApiName = StringUtils::format("ch{0}Graphics", apiName);
  const String graphicsAPIName = exeDir + "/" + apiName;
#else
  const String graphicsAPIName = std::format("ch{0}Graphics", CommandParser::getInstance().getParam("GraphicsAPI", "Vulkan"));
#endif //USING(CH_PLATFORM_LINUX)

  CommandParser& commandParser = CommandParser::getInstance();
  
  // Parse command line arguments.
  commandParser.parse(static_cast<int32>(argc), argv);

  SCREEN_DESC winDesc;
  winDesc.name = commandParser.getParam("AppName", "Chimera Engine");
  winDesc.title = commandParser.getParam("WindowTitle", "Chimera Engine Base Application");
  winDesc.width = std::stoi(commandParser.getParam("Width", "1280"));
  winDesc.height = std::stoi(commandParser.getParam("Height", "720"));

  m_eventhandler = ch_shared_ptr_new<ScreenEventHandle>();
  m_screen = ScreenModule::instance().createScreen(winDesc, m_eventhandler);
}

/*
*/
void
BaseApplication::initializeModules() {
  DynamicLibraryManager::startUp();
  InputModule::startUp();
  SystemEventDipatcherModule::startUp();
  ScreenModule::startUp();
}

/*
*/
void
BaseApplication::initializeGraphics() {
  const String APIName = "ch" + 
    CommandParser::getInstance().getParam("GraphicsAPI", "Vulkan") +
    "Graphics";
  WPtr<DynamicLibrary> dllGraphics = DynamicLibraryManager::instance().loadDynLibrary(APIName);
  if (SPtr<DynamicLibrary> dll = dllGraphics.lock()) {
    auto startUp = reinterpret_cast<void(*)()>(dll->getSymbol("loadPlugin"));
    CH_ASSERT(startUp);
    startUp();
    CH_ASSERT(m_screen);
    GraphicsModule::instance().initialize(m_screen);
  }
  else {
    LOG_ERROR(StringUtils::format("Could not load graphics API: {0}", APIName));
  }

  Renderer::startUp();
  Renderer::instance().initialize();
}

/*
*/
void
BaseApplication::destroyModules() {
  ScreenModule::shutDown();
  InputModule::shutDown();
  SystemEventDipatcherModule::shutDown();
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
  LOG_DBG("Running BaseApplication in Debug mode.");
#endif //CH_DEBUG_MODE

 // Make sure the application is initialized.
  CH_ASSERT(m_isInitialized);

  auto [inputDispatcher, systemDispatcher, renderer] = 
    std::tie(InputModule::instance(),
             SystemEventDipatcherModule::instance(),
             Renderer::instance());

  bool running = true;
  HEvent OnClose = systemDispatcher.listenOnClose([&]() { m_screen->close();  running = false; } );

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
    systemDispatcher.dispatchEvents(m_eventhandler);
    inputDispatcher.dispatchEvents(m_eventhandler);

    while (accumulator >= fixedTimeStamp) {
      //update();
      accumulator -= fixedTimeStamp;
    }

    // Render
    renderer.render();
  }
}
} // namespace chEngineSDK