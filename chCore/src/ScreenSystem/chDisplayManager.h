/************************************************************************/
/**
 * @file chDisplaySystem.h
 * @author AccelMR
 * @date 2022/09/11
 *   DisplaySurface System module to handle creation of a window.
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

#include "chDisplaySurface.h"

namespace chEngineSDK{
class CH_CORE_EXPORT DisplayManager: public Module<DisplayManager>
{
 public:
 /*
  *   Default constructor
  */
  DisplayManager() = default;
  
  /** 
   *   Creates a screen with a given descriptor.
   * 
   * @param screenDesc
   *    Descriptor to create screen.
   * 
   * @return SPtr<DisplaySurface> 
   *    The created screen for specific OS.
   **/
  const WeakPtr<DisplaySurface> 
  createDisplay(ScreenDescriptor screenDesc, SPtr<DisplayEventHandle> eventHandler);

  /** 
   *   Returns shared pointer to main screen.
   **/
  FORCEINLINE const SPtr<DisplaySurface> 
  getMainDisplay();

 private:
  SPtr<DisplaySurface> m_mainDisplay;
};

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/

/*
*/
FORCEINLINE const SPtr<DisplaySurface>
DisplayManager::getMainDisplay() {
  return m_mainDisplay;
}
}