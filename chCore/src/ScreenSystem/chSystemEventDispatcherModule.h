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
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
 #include "chPrerequisitesCore.h"

#include "chModule.h"
#include "chEventSystem.h"

namespace chEngineSDK {
using CloseCallback = std::function<void()>;
using ResizeCallback = std::function<void(uint32,uint32)>;

class CH_CORE_EXPORT SystemEventDipatcherModule: public Module<SystemEventDipatcherModule>
{
 public:
 
 /** 
  *   Dispatches an event calling its respective callback.
  * 
  * @param sEvent
  *    Event to be dispatched.
  *
  * @return bool
  *   True if event could be dispatched.
  **/
  NODISCARD bool 
  dispatchInputEvents(const ScreenEvent& sEvent);
  
  /** 
   *   Dispatches a whole queue of events.
   * 
   * @param eventHandler
   *    Pointer to the event handler to dispatch.
   **/
  void
  dispatchEvents(const SPtr<ScreenEventHandle>& eventHandler);

  HEvent
  listenOnClose(CloseCallback func) const;

  HEvent
  listenOnResize(ResizeCallback func) const;


 private:
  const Event<void()> OnClose;
  const Event<void(uint32,uint32)> OnResize;
};
}