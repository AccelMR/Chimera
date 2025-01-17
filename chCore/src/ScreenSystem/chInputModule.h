/************************************************************************/
/**
 * @file chScreenEventDispatcher.h
 * @author AccelMR
 * @date 2022/09/12
 * 
 *    Input system to handle events from system and call events for each key, if there's an action.
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

namespace chEngineSDK {
using chKeyBoard::Key;
using KeyDownCallback = std::function<void()>;
using KeyUpCallback = std::function<void()>;
using KeyPressedCallback = std::function<void()>;
using OnMouseMoveCallback = std::function<void(const MouseMoveData&)>;

class CH_CORE_EXPORT InputModule: public Module<InputModule>
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

#pragma region ListenersKeyDown
  HEvent
  listenAKeyDown(KeyDownCallback func) const;

  HEvent
  listenBKeyDown(KeyDownCallback func) const;

  HEvent
  listenCKeyDown(KeyDownCallback func) const;

  HEvent
  listenDKeyDown(KeyDownCallback func) const;

  HEvent
  listenEKeyDown(KeyDownCallback func) const;

  HEvent
  listenFKeyDown(KeyDownCallback func) const;

  HEvent
  listenGKeyDown(KeyDownCallback func) const;

  HEvent
  listenHKeyDown(KeyDownCallback func) const;

  HEvent
  listenIKeyDown(KeyDownCallback func) const;

  HEvent
  listenJKeyDown(KeyDownCallback func) const;

  HEvent
  listenKKeyDown(KeyDownCallback func) const;

  HEvent
  listenLKeyDown(KeyDownCallback func) const;

  HEvent
  listenMKeyDown(KeyDownCallback func) const;

  HEvent
  listenNKeyDown(KeyDownCallback func) const;

  HEvent
  listenOKeyDown(KeyDownCallback func) const;

  HEvent
  listenPKeyDown(KeyDownCallback func) const;

  HEvent
  listenQKeyDown(KeyDownCallback func) const;

  HEvent
  listenRKeyDown(KeyDownCallback func) const;

  HEvent
  listenSKeyDown(KeyDownCallback func) const;

  HEvent
  listenTKeyDown(KeyDownCallback func) const;

  HEvent
  listenUKeyDown(KeyDownCallback func) const;

  HEvent
  listenVKeyDown(KeyDownCallback func) const;

  HEvent
  listenWKeyDown(KeyDownCallback func) const;

  HEvent
  listenXKeyDown(KeyDownCallback func) const;

  HEvent
  listenYKeyDown(KeyDownCallback func) const;

  HEvent
  listenZKeyDown(KeyDownCallback func) const;
#pragma endregion ListenersKeyDown
  
#pragma region ListenersKeyUp
  HEvent
  listenAKeyUp(KeyUpCallback func) const;

  HEvent
  listenBKeyUp(KeyUpCallback func) const;

  HEvent
  listenCKeyUp(KeyUpCallback func) const;

  HEvent
  listenDKeyUp(KeyUpCallback func) const;

  HEvent
  listenEKeyUp(KeyUpCallback func) const;

  HEvent
  listenFKeyUp(KeyUpCallback func) const;

  HEvent
  listenGKeyUp(KeyUpCallback func) const;

  HEvent
  listenHKeyUp(KeyUpCallback func) const;

  HEvent
  listenIKeyUp(KeyUpCallback func) const;

  HEvent
  listenJKeyUp(KeyUpCallback func) const;

  HEvent
  listenKKeyUp(KeyUpCallback func) const;

  HEvent
  listenLKeyUp(KeyUpCallback func) const;

  HEvent
  listenMKeyUp(KeyUpCallback func) const;

  HEvent
  listenNKeyUp(KeyUpCallback func) const;

  HEvent
  listenOKeyUp(KeyUpCallback func) const;

  HEvent
  listenPKeyUp(KeyUpCallback func) const;

  HEvent
  listenQKeyUp(KeyUpCallback func) const;

  HEvent
  listenRKeyUp(KeyUpCallback func) const;

  HEvent
  listenSKeyUp(KeyUpCallback func) const;

  HEvent
  listenTKeyUp(KeyUpCallback func) const;

  HEvent
  listenUKeyUp(KeyUpCallback func) const;

  HEvent
  listenVKeyUp(KeyUpCallback func) const;

  HEvent
  listenWKeyUp(KeyUpCallback func) const;

  HEvent
  listenXKeyUp(KeyUpCallback func) const;

  HEvent
  listenYKeyUp(KeyUpCallback func) const;

  HEvent
  listenZKeyUp(KeyUpCallback func) const;
#pragma endregion ListenersKeyUp

#pragma region ListenersKeyPressed
  HEvent
  listenAKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenBKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenCKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenDKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenEKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenFKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenGKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenHKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenIKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenJKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenKKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenLKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenMKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenNKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenOKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenPKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenQKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenRKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenSKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenTKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenUKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenVKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenWKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenXKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenYKeyPressed(KeyPressedCallback func) const;

  HEvent
  listenZKeyPressed(KeyPressedCallback func) const;
#pragma endregion ListenersKeyPressed

  HEvent
  listenOnMouseMove(OnMouseMoveCallback func) const;

 private:

 void
 _keyDownCallback(const KeyBoardData& kbData);

 void
 _keyUpCallback(const KeyBoardData& kbData);

 private:
  const Event<void(const MouseMoveData& data)> OnMouseMove;

  const Event<void()> OnADown;
  const Event<void()> OnBDown;
  const Event<void()> OnCDown;
  const Event<void()> OnDDown;
  const Event<void()> OnEDown;
  const Event<void()> OnFDown;
  const Event<void()> OnGDown;
  const Event<void()> OnHDown;
  const Event<void()> OnIDown;
  const Event<void()> OnJDown;
  const Event<void()> OnKDown;
  const Event<void()> OnLDown;
  const Event<void()> OnMDown;
  const Event<void()> OnNDown;
  const Event<void()> OnODown;
  const Event<void()> OnPDown;
  const Event<void()> OnQDown;
  const Event<void()> OnRDown;
  const Event<void()> OnSDown;
  const Event<void()> OnTDown;
  const Event<void()> OnUDown;
  const Event<void()> OnVDown;
  const Event<void()> OnWDown;
  const Event<void()> OnXDown;
  const Event<void()> OnYDown;
  const Event<void()> OnZDown;

  const Event<void()> OnAPressed;
  const Event<void()> OnBPressed;
  const Event<void()> OnCPressed;
  const Event<void()> OnDPressed;
  const Event<void()> OnEPressed;
  const Event<void()> OnFPressed;
  const Event<void()> OnGPressed;
  const Event<void()> OnHPressed;
  const Event<void()> OnIPressed;
  const Event<void()> OnJPressed;
  const Event<void()> OnKPressed;
  const Event<void()> OnLPressed;
  const Event<void()> OnMPressed;
  const Event<void()> OnNPressed;
  const Event<void()> OnOPressed;
  const Event<void()> OnPPressed;
  const Event<void()> OnQPressed;
  const Event<void()> OnRPressed;
  const Event<void()> OnSPressed;
  const Event<void()> OnTPressed;
  const Event<void()> OnUPressed;
  const Event<void()> OnVPressed;
  const Event<void()> OnWPressed;
  const Event<void()> OnXPressed;
  const Event<void()> OnYPressed;
  const Event<void()> OnZPressed;

  const Event<void()> OnAUp;
  const Event<void()> OnBUp;
  const Event<void()> OnCUp;
  const Event<void()> OnDUp;
  const Event<void()> OnEUp;
  const Event<void()> OnFUp;
  const Event<void()> OnGUp;
  const Event<void()> OnHUp;
  const Event<void()> OnIUp;
  const Event<void()> OnJUp;
  const Event<void()> OnKUp;
  const Event<void()> OnLUp;
  const Event<void()> OnMUp;
  const Event<void()> OnNUp;
  const Event<void()> OnOUp;
  const Event<void()> OnPUp;
  const Event<void()> OnQUp;
  const Event<void()> OnRUp;
  const Event<void()> OnSUp;
  const Event<void()> OnTUp;
  const Event<void()> OnUUp;
  const Event<void()> OnVUp;
  const Event<void()> OnWUp;
  const Event<void()> OnXUp;
  const Event<void()> OnYUp;
  const Event<void()> OnZUp;


  Map<Key, bool> KeyboardState = {
      { Key::Escape, false},
      { Key::Num0, false},
      { Key::Num1, false},
      { Key::Num2, false},
      { Key::Num3, false},
      { Key::Num4, false},
      { Key::Num5, false},
      { Key::Num6, false},
      { Key::Num7, false},
      { Key::Num8, false},
      { Key::Num9, false},
      { Key::A, false},
      { Key::B, false},
      { Key::C, false},
      { Key::D, false},
      { Key::E, false},
      { Key::F, false},
      { Key::G, false},
      { Key::H, false},
      { Key::I, false},
      { Key::J, false},
      { Key::K, false},
      { Key::L, false},
      { Key::M, false},
      { Key::N, false},
      { Key::O, false},
      { Key::P, false},
      { Key::Q, false},
      { Key::R, false},
      { Key::S, false},
      { Key::T, false},
      { Key::U, false},
      { Key::V, false},
      { Key::W, false},
      { Key::X, false},
      { Key::Y, false},
      { Key::Z, false},
      { Key::Minus, false},
      { Key::Add, false},
      { Key::Multiply, false},
      { Key::Divide, false},
      { Key::Back, false},
      { Key::Enter, false},
      { Key::Del, false},
      { Key::Tab, false},
      { Key::Numpad0, false},
      { Key::Numpad1, false},
      { Key::Numpad2, false},
      { Key::Numpad3, false},
      { Key::Numpad4, false},
      { Key::Numpad5, false},
      { Key::Numpad6, false},
      { Key::Numpad7, false},
      { Key::Numpad8, false},
      { Key::Numpad9, false},
      { Key::Up, false},
      { Key::Left, false},
      { Key::Down, false},
      { Key::Right, false},
      { Key::Space, false},
      { Key::Home, false},
      { Key::F1, false},
      { Key::F2, false},
      { Key::F3, false},
      { Key::F4, false},
      { Key::F5, false},
      { Key::F6, false},
      { Key::F7, false},
      { Key::F8, false},
      { Key::F9, false},
      { Key::F10, false},
      { Key::F11, false},
      { Key::F12, false},
      { Key::LShift, false},
      { Key::LControl, false},
      { Key::LAlt, false},
      { Key::Period, false},
      { Key::Comma, false},
      { Key::Semicolon, false},
      { Key::Backslash, false},
      { Key::Grave, false},
      { Key::LBracket, false},
      { Key::RBracket, false},
      { Key::Apostrophe, false},
      { Key::KeysMax, false} };

};
}
