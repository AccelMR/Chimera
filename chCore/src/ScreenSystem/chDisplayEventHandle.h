/************************************************************************/
/**
 * @file chDisplayEventHandle.h
 * @author AccelMR
 * @date 2022/09/12
 *    DisplaySurface event handler. Platform independent.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chDisplayEvent.h"

namespace chEngineSDK {
//Alias for a platform specific callback.
using PlatformCallback = void *;

/*
 * Description: 
 *     DisplaySurface Event handler that keeps track of all messages sent from platform to Chimera.
 *
 * Sample usage: *
 *  DisplayManager::startUp();
 *
 *  SCREEN_DESC winDesc;
 *  winDesc.name = "ChimeraCoreUnitTest";
 *  winDesc.title = "Chimera Core Unit Test";
 *
 *  auto Eventhandler = chMakeShared<DisplayEventHandle>();
 *  auto screen = DisplayManager::instance().createDisplay(winDesc, Eventhandler);
 *
 *  bool running = true;
 *  while (running) {
 *    Eventhandler->update();
 *
 *    while (!Eventhandler->empty()) {
 *      DisplayEvent event = Eventhandler->frontPop();
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
class CH_CORE_EXPORT DisplayEventHandle
{
 public:
 /*
  *   Default constructor
  */
  DisplayEventHandle() = default;

 /*
  *   Default destructor
  */
  ~DisplayEventHandle() = default;

  /** 
   *   Updates queue message from system.
   **/
  void
  update();

  /** 
   *   Returns the front event in the queue.
   **/
  NODISCARD FORCEINLINE DisplayEvent&
  frontEventQueue();

  /** 
   *   Removes the front event in the queue.
   **/
  FORCEINLINE void
  popEventQueue();

  /** 
   *   Returns and removes the front event in queue.
   **/
  NODISCARD FORCEINLINE DisplayEvent&
  frontPopEventQueue();

  /** 
   *   Returns true if the event queue is empty.
   **/
  NODISCARD FORCEINLINE bool
  isEventQueueEmpty();

 protected:
  friend class DisplaySurface;

  /** 
   *   Returns callback for platform specific.
   **/
  PlatformCallback*
  getPlatformCallBack();

  /** 
   *   Adds a event to the queue.
   **/
  template<typename ... Args>
  FORCEINLINE void
  addEvent(Args&& ... args) {
    m_eventQueue.emplace(std::forward<Args>(args)...);
  }

 private:
 // TODO: create a custom Prealocator for this queue, this way we can avoid fragmentation and reallocation.
  Queue<DisplayEvent> m_eventQueue;
};

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/

/*
*/
DisplayEvent&
DisplayEventHandle::frontEventQueue() {
  return m_eventQueue.front();
}

/*
*/
void
DisplayEventHandle::popEventQueue() {
  m_eventQueue.pop();
}

/*
*/
DisplayEvent&
DisplayEventHandle::frontPopEventQueue() {
  DisplayEvent& returnEvent = m_eventQueue.front();
  popEventQueue();
  return returnEvent;
}

/*
*/
bool
DisplayEventHandle::isEventQueueEmpty() {
  return m_eventQueue.empty();
}
}