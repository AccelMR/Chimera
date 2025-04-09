/************************************************************************/
/**
 * @file chDisplaySystem.cpp
 * @author AccelMR
 * @date 2022/09/11
 *   DisplaySurface System module to handle creation of a window.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDisplayManager.h"

#include "chDebug.h"
#include "chStringUtils.h"

#include "chDisplaySurface.h"

namespace chEngineSDK{

/*
*/
const WeakPtr<DisplaySurface>
DisplayManager::createDisplay(ScreenDescriptor screenDesc, SPtr<DisplayEventHandle> eventHandler) {
  if (!m_mainDisplay) {
    m_mainDisplay = chMakeShared<DisplaySurface>();
    if (!m_mainDisplay->init(screenDesc, eventHandler)) {
      return WeakPtr<DisplaySurface>();
    }
    CH_LOG_INFO(StringUtils::format("Main DisplaySurface [{0}] has been created.", screenDesc.name));
  }
  return m_mainDisplay;
}
} // namespace chEngineSDK
