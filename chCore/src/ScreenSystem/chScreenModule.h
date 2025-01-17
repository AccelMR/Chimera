/************************************************************************/
/**
 * @file chScreenSystem.h
 * @author AccelMR
 * @date 2022/09/11
 *   Screen System module to handle creation of a window.
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

#include "chScreen.h"

namespace chEngineSDK{
class CH_CORE_EXPORT ScreenModule: public Module<ScreenModule>
{
 public:
 /*
  *   Default constructor
  */
  ScreenModule() = default;
  
  /** 
   *   Creates a screen with a given descriptor.
   * 
   * @param screenDesc
   *    Descriptor to create screen.
   * 
   * @return SPtr<Screen> 
   *    The created screen for specific OS.
   **/
  const SPtr<Screen> 
  createScreen(SCREEN_DESC screenDesc, SPtr<ScreenEventHandle> eventHandler);

  /** 
   *   Returns shared pointer to main screen.
   **/
  FORCEINLINE const SPtr<Screen> 
  getMainScreen();

 private:
  SPtr<Screen> m_mainScreen;
};

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/

/*
*/
FORCEINLINE const SPtr<Screen>
ScreenModule::getMainScreen() {
  return m_mainScreen;
}
}