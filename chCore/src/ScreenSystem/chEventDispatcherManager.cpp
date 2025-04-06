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

#include "chDebug.h"

#include "chDisplayEvent.h"
#include "chDisplayEventHandle.h"
#include "chStringUtils.h"
#include "chEventSystem.h"

namespace chEngineSDK{
using std::get_if;

EventDispatcherManager::EventDispatcherManager()
  : m_currentKeyboardState(0)
  , m_previousKeyboardState(0)
{
  for (int i = 0; i < static_cast<int>(Key::KeysMax); ++i) {
    Key key = static_cast<Key>(i);
    KeyDownCallbacks.emplace(key, Event<void()>());
    KeyPressedCallbacks.emplace(key, Event<void()>());
    KeyUpCallbacks.emplace(key, Event<void()>());
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
  switch (sEvent.getType())
  {
  case PLATFORM_EVENT_TYPE::kCLOSE:
    OnClose();
    break;

  case PLATFORM_EVENT_TYPE::kRESIZE:
  {
    const auto eventData = sEvent.getData();
    if (const auto& resizeData = get_if<ResizeData>(&eventData)) {
      OnResize(resizeData->width, resizeData->height);
    }
    else {
      CH_LOG_ERROR("Resize data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kMOUSE_MOVE:
  {
    const auto eventData = sEvent.getData();
    if (const auto& mouseData = get_if<MouseMoveData>(&eventData)) {
      OnMouseMove(*mouseData);
    }
    else {
      CH_LOG_ERROR("Mouse data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kKEY_DOWN:
  {
    const auto eventData = sEvent.getData();
    if (const auto& keyData = get_if<KeyBoardData>(&eventData)) {
      dispatchKeyEvent(PLATFORM_EVENT_TYPE::kKEY_DOWN, keyData->key);
    }
    else {
      CH_LOG_ERROR("Key data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PLATFORM_EVENT_TYPE::kKEY_UP:
  {
    const auto eventData = sEvent.getData();
    if (const auto& keyData = get_if<KeyBoardData>(&eventData)) {
      dispatchKeyEvent(PLATFORM_EVENT_TYPE::kKEY_UP, keyData->key);
    }
    else {
      CH_LOG_ERROR("Key data is not valid.");
      dispatched = false;
    }
  }
  break;

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
EventDispatcherManager::dispatchKeyEvent(const PLATFORM_EVENT_TYPE& type, const Key& key) {
  switch (type)
  {
  case PLATFORM_EVENT_TYPE::kKEY_DOWN:
  {
    m_currentKeyboardState.set(static_cast<SIZE_T>(key));

    if (m_previousKeyboardState.test(static_cast<SIZE_T>(key))) {
      KeyPressedCallbacks.at(key)();
    }
    else {
      KeyDownCallbacks.at(key)();
    }
  }
    break;

  case PLATFORM_EVENT_TYPE::kKEY_UP:
  {
    m_currentKeyboardState.reset(static_cast<SIZE_T>(key));

    if (m_previousKeyboardState.test(static_cast<SIZE_T>(key))) {
      KeyUpCallbacks.at(key)();
    }
  }
  break;

  default:
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
      CH_LOG_ERROR(StringUtils::format("Could not dispatch event : [{0}] in System Queue.", screenEvent.getTypeAsString()));
    }
  }
}

}  // namespace chEngineSDK