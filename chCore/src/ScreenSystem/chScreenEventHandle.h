/************************************************************************/
/**
 * @file chScreenEventHandle.h
 * @author AccelMR
 * @date 2022/09/12
 *    Screen event handler. Platform independent.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chScreenEvent.h"

namespace chEngineSDK {
//Alias for a platform specific callback.
using PlatformCallback = void *;

/*
 * Description: 
 *     Screen Event handler that keeps track of all messages sent from platform to Chimera.
 *
 * Sample usage: *
 *  ScreenModule::startUp();
 *
 *  SCREEN_DESC winDesc;
 *  winDesc.name = "ChimeraCoreUnitTest";
 *  winDesc.title = "Chimera Core Unit Test";
 *
 *  auto Eventhandler = ch_shared_ptr_new<ScreenEventHandle>();
 *  auto screen = ScreenModule::instance().createScreen(winDesc, Eventhandler);
 *
 *  bool running = true;
 *  while (running) {
 *    Eventhandler->update();
 *
 *    while (!Eventhandler->empty()) {
 *      ScreenEvent event = Eventhandler->frontPop();
 *      switch (event.getType())
 *      {
 *      case PLATFORM_EVENT_TYPE::kCLOSE:
 *        screen->close();
 *        running = false;
 *      break;
 *        break;
 *      default:
 *        break;
 *      }
 *    }
 *  }
 *
 */
class CH_CORE_EXPORT ScreenEventHandle
{
 public:
 /*
  *   Default constructor
  */
  ScreenEventHandle() = default;

 /*
  *   Default destructor
  */
  ~ScreenEventHandle() = default;

  /** 
   *   Updates queue message from system.
   **/
  void
  update();

  /** 
   *   Returns the front event in the queue.
   **/
  FORCEINLINE ScreenEvent&
  frontInputEventQueue();

  /** 
   *   Removes the front event in the queue.
   **/
  FORCEINLINE void
  popInputEventQueue();

  /** 
   *   Returns and removes the front event in queue.
   **/
  FORCEINLINE ScreenEvent&
  frontPopInputEventQueue();

  /** 
   *   Returns true if the event queue is empty.
   **/
  NODISCARD FORCEINLINE bool
  isInputEventQueueEmpty();

  /** 
   *   Adds a event to the queue.
   * TOD: NOTE: should probably not be public.
   **/
  FORCEINLINE void
  addInputEvent(ScreenEvent newEvent);

  /** 
   *   Returns the front event in the queue.
   **/
  FORCEINLINE ScreenEvent&
  frontSystemEventQueue();

  /** 
   *   Removes the front event in the queue.
   **/
  FORCEINLINE void
  popSytemEventQueue();

  /** 
   *   Returns and removes the front event in queue.
   **/
  FORCEINLINE ScreenEvent&
  frontPopSystemEventQueue();

  /** 
   *   Returns true if the event queue is empty.
   **/
  NODISCARD FORCEINLINE bool
  isSystemEventQueueEmpty();

  /** 
   *   Adds a event to the queue.
   * TOD: NOTE: should probably not be public.
   **/
  FORCEINLINE void
  addSystemEvent(ScreenEvent newEvent);

 protected:
  friend class Screen;

  /** 
   *   Returns callback for platform specific.
   **/
  PlatformCallback*
  getPlatformCallBack();

 private:
  Queue<ScreenEvent> m_inputEventQueue;
  Queue<ScreenEvent> m_systemEventQueue;
};

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/

/*
*/
ScreenEvent&
ScreenEventHandle::frontInputEventQueue() {
  return m_inputEventQueue.front();
}

/*
*/
void
ScreenEventHandle::popInputEventQueue() {
  m_inputEventQueue.pop();
}

/*
*/
ScreenEvent&
ScreenEventHandle::frontPopInputEventQueue() {
  ScreenEvent& returnEvent = m_inputEventQueue.front();
  popInputEventQueue();
  return returnEvent;
}

/*
*/
void
ScreenEventHandle::addInputEvent(ScreenEvent newEvent) {
  m_inputEventQueue.push(newEvent);
}

/*
*/
bool
ScreenEventHandle::isInputEventQueueEmpty() {
  return m_inputEventQueue.empty();
}


/*
*/
ScreenEvent&
ScreenEventHandle::frontSystemEventQueue() {
  return m_inputEventQueue.front();
}

/*
*/
void
ScreenEventHandle::popSytemEventQueue() {
  m_systemEventQueue.pop();
}

/*
*/
ScreenEvent&
ScreenEventHandle::frontPopSystemEventQueue() {
  ScreenEvent& returnEvent = m_systemEventQueue.front();
  popSytemEventQueue();
  return returnEvent;
}

/*
*/
void
ScreenEventHandle::addSystemEvent( ScreenEvent newEvent ) {
  m_systemEventQueue.push( newEvent );
}

/*
*/
bool
ScreenEventHandle::isSystemEventQueueEmpty() {
  return m_systemEventQueue.empty();
}
}