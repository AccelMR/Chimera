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

#include <chrono>

#include "chCommandLine.h"
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chException.h"
#include "chLogger.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {
using namespace std::chrono;
using std::stoi;

#if USING(CH_DEBUG_MODE)
#define CH_BASE_APPLICATION_LOG_LEVEL All
#else
#define CH_BASE_APPLICATION_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)


CH_LOG_DECLARE_STATIC(BaseApp, CH_BASE_APPLICATION_LOG_LEVEL);

/*
*/
BaseApplication::BaseApplication() {
}

/*
*/
BaseApplication::~BaseApplication() {
  CH_LOG_INFO(BaseApp, "Destroying BaseApplication");
}

/*
*/
int32
BaseApplication::launch(int32 argc,
                        ANSICHAR* argv[],
                        const LaunchInfo& info,
                        void (*startUpApplication)())
{
  Logger::startUp();
  Logger& logger = Logger::instance();
  logger.setBufferingEnabled(info.logBufferSize > 0, info.logBufferSize);

  CommandLine::initialize(argc, argv);

  int32 exitCode = 0;
  try {
    // The log file lives in the project's Saved folder, so the project (-project) must
    // be known before the file is opened.
    EnginePaths::initialize();
    const Path logFile = EnginePaths::getLogDirectory().join(Path(info.logFileName));
    logger.setFileOutput(true, logFile.toString());

    // After the paths, because the project decides which config files exist.
    EngineConfig::initialize();

    startUpApplication();
    BaseApplication& app = instance();
    app.initialize();
    app.run();
    CH_LOG_INFO(BaseApp, "Application finished successfully.");
  } catch (const Exception& e) {
    CH_LOG_ERROR(BaseApp, "Exception caught: {0}", e.what());
    exitCode = 1;
  } catch (...) {
    CH_LOG_ERROR(BaseApp, "Unknown exception caught.");
    exitCode = 1;
  }

  // Also after a failed start up, so the modules that did start are shut down.
  try {
    if (isStarted()) {
      shutDown();
    }
  } catch (const Exception& e) {
    CH_LOG_ERROR(BaseApp, "Exception caught while shutting down: {0}", e.what());
    exitCode = 1;
  } catch (...) {
    CH_LOG_ERROR(BaseApp, "Unknown exception caught while shutting down.");
    exitCode = 1;
  }

  Logger::shutDown();
  return exitCode;
}

/*
*/
void
BaseApplication::onShutDown()
{
  // Module::shutDown calls this before the delete, while the object still has its full
  // type. Called from the destructor, destroyModules would not reach the derived classes.
  destroyModules();
}

/*
*/
void
BaseApplication::initialize() {
  initializeModules();
  onPostInitialize();

  CH_LOG_INFO(BaseApp, "BaseApplication initialized successfully.");
}

/*
 */
void
BaseApplication::run() {
  const double fixedTimeStamp = 1.0 / 60.0;
  double accumulator = 0.0;
  auto previousTime = high_resolution_clock::now();

  while (m_running) {
    //Calculate delta time
    auto currentTime = high_resolution_clock::now();
    auto deltaTime = duration_cast<duration<double>>(currentTime - previousTime).count();
    previousTime = currentTime;

    if (!m_running) { break; }

    // Update the accumulator
    accumulator += deltaTime;
    while (accumulator >= fixedTimeStamp) {
      // Update the application logic
      update(static_cast<float>(fixedTimeStamp));
      accumulator -= fixedTimeStamp;
    }
  }
}

/*
*/
void
BaseApplication::requestExit(const String& reason) {
  CH_LOG_INFO(BaseApp, "Requesting exit: {0}", reason);
  m_running = false;
}

} // namespace chEngineSDK
