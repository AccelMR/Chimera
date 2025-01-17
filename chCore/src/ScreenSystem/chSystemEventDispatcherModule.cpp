/************************************************************************/
/**
 * @file chSystemEventDispatcherModule.h
 * @author AccelMR
 * @date 2022/09/13
 *   
 *    Event handler for system events specific. 
  Will execute platform specific function if needed.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chSystemEventDispatcherModule.h"

#include "chDebug.h"

#include "chScreenEvent.h"
#include "chScreenEventHandle.h"
#include "chStringUtils.h"
#include "chEventSystem.h"

namespace chEngineSDK{

/*
*/
NODISCARD bool
SystemEventDipatcherModule::dispatchInputEvents(const ScreenEvent &sEvent) {
  bool dispatched = true;
  switch (sEvent.getType())
  {
  case PLATFORM_EVENT_TYPE::kCLOSE:
    OnClose();
    break;

  case PLATFORM_EVENT_TYPE::kRESIZE:
  {
    auto resizeData = sEvent.getData().resizeData;
    OnResize(resizeData.width, resizeData.height);
    break;
  }

  case PLATFORM_EVENT_TYPE::kNONE:
  default:
    dispatched = false;
    break;
  }
  
  return dispatched;
}

/*
*/
void
SystemEventDipatcherModule::dispatchEvents(const SPtr<ScreenEventHandle>& eventHandler) {
  while (!eventHandler->isSystemEventQueueEmpty()) {
    ScreenEvent& screenEvent = eventHandler->frontPopSystemEventQueue();
    if (!dispatchInputEvents(screenEvent)) {
      LOG_ERROR(StringUtils::format("Could not dispatch event : [{0}] in System Queue.", screenEvent.getTypeAsString()));
    }
  }
}

/*
*/
HEvent
SystemEventDipatcherModule::listenOnClose(CloseCallback func) const {
  return OnClose.connect(func);
}

/*
*/
HEvent
SystemEventDipatcherModule::listenOnResize(ResizeCallback func) const {
  return OnResize.connect(func);
}
}