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

#include "chLogger.h"
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

  destroyModules();
  onPostDestoyModules();
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
