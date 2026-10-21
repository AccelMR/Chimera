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

#include "chLogger.h"
#include "chStringUtils.h"

#include "chDisplaySurface.h"

namespace chEngineSDK{

#if USING(CH_DEBUG_MODE)
#define CH_DISPLAY_MANAGER_LOG_LEVEL All
#else
#define CH_DISPLAY_MANAGER_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)

CH_LOG_DEFINE_CATEGORY_SHARED(DisplaySystem, CH_DISPLAY_MANAGER_LOG_LEVEL);

/*
*/
const WeakPtr<DisplaySurface>
DisplayManager::createDisplay(ScreenDescriptor screenDesc, SPtr<DisplayEventHandle> eventHandler) {
  if (!m_mainDisplay) {
    m_mainDisplay = chMakeShared<DisplaySurface>();
    if (!m_mainDisplay->init(screenDesc, eventHandler)) {
      return WeakPtr<DisplaySurface>();
    }
    CH_LOG_INFO(DisplaySystem, "Main DisplaySurface [{0}] has been created.", screenDesc.name);
  }
  return m_mainDisplay;
}
} // namespace chEngineSDK
