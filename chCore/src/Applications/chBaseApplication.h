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
 *     What each executable sets before the application exists.
 */
struct LaunchInfo
{
  // File name inside EnginePaths::getLogDirectory().
  String logFileName = "Chimera.log";
  // Log entries kept in memory for listeners such as the editor's output log. 0 turns
  // the buffer off, and then the log file misses what was logged before it was opened.
  uint32 logBufferSize = 500;
};

/*
 * Description:
 *     Base class for the application.
 */
class CH_CORE_EXPORT BaseApplication : public Module<BaseApplication> {
 public:
  /*
   * Description:
   *     Runs the whole program: starts the Logger, reads the command line, finds the
   *     engine and project folders, then starts, runs and shuts down an application of
   *     type T, also when its start up fails. Returns the process exit code.
   */
  template<class T>
  static int32
  launch(int32 argc, ANSICHAR* argv[], const LaunchInfo& info = LaunchInfo())
  {
    return launch(argc, argv, info, [] { startUp<T>(); });
  }

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
  static int32
  launch(int32 argc, ANSICHAR* argv[], const LaunchInfo& info, void (*startUpApplication)());

 private:
  bool m_running = true; ///< Flag to indicate if the application is running.
};
} // namespace chEngineSDK

#define CH_BASE_APPLICATION_MAIN
