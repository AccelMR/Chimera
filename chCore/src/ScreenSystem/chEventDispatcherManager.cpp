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

  for (uint32 i = 0; i < static_cast<uint32>(MouseButton::MouseButtonsMax); ++i) {
    MouseButton button = static_cast<MouseButton>(i);
    MouseButtonDownCallbacks.emplace(button, Event<void(const MouseButtonData&)>());
    //MouseButtonPressedCallbacks.emplace(button, Event<void(const MouseButtonData&)>());
    MouseButtonUpCallbacks.emplace(button, Event<void(const MouseButtonData&)>());
  }

  // Initialize the mouse state
  m_currentMouseState.reset();
}

/*
*/
NODISCARD bool
EventDispatcherManager::dispatchInputEvents(const DisplayEvent &sEvent) {
  bool dispatched = true;
  const DisplayEventData& eventData = sEvent.getData();
  switch (sEvent.getType())
  {
  case PlatformEventType::Close:
    OnClose();
    break;

  case PlatformEventType::Resize:
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

  case PlatformEventType::MouseMove:
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

  case PlatformEventType::MouseButton:
  {
    if (const auto& buttonData = get_if<MouseButtonData>(&eventData)) {
      dispatchMouseButtonEvent(*buttonData);
    }
    else {
      CH_LOG_DEBUG(InputSystem, "Mouse button data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PlatformEventType::Keyboard:
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

  case PlatformEventType::MouseWheel:
  {
    if (const auto& wheelData = get_if<MouseWheelData>(&eventData)) {
      OnMouseWheel(*wheelData);
    }
    else {
      CH_LOG_DEBUG(InputSystem, "Mouse wheel data is not valid.");
      dispatched = false;
    }
  }
  break;

  case PlatformEventType::NonePlatformEvent:
  default:
    dispatched = false;
  }

  return dispatched;
}

/*
*/
void
EventDispatcherManager::dispatchKeyboardEvent(const KeyBoardData& keyData) {
  CH_ASSERT(keyData.key <= Key::KeysMax);

  switch (keyData.state) {
    case KeyBoardState::PRESSED:
      m_currentKeyboardState.set(static_cast<uint32_t>(keyData.key));
      KeyPressedCallbacks.at(keyData.key)(keyData);
      OnKeyPressed(keyData);
      break;
    case KeyBoardState::DOWN:
      m_currentKeyboardState.set(static_cast<uint32_t>(keyData.key));
      KeyDownCallbacks.at(keyData.key)(keyData);
      OnKeyDown(keyData);
      break;

    case KeyBoardState::UP:
      m_currentKeyboardState.reset(static_cast<uint32_t>(keyData.key));
      KeyUpCallbacks.at(keyData.key)(keyData);
      OnKeyUp(keyData);
      break;

    default:
      CH_LOG_DEBUG(InputSystem, "Invalid key state.");
      break;
  }
}

/*
*/
void
EventDispatcherManager::dispatchMouseButtonEvent(const MouseButtonData& buttonData) {
  CH_ASSERT(buttonData.button <= MouseButton::MouseButtonsMax);

  switch (buttonData.state) {
    // case MouseState::Pressed:
    //   m_currentMouseState.set(static_cast<uint32_t>(buttonData.button));
    //   MouseButtonPressedCallbacks.at(buttonData.button)(buttonData);
    //   OnMouseButtonPressed(buttonData);
    //   break;
    case MouseState::Down:
      m_currentMouseState.set(static_cast<uint32_t>(buttonData.button));
      MouseButtonDownCallbacks.at(buttonData.button)(buttonData);
      OnMouseButtonDown(buttonData);
      break;
    case MouseState::Up:
      m_currentMouseState.reset(static_cast<uint32_t>(buttonData.button));
      MouseButtonUpCallbacks.at(buttonData.button)(buttonData);
      OnMouseButtonUp(buttonData);
      break;
    default:
      CH_LOG_DEBUG(InputSystem, "Invalid mouse button state.");
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
