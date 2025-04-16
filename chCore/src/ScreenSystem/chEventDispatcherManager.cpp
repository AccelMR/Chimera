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
#include "chEventDispatcherManager.h"

#include "chDisplayEvent.h"
#include "chDisplayEventHandle.h"
#include "chStringUtils.h"
#include "chEventSystem.h"

namespace chEngineSDK{
using std::get_if;

#if USING(CH_DEBUG_MODE)
#define CH_EVENT_DISPATCHER_MANAGER_LOG_LEVEL All
#else
#define CH_EVENT_DISPATCHER_MANAGER_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)

CH_LOG_DEFINE_CATEGORY_SHARED(InputSystem, CH_EVENT_DISPATCHER_MANAGER_LOG_LEVEL);

EventDispatcherManager::EventDispatcherManager()
{
  for (int i = 0; i < static_cast<int>(Key::KeysMax); ++i) {
    Key key = static_cast<Key>(i);
    KeyDownCallbacks.emplace(key, Event<void(const KeyBoardData&)>());
    KeyPressedCallbacks.emplace(key, Event<void(const KeyBoardData&)>());
    KeyUpCallbacks.emplace(key, Event<void(const KeyBoardData&)>());
  }

  // Initialize the keyboard state
  m_currentKeyboardState.reset();
  m_previousKeyboardState.reset();
}

/*
*/
NODISCARD bool
EventDispatcherManager::dispatchInputEvents(const DisplayEvent &sEvent) {
  bool dispatched = true;
  const DisplayEventData& eventData = sEvent.getData();
  switch (sEvent.getType())
  {
  case PLATFORM_EVENT_TYPE::kCLOSE:
    OnClose();
    break;

  case PLATFORM_EVENT_TYPE::kRESIZE:
  {
    if (const auto& resizeData = get_if<ResizeData>(&eventData)) {
      OnResize(resizeData->width, resizeData->height);
    }
    else {
      CH_LOG_DEBUG(InputSystem, "Resize data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kMOUSE_MOVE:
  {
    if (const auto& mouseData = get_if<MouseMoveData>(&eventData)) {
      OnMouseMove(*mouseData);
    }
    else {
      CH_LOG_DEBUG(InputSystem, "Mouse data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kKEYBOARD:
  {
    if (const auto& keyData = get_if<KeyBoardData>(&eventData)) {
      dispatchKeyboardEvent(*keyData);
    }
    else {
      CH_LOG_DEBUG(InputSystem, "Key data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kNONE:
  default:
    dispatched = false;
  }
  
  return dispatched;
}

/*
*/
void 
EventDispatcherManager::dispatchKeyboardEvent(const KeyBoardData& keyData) {
  if (keyData.key >= Key::KeysMax) {
    CH_LOG_DEBUG(InputSystem, "Key out of range: {0}", static_cast<uint32_t>(keyData.key));
    return;
  }

  switch (keyData.state) {
    case KEYBOARD_STATE::PRESSED:
      m_currentKeyboardState.set(static_cast<uint32_t>(keyData.key));
      KeyPressedCallbacks.at(keyData.key)(keyData);
      break;
    case KEYBOARD_STATE::DOWN:
      m_currentKeyboardState.set(static_cast<uint32_t>(keyData.key));
      KeyDownCallbacks.at(keyData.key)(keyData);
      break;

    case KEYBOARD_STATE::UP:
      m_currentKeyboardState.reset(static_cast<uint32_t>(keyData.key));
      KeyUpCallbacks.at(keyData.key)(keyData);
      break;

    default:
      CH_LOG_DEBUG(InputSystem, "Invalid key state.");
      break;
  }
}

/*
*/
void
EventDispatcherManager::dispatchEvents(const SPtr<DisplayEventHandle>& eventHandler) {
  while (!eventHandler->isEventQueueEmpty()) {
    DisplayEvent& screenEvent = eventHandler->frontPopEventQueue();
    if (!dispatchInputEvents(screenEvent)) {
      CH_LOG_DEBUG(InputSystem, "Could not dispatch event : [{0}] in System Queue.", screenEvent.getTypeAsString());
    }
  }
}

}  // namespace chEngineSDK