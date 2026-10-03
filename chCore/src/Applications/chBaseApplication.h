/************************************************************************/
/**
 * @file chBaseApplication.h
 * @author AccelMR
 * @date 2025/12/09
 *   BaseApplication class that handles the main loop, and the initialization of the engine.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
  * Includes
  */
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chModule.h"

namespace chEngineSDK {
/*
 * Description:
 *     Base class for the application.
 */
class CH_CORE_EXPORT BaseApplication : public Module<BaseApplication> {
 public:


  /*
   * Description:
   *     Default constructor.
   */
  BaseApplication();

  /*
   * Description:
   *     Default destructor.
   */
  virtual ~BaseApplication();

  virtual void
  initialize();

  /*
   * Description:
   *     Updates the application.
   */
  virtual void
  run();

  virtual void
  requestExit(const String& reason);

 protected:
  void
  onShutDown() override;

  virtual void
  initializeModules() {}

  /*
   * Description:
   *     Each level frees what it created, in reverse order of initialization, and then
   *     calls its parent. It also runs when initialize() failed half way, so every step
   *     must accept modules and objects that were never created.
   */
  virtual void
  destroyModules() {}

  virtual void
  onPostInitialize() {}

  virtual void
  update(const float deltaTime) { CH_PARAMETER_UNUSED(deltaTime); }

 private:
  bool m_running = true; ///< Flag to indicate if the application is running.
};
} // namespace chEngineSDK

#define CH_BASE_APPLICATION_MAIN
