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
#include "chKeyBoard.h"
#include "chDisplayEvent.h"

namespace chEngineSDK {
using CloseCallback = std::function<void()>;
using ResizeCallback = std::function<void(uint32,uint32)>;
using chKeyBoard::Key;
using KeyDownCallback = std::function<void()>;
using KeyUpCallback = std::function<void()>;
using KeyPressedCallback = std::function<void()>;
using OnMouseMoveCallback = std::function<void(const MouseMoveData&)>;

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

  /** 
   *   True if the key is down in the current frame.
   **/
  NODISCARD FORCEINLINE bool
  isKeyDown(Key key) const {
    return m_currentKeyboardState.test(static_cast<SIZE_T>(key));
  }

  /** 
   *   True if the key was down in the previous frame.
   **/
  NODISCARD FORCEINLINE bool
  wasKeyDown(Key key) const {
    return m_previousKeyboardState.test(static_cast<SIZE_T>(key));
  }

  /** 
   *   True if the key is up in the current frame.
   **/
  NODISCARD FORCEINLINE bool
  isKeyJustPressed(Key key) const {
    return m_currentKeyboardState.test(static_cast<SIZE_T>(key)) && 
          !m_previousKeyboardState.test(static_cast<SIZE_T>(key));
  }

  /** 
   *   True if the key was up in the previous frame.
   **/
  NODISCARD FORCEINLINE bool
  isKeyJustReleased(Key key) const {
    return !m_currentKeyboardState.test(static_cast<SIZE_T>(key)) && 
            m_previousKeyboardState.test(static_cast<SIZE_T>(key));
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

  /*
   *  Update the keyboard state to the current frame.
  */
  FORCEINLINE void
  updateKeyboardState() {
    m_previousKeyboardState = m_currentKeyboardState;
  }

 public:
  const Event<void(const MouseMoveData &data)> OnMouseMove;
  const Event<void()> OnClose;
  const Event<void(uint32,uint32)> OnResize;

private:
  void
  dispatchKeyEvent(const PLATFORM_EVENT_TYPE& type, const Key& key);
  
  BitSet<static_cast<SIZE_T>(Key::KeysMax)> m_currentKeyboardState;
  BitSet<static_cast<SIZE_T>(Key::KeysMax)> m_previousKeyboardState;

  UnorderedMap<Key, Event<void()>> KeyUpCallbacks;
  UnorderedMap<Key, Event<void()>> KeyPressedCallbacks;
  UnorderedMap<Key, Event<void()>> KeyDownCallbacks;
};
} // namespace chEngineSDK