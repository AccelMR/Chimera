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
class Renderer;

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
  initialize(int argc, char** argv);

  /*
   * Description: 
   *     Updates the application.
   */
  void
  run();

  /*
   * Description: 
   *     Initializes the platform.
   */
  FORCEINLINE bool 
  getIsInitialized() const {
    return m_isInitialized;
  }

 protected:
  virtual void
  initPlatform(int argc, char** argv);

  virtual void
  initializeGraphics();

  virtual void
  initializeModules();

  virtual void
  destroyModules();

  virtual void 
  destroyGraphics();

  //TODO: delete this
  void
  render();

 protected:
  bool m_isInitialized = false;
  SPtr<DisplayEventHandle> m_eventhandler;
  SPtr<DisplaySurface> m_display;

  SPtr<Renderer> m_renderer;
};
} // namespace chEngineSDK

#define CH_BASE_APPLICATION_MAIN 
