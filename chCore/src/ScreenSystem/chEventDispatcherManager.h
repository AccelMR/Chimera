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

#include "chLogger.h"
#include "chDisplayEvent.h"
#include "chEventSystem.h"
#include "chKeyBoard.h"
#include "chModule.h"
#include "chStringUtils.h"

namespace chEngineSDK {
using chKeyBoard::Key;
using CloseCallback = Function<void()>;
using ResizeCallback = Function<void(uint32,uint32)>;
using KeyDownCallback = Function<void(const KeyBoardData&)>;
using KeyUpCallback = Function<void(const KeyBoardData&)>;
using KeyPressedCallback = Function<void(const KeyBoardData&)>;
using OnMouseMoveCallback = Function<void(const MouseMoveData&)>;

using KeyboardBitSet = BitSet<static_cast<int32>(Key::KeysMax)>;
using MouseBitSet = BitSet<static_cast<int32>(MouseButton::MouseButtonsMax)>;

CH_LOG_DECLARE_EXTERN(InputSystem);

class CH_CORE_EXPORT EventDispatcherManager: public Module<EventDispatcherManager>
{
 public:

  /**
   *   Default constructor
   **/
  EventDispatcherManager();

  /**
   *   Default destructor
   **/
  ~EventDispatcherManager() = default;

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
  dispatchInputEvents(const DisplayEvent& sEvent);

  /**
   *   Dispatches a whole queue of events.
   *
   * @param eventHandler
   *    Pointer to the event handler to dispatch.
   **/
  void
  dispatchEvents(const SPtr<DisplayEventHandle>& eventHandler);

  /**
   *   Adds a callback for a key down event.
   *
   * @param key
   *    Key to add the callback for.
   *
   * @param callback
   *    Callback to be called when the key is pressed.
   **/
  FORCEINLINE HEvent
  listenKeyDown(Key key, KeyDownCallback callback) const {
    return KeyDownCallbacks.at(key).connect(callback);
  }

  FORCEINLINE HEvent
  listenAnyKeyDown(KeyDownCallback callback) const {
    return KeyDownCallbacks.at(Key::KeysMax).connect(callback);
  }

  /**
   *   Adds a callback for a key up event.
   *
   * @param key
   *    Key to add the callback for.
   *
   * @param callback
   *    Callback to be called when the key is released.
   **/
  FORCEINLINE HEvent
  listenKeyUp(Key key, KeyUpCallback callback) const {
    return KeyUpCallbacks.at(key).connect(callback);
  }

  /**
   *   Adds a callback for a key pressed event.
   *
   * @param key
   *    Key to add the callback for.
   *
   * @param callback
   *    Callback to be called when the key is pressed.
   **/
  FORCEINLINE HEvent
  listenKeyPressed(Key key, KeyPressedCallback callback) const {
    return KeyPressedCallbacks.at(key).connect(callback);
  }

  FORCEINLINE HEvent
  listenAnyKeyPressed(KeyPressedCallback callback) const {
    return KeyPressedCallbacks.at(Key::KeysMax).connect(callback);
  }

  /**
   *   True if the key is down in the current frame.
   **/
  NODISCARD FORCEINLINE bool
  isKeyDown(Key key) const {
    CH_ASSERT(key < Key::KeysMax);
    return m_currentKeyboardState.test(static_cast<SIZE_T>(key));
  }


  NODISCARD FORCEINLINE bool
  areKeysDown(std::initializer_list<Key> keys) const {
    for (const auto& key : keys) {
      if (!m_currentKeyboardState.test(static_cast<SIZE_T>(key))) {
        return false;
      }
    }
    return true;
  }

  FORCEINLINE HEvent
  listenMouseButtonDown(MouseButton button, Function<void(const MouseButtonData&)> callback) const {
    return MouseButtonDownCallbacks.at(button).connect(callback);
  }

  FORCEINLINE HEvent
  listenMouseButtonUp(MouseButton button, Function<void(const MouseButtonData&)> callback) const {
    return MouseButtonUpCallbacks.at(button).connect(callback);
  }

  FORCEINLINE HEvent
  listenMouseButtonPressed(MouseButton /*button*/, Function<void(const MouseButtonData&)> /*callback*/) const {
    //return MouseButtonPressedCallbacks.at(button).connect(callback);
    CH_LOG_ERROR(InputSystem, "Mouse button pressed events are not supported yet.");
    return HEvent();
  }

  NODISCARD FORCEINLINE bool
  isMouseButtonDown(MouseButton button) const {
    CH_ASSERT(button < MouseButton::MouseButtonsMax);
    return m_currentMouseState.test(static_cast<SIZE_T>(button));
  }

 public:
  const Event<void(const MouseMoveData &data)> OnMouseMove;
  const Event<void()> OnClose;
  const Event<void(uint32,uint32)> OnResize;
  const Event<void(const KeyBoardData&)> OnKeyDown;
  const Event<void(const KeyBoardData&)> OnKeyUp;
  const Event<void(const KeyBoardData&)> OnKeyPressed;
  const Event<void(const MouseButtonData &data)> OnMouseButtonDown;
  const Event<void(const MouseButtonData &data)> OnMouseButtonUp;
  //const Event<void(const MouseButtonData &data)> OnMouseButtonPressed;
  const Event<void(const MouseWheelData &data)> OnMouseWheel;

private:
  friend class DisplayEventHandle;

  void
  dispatchKeyboardEvent(const KeyBoardData& keyData);

  void
  dispatchMouseButtonEvent(const MouseButtonData& mouseData);

  KeyboardBitSet m_currentKeyboardState;
  MouseBitSet m_currentMouseState;

  UnorderedMap<Key, Event<void(const KeyBoardData&)>> KeyUpCallbacks;
  UnorderedMap<Key, Event<void(const KeyBoardData&)>> KeyPressedCallbacks;
  UnorderedMap<Key, Event<void(const KeyBoardData&)>> KeyDownCallbacks;

  UnorderedMap<MouseButton, Event<void(const MouseButtonData&)>> MouseButtonUpCallbacks;
  //UnorderedMap<MouseButton, Event<void(const MouseButtonData&)>> MouseButtonPressedCallbacks;
  UnorderedMap<MouseButton, Event<void(const MouseButtonData&)>> MouseButtonDownCallbacks;

};
} // namespace chEngineSDK
