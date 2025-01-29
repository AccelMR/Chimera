/************************************************************************/
/**
 * @file chScreenSystem.cpp
 * @author AccelMR
 * @date 2022/09/11
 *   Screen System module to handle creation of a window.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chScreenModule.h"

#include "chDebug.h"
#include "chStringUtils.h"

#include "chScreen.h"

namespace chEngineSDK{

/*
*/
const SPtr<Screen>
ScreenModule::createScreen(SCREEN_DESC screenDesc, SPtr<ScreenEventHandle> eventHandler) {
  if (!m_mainScreen) {
    m_mainScreen = chMakeShared<Screen>();
    if (!m_mainScreen->init(screenDesc, eventHandler)) {
      return nullptr;
    }
    CH_LOG_INFO(StringUtils::format("Main Screen [{0}] has been created.", screenDesc.name));
  }
  return m_mainScreen;
}

}
