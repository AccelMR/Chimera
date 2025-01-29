/************************************************************************/
/**
 * @file chScreenEventDispatcherModule.cpp
 * @author AccelMR
 * @date 2022/09/12
 *
 *    Input system to handle events from system and call events for each key, if there's an action.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chInputModule.h"

#include "chDebug.h"

#include "chScreenEvent.h"
#include "chScreenEventHandle.h"
#include "chStringUtils.h"

namespace chEngineSDK {
using chKeyBoard::Key;

/*
*/
void
InputModule::_keyDownCallback(const KeyBoardData& kbData) {
  switch (kbData.key)
  {
  default:
    break;
  case chKeyBoard::Key::Escape:
    break;
  case chKeyBoard::Key::Num0:
    break;
  case chKeyBoard::Key::Num1:
    break;
  case chKeyBoard::Key::Num2:
    break;
  case chKeyBoard::Key::Num3:
    break;
  case chKeyBoard::Key::Num4:
    break;
  case chKeyBoard::Key::Num5:
    break;
  case chKeyBoard::Key::Num6:
    break;
  case chKeyBoard::Key::Num7:
    break;
  case chKeyBoard::Key::Num8:
    break;
  case chKeyBoard::Key::Num9:
    break;
  case Key::A:
    if(!KeyboardState.find(Key::A)->second) {
      OnADown();
      KeyboardState.find(Key::A)->second = true;
    }
    else {
      OnAPressed();
    }
    break;
  case chKeyBoard::Key::B:
    if(!KeyboardState.find(Key::B)->second) {
      OnBDown();
      KeyboardState.find(Key::B)->second = true;
    }
    else {
      OnBPressed();
    }
    break;
  case chKeyBoard::Key::C:
    if ( !KeyboardState.find(Key::C)->second ) {
      OnCDown();
      KeyboardState.find(Key::C)->second = true;
    }
    else {
      OnCPressed();
    }
    break;
  case chKeyBoard::Key::D:
    if(!KeyboardState.find(Key::D)->second) {
      OnDDown();
      KeyboardState.find(Key::D)->second = true;
    }
    else {
      OnDPressed();
    }
    break;
  case chKeyBoard::Key::E:
    if(!KeyboardState.find(Key::E)->second) {
      OnEDown();
      KeyboardState.find(Key::E)->second = true;
    }
    else {
      OnEPressed();
    }
    break;
  case chKeyBoard::Key::F:
    if(!KeyboardState.find(Key::F)->second) {
      OnFDown();
      KeyboardState.find(Key::F)->second = true;
    }
    else {
      OnFPressed();
    }
    break;
  case chKeyBoard::Key::G:
    if(!KeyboardState.find(Key::G)->second) {
      OnGDown();
      KeyboardState.find(Key::G)->second = true;
    }
    else {
      OnGPressed();
    }
    break;
  case chKeyBoard::Key::H:
    if(!KeyboardState.find(Key::H)->second) {
      OnHDown();
      KeyboardState.find(Key::H)->second = true;
    }
    else {
      OnHPressed();
    }
    break;
  case chKeyBoard::Key::I:
    if(!KeyboardState.find(Key::I)->second) {
      OnIDown();
      KeyboardState.find(Key::I)->second = true;
    }
    else {
      OnIPressed();
    }
    break;
  case chKeyBoard::Key::J:
    if(!KeyboardState.find(Key::J)->second) {
      OnJDown();
      KeyboardState.find(Key::J)->second = true;
    }
    else {
      OnJPressed();
    }
    break;
  case chKeyBoard::Key::K:
    if(!KeyboardState.find(Key::K)->second) {
      OnKDown();
      KeyboardState.find(Key::K)->second = true;
    }
    else {
      OnKPressed();
    }
    break;
  case chKeyBoard::Key::L:
    if(!KeyboardState.find(Key::L)->second) {
      OnLDown();
      KeyboardState.find(Key::L)->second = true;
    }
    else {
      OnLPressed();
    }
    break;
  case chKeyBoard::Key::M:
    if ( !KeyboardState.find( Key::M )->second ) {
      OnMDown();
      KeyboardState.find( Key::M )->second = true;
    }
    else {
      OnMPressed();
    }
    break;
  case chKeyBoard::Key::N:
    if(!KeyboardState.find(Key::N)->second) {
      OnNDown();
      KeyboardState.find(Key::N)->second = true;
    }
    else {
      OnNPressed();
    }
    break;
  case chKeyBoard::Key::O:
    if ( !KeyboardState.find( Key::O )->second ) {
      OnODown();
      KeyboardState.find( Key::O )->second = true;
    }
    else {
      OnOPressed();
    }
    break;
  case chKeyBoard::Key::P:
    if(!KeyboardState.find(Key::P)->second) {
      OnPDown();
      KeyboardState.find(Key::P)->second = true;
    }
    else {
      OnPPressed();
    }
    break;
  case chKeyBoard::Key::Q:
    if(!KeyboardState.find(Key::Q)->second) {
      OnQDown();
      KeyboardState.find(Key::Q)->second = true;
    }
    else {
      OnQPressed();
    }
    break;
  case chKeyBoard::Key::R:
    if(!KeyboardState.find(Key::R)->second) {
      OnRDown();
      KeyboardState.find(Key::R)->second = true;
    }
    else {
      OnRPressed();
    }
    break;
  case chKeyBoard::Key::S:
    if(!KeyboardState.find(Key::S)->second) {
      OnSDown();
      KeyboardState.find(Key::S)->second = true;
    }
    else {
      OnSPressed();
    }
    break;
  case chKeyBoard::Key::T:
    if(!KeyboardState.find(Key::T)->second) {
      OnTDown();
      KeyboardState.find(Key::T)->second = true;
    }
    else {
      OnTPressed();
    }
    break;
  case chKeyBoard::Key::U:
    if(!KeyboardState.find(Key::U)->second) {
      OnUDown();
      KeyboardState.find(Key::U)->second = true;
    }
    else {
      OnUPressed();
    }
    break;
  case chKeyBoard::Key::V:
    if(!KeyboardState.find(Key::V)->second) {
      OnVDown();
      KeyboardState.find(Key::V)->second = true;
    }
    else {
      OnVPressed();
    }
    break;
  case chKeyBoard::Key::W:
    if(!KeyboardState.find(Key::W)->second) {
      OnWDown();
      KeyboardState.find(Key::W)->second = true;
    }
    else {
      OnWPressed();
    }
    break;
  case chKeyBoard::Key::X:
    if(!KeyboardState.find(Key::X)->second) {
      OnXDown();
      KeyboardState.find(Key::X)->second = true;
    }
    else {
      OnXPressed();
    }
    break;
  case chKeyBoard::Key::Y:
    if ( !KeyboardState.find(Key::Y)->second) {
      OnYDown();
      KeyboardState.find(Key::Y)->second = true;
    }
    else {
      OnZPressed();
    }
    break;
  case chKeyBoard::Key::Z:
    if(!KeyboardState.find(Key::Z)->second) {
      OnZDown();
      KeyboardState.find(Key::Z)->second = true;
    }
    else {
      OnZPressed();
    }
    break;
  case chKeyBoard::Key::Minus:
    break;
  case chKeyBoard::Key::Add:
    break;
  case chKeyBoard::Key::Multiply:
    break;
  case chKeyBoard::Key::Divide:
    break;
  case chKeyBoard::Key::Back:
    break;
  case chKeyBoard::Key::Enter:
    break;
  case chKeyBoard::Key::Del:
    break;
  case chKeyBoard::Key::Tab:
    break;
  case chKeyBoard::Key::Numpad0:
    break;
  case chKeyBoard::Key::Numpad1:
    break;
  case chKeyBoard::Key::Numpad2:
    break;
  case chKeyBoard::Key::Numpad3:
    break;
  case chKeyBoard::Key::Numpad4:
    break;
  case chKeyBoard::Key::Numpad5:
    break;
  case chKeyBoard::Key::Numpad6:
    break;
  case chKeyBoard::Key::Numpad7:
    break;
  case chKeyBoard::Key::Numpad8:
    break;
  case chKeyBoard::Key::Numpad9:
    break;
  case chKeyBoard::Key::Up:
    break;
  case chKeyBoard::Key::Left:
    break;
  case chKeyBoard::Key::Down:
    break;
  case chKeyBoard::Key::Right:
    break;
  case chKeyBoard::Key::Space:
    break;
  case chKeyBoard::Key::Home:
    break;
  case chKeyBoard::Key::F1:
    break;
  case chKeyBoard::Key::F2:
    break;
  case chKeyBoard::Key::F3:
    break;
  case chKeyBoard::Key::F4:
    break;
  case chKeyBoard::Key::F5:
    break;
  case chKeyBoard::Key::F6:
    break;
  case chKeyBoard::Key::F7:
    break;
  case chKeyBoard::Key::F8:
    break;
  case chKeyBoard::Key::F9:
    break;
  case chKeyBoard::Key::F10:
    break;
  case chKeyBoard::Key::F11:
    break;
  case chKeyBoard::Key::F12:
    break;
  case chKeyBoard::Key::LShift:
    break;
  case chKeyBoard::Key::LControl:
    break;
  case chKeyBoard::Key::LAlt:
    break;
  case chKeyBoard::Key::Period:
    break;
  case chKeyBoard::Key::Comma:
    break;
  case chKeyBoard::Key::Semicolon:
    break;
  case chKeyBoard::Key::Backslash:
    break;
  case chKeyBoard::Key::Grave:
    break;
  case chKeyBoard::Key::LBracket:
    break;
  case chKeyBoard::Key::RBracket:
    break;
  case chKeyBoard::Key::Apostrophe:
    break;
  case chKeyBoard::Key::KeysMax:
    break;
  }
}

/*
*/

void
InputModule::_keyUpCallback(const KeyBoardData& kbData) {
  switch (kbData.key)
  {
  default:
    break;
  case chKeyBoard::Key::Escape:
    break;
  case chKeyBoard::Key::Num0:
    break;
  case chKeyBoard::Key::Num1:
    break;
  case chKeyBoard::Key::Num2:
    break;
  case chKeyBoard::Key::Num3:
    break;
  case chKeyBoard::Key::Num4:
    break;
  case chKeyBoard::Key::Num5:
    break;
  case chKeyBoard::Key::Num6:
    break;
  case chKeyBoard::Key::Num7:
    break;
  case chKeyBoard::Key::Num8:
    break;
  case chKeyBoard::Key::Num9:
    break;
  case Key::A:
      OnAUp();
      KeyboardState.find(Key::A)->second = false;
    break;
  case chKeyBoard::Key::B:
      OnBUp();
      KeyboardState.find(Key::B)->second = false;
    break;
  case chKeyBoard::Key::C:
      OnCUp();
      KeyboardState.find(Key::C)->second = false;
    break;
  case chKeyBoard::Key::D:
      OnDUp();
      KeyboardState.find(Key::D)->second = false;
    break;
  case chKeyBoard::Key::E:
      OnEUp();
      KeyboardState.find(Key::E)->second = false;
  case chKeyBoard::Key::F:
      OnFUp();
      KeyboardState.find(Key::F)->second = false;
      break;
  case chKeyBoard::Key::G:
      OnGUp();
      KeyboardState.find(Key::G)->second = false;
      break;
  case chKeyBoard::Key::H:
      OnHUp();
      KeyboardState.find(Key::H)->second = false;
      break;
  case chKeyBoard::Key::I:
      OnIUp();
      KeyboardState.find(Key::I)->second = false;
      break;
  case chKeyBoard::Key::J:
      OnJUp();
      KeyboardState.find(Key::J)->second = false;
      break;
  case chKeyBoard::Key::K:
      OnKUp();
      KeyboardState.find(Key::K)->second = false;
      break;
  case chKeyBoard::Key::L:
      OnLUp();
      KeyboardState.find(Key::L)->second = false;
      break;
  case chKeyBoard::Key::M:
      OnMUp();
      KeyboardState.find( Key::M )->second = false;
  case chKeyBoard::Key::N:
      OnNUp();
      KeyboardState.find(Key::N)->second = false;
      break;
  case chKeyBoard::Key::O:
      OnOUp();
      KeyboardState.find( Key::O )->second = false;
  case chKeyBoard::Key::P:
      OnPUp();
      KeyboardState.find(Key::P)->second = false;
      break;
  case chKeyBoard::Key::Q:
      OnQUp();
      KeyboardState.find(Key::Q)->second = false;
      break;
  case chKeyBoard::Key::R:
      OnRUp();
      KeyboardState.find(Key::R)->second = false;
      break;
  case chKeyBoard::Key::S:
      OnSUp();
      KeyboardState.find(Key::S)->second = false;
      break;
  case chKeyBoard::Key::T:
      OnTUp();
      KeyboardState.find(Key::T)->second = false;
      break;
  case chKeyBoard::Key::U:
      OnUUp();
      KeyboardState.find(Key::U)->second = false;
      break;
  case chKeyBoard::Key::V:
      OnVUp();
      KeyboardState.find(Key::V)->second = false;
      break;
  case chKeyBoard::Key::W:
      OnWUp();
      KeyboardState.find(Key::W)->second = false;
      break;
  case chKeyBoard::Key::X:
      OnXUp();
      KeyboardState.find(Key::X)->second = false;
      break;
  case chKeyBoard::Key::Y:
      OnYUp();
      KeyboardState.find(Key::Y)->second = false;
      break;
  case chKeyBoard::Key::Z:
      OnZUp();
      KeyboardState.find(Key::Z)->second = false;
      break;
  case chKeyBoard::Key::Minus:
    break;
  case chKeyBoard::Key::Add:
    break;
  case chKeyBoard::Key::Multiply:
    break;
  case chKeyBoard::Key::Divide:
    break;
  case chKeyBoard::Key::Back:
    break;
  case chKeyBoard::Key::Enter:
    break;
  case chKeyBoard::Key::Del:
    break;
  case chKeyBoard::Key::Tab:
    break;
  case chKeyBoard::Key::Numpad0:
    break;
  case chKeyBoard::Key::Numpad1:
    break;
  case chKeyBoard::Key::Numpad2:
    break;
  case chKeyBoard::Key::Numpad3:
    break;
  case chKeyBoard::Key::Numpad4:
    break;
  case chKeyBoard::Key::Numpad5:
    break;
  case chKeyBoard::Key::Numpad6:
    break;
  case chKeyBoard::Key::Numpad7:
    break;
  case chKeyBoard::Key::Numpad8:
    break;
  case chKeyBoard::Key::Numpad9:
    break;
  case chKeyBoard::Key::Up:
    break;
  case chKeyBoard::Key::Left:
    break;
  case chKeyBoard::Key::Down:
    break;
  case chKeyBoard::Key::Right:
    break;
  case chKeyBoard::Key::Space:
    break;
  case chKeyBoard::Key::Home:
    break;
  case chKeyBoard::Key::F1:
    break;
  case chKeyBoard::Key::F2:
    break;
  case chKeyBoard::Key::F3:
    break;
  case chKeyBoard::Key::F4:
    break;
  case chKeyBoard::Key::F5:
    break;
  case chKeyBoard::Key::F6:
    break;
  case chKeyBoard::Key::F7:
    break;
  case chKeyBoard::Key::F8:
    break;
  case chKeyBoard::Key::F9:
    break;
  case chKeyBoard::Key::F10:
    break;
  case chKeyBoard::Key::F11:
    break;
  case chKeyBoard::Key::F12:
    break;
  case chKeyBoard::Key::LShift:
    break;
  case chKeyBoard::Key::LControl:
    break;
  case chKeyBoard::Key::LAlt:
    break;
  case chKeyBoard::Key::Period:
    break;
  case chKeyBoard::Key::Comma:
    break;
  case chKeyBoard::Key::Semicolon:
    break;
  case chKeyBoard::Key::Backslash:
    break;
  case chKeyBoard::Key::Grave:
    break;
  case chKeyBoard::Key::LBracket:
    break;
  case chKeyBoard::Key::RBracket:
    break;
  case chKeyBoard::Key::Apostrophe:
    break;
  case chKeyBoard::Key::KeysMax:
    break;
  }
}

/*
*/
NODISCARD bool
InputModule::dispatchInputEvents(const ScreenEvent& sEvent) {
  switch (sEvent.getType())
  {
  case PLATFORM_EVENT_TYPE::kMOUSE_MOVE:
    OnMouseMove(sEvent.getData().mouseMoveData);
    break;

  case PLATFORM_EVENT_TYPE::kKEY_DOWN:
    _keyDownCallback(sEvent.getData().keyBoardData);
    break;

  case PLATFORM_EVENT_TYPE::kKEY_UP:
    _keyUpCallback(sEvent.getData().keyBoardData);
    break;

  case PLATFORM_EVENT_TYPE::kNONE:
  default:
    return false;
    break;

  }
  return true;
}

/*
*/
void
InputModule::dispatchEvents(const SPtr<ScreenEventHandle>& eventHandler) {
  while (!eventHandler->isInputEventQueueEmpty()) {
    ScreenEvent& screenEvent = eventHandler->frontPopInputEventQueue();
    if (!dispatchInputEvents(screenEvent)) {
      CH_LOG_ERROR(StringUtils::format("Could not dispatch event : [{0}]  in Input Queue.", screenEvent.getTypeAsString()));
    }
  }
}
#pragma region ListenersKeyDown
/*
*/
HEvent
InputModule::listenAKeyDown(KeyDownCallback func) const{
  return OnADown.connect(func);
}

/*
*/
HEvent
InputModule::listenBKeyDown( KeyDownCallback func ) const {
  return OnBDown.connect(func);
}

/*
*/
HEvent
InputModule::listenCKeyDown(KeyDownCallback func) const {
  return OnCDown.connect(func);
}

/*
*/
HEvent
InputModule::listenDKeyDown(KeyDownCallback func) const {
  return OnDDown.connect(func);
}

/*
*/
HEvent
InputModule::listenEKeyDown(KeyDownCallback func) const {
  return OnEDown.connect(func);
}

/*
*/
HEvent
InputModule::listenFKeyDown(KeyDownCallback func) const {
  return OnFDown.connect(func);
}

/*
*/
HEvent
InputModule::listenGKeyDown(KeyDownCallback func) const {
  return OnGDown.connect(func);
}

/*
*/
HEvent
InputModule::listenHKeyDown(KeyDownCallback func) const {
  return OnHDown.connect(func);
}

/*
*/
HEvent
InputModule::listenIKeyDown(KeyDownCallback func) const {
  return OnIDown.connect(func);
}

/*
*/
HEvent
InputModule::listenJKeyDown(KeyDownCallback func) const {
  return OnJDown.connect(func);
}

/*
*/
HEvent
InputModule::listenKKeyDown(KeyDownCallback func) const {
  return OnKDown.connect(func);
}

/*
*/
HEvent
InputModule::listenLKeyDown(KeyDownCallback func) const {
  return OnLDown.connect(func);
}

/*
*/
HEvent
InputModule::listenMKeyDown(KeyDownCallback func) const {
  return OnMDown.connect(func);
}

/*
*/
HEvent
InputModule::listenNKeyDown(KeyDownCallback func) const {
  return OnNDown.connect(func);
}

/*
*/
HEvent
InputModule::listenOKeyDown(KeyDownCallback func) const {
  return OnODown.connect(func);
}

/*
*/
HEvent
InputModule::listenPKeyDown(KeyDownCallback func) const {
  return OnPDown.connect(func);
}

/*
*/
HEvent
InputModule::listenQKeyDown(KeyDownCallback func) const {
  return OnQDown.connect(func);
}

/*
*/
HEvent
InputModule::listenRKeyDown(KeyDownCallback func) const {
  return OnRDown.connect(func);
}

/*
*/
HEvent
InputModule::listenSKeyDown(KeyDownCallback func) const {
  return OnSDown.connect(func);
}

/*
*/
HEvent
InputModule::listenTKeyDown(KeyDownCallback func) const {
  return OnTDown.connect(func);
}

/*
*/
HEvent
InputModule::listenUKeyDown(KeyDownCallback func) const {
  return OnUDown.connect(func);
}

/*
*/
HEvent
InputModule::listenVKeyDown(KeyDownCallback func) const {
  return OnVDown.connect(func);
}

/*
*/
HEvent
InputModule::listenWKeyDown(KeyDownCallback func) const {
  return OnWDown.connect(func);
}

/*
*/
HEvent
InputModule::listenXKeyDown(KeyDownCallback func) const {
  return OnXDown.connect(func);
}

/*
*/
HEvent
InputModule::listenYKeyDown(KeyDownCallback func) const {
  return OnYDown.connect(func);
}

/*
*/
HEvent
InputModule::listenZKeyDown(KeyDownCallback func) const {
  return OnZDown.connect(func);
}
#pragma endregion ListenersKeyDown

#pragma region ListenersKeyUp
/*
*/
HEvent
InputModule::listenAKeyUp(KeyUpCallback func) const{
  return OnAUp.connect(func);
}

/*
*/
HEvent
InputModule::listenBKeyUp( KeyUpCallback func ) const {
  return OnBUp.connect(func);
}

/*
*/
HEvent
InputModule::listenCKeyUp(KeyUpCallback func) const {
  return OnCUp.connect(func);
}

/*
*/
HEvent
InputModule::listenDKeyUp(KeyUpCallback func) const {
  return OnDUp.connect(func);
}

/*
*/
HEvent
InputModule::listenEKeyUp(KeyUpCallback func) const {
  return OnEUp.connect(func);
}

/*
*/
HEvent
InputModule::listenFKeyUp(KeyUpCallback func) const {
  return OnFUp.connect(func);
}

/*
*/
HEvent
InputModule::listenGKeyUp(KeyUpCallback func) const {
  return OnGUp.connect(func);
}

/*
*/
HEvent
InputModule::listenHKeyUp(KeyUpCallback func) const {
  return OnHUp.connect(func);
}

/*
*/
HEvent
InputModule::listenIKeyUp(KeyUpCallback func) const {
  return OnIUp.connect(func);
}

/*
*/
HEvent
InputModule::listenJKeyUp(KeyUpCallback func) const {
  return OnJUp.connect(func);
}

/*
*/
HEvent
InputModule::listenKKeyUp(KeyUpCallback func) const {
  return OnKUp.connect(func);
}

/*
*/
HEvent
InputModule::listenLKeyUp(KeyUpCallback func) const {
  return OnLUp.connect(func);
}

/*
*/
HEvent
InputModule::listenMKeyUp(KeyUpCallback func) const {
  return OnMUp.connect(func);
}

/*
*/
HEvent
InputModule::listenNKeyUp(KeyUpCallback func) const {
  return OnNUp.connect(func);
}

/*
*/
HEvent
InputModule::listenOKeyUp(KeyUpCallback func) const {
  return OnOUp.connect(func);
}

/*
*/
HEvent
InputModule::listenPKeyUp(KeyUpCallback func) const {
  return OnPUp.connect(func);
}

/*
*/
HEvent
InputModule::listenQKeyUp(KeyUpCallback func) const {
  return OnQUp.connect(func);
}

/*
*/
HEvent
InputModule::listenRKeyUp(KeyUpCallback func) const {
  return OnRUp.connect(func);
}

/*
*/
HEvent
InputModule::listenSKeyUp(KeyUpCallback func) const {
  return OnSUp.connect(func);
}

/*
*/
HEvent
InputModule::listenTKeyUp(KeyUpCallback func) const {
  return OnTUp.connect(func);
}

/*
*/
HEvent
InputModule::listenUKeyUp(KeyUpCallback func) const {
  return OnUUp.connect(func);
}

/*
*/
HEvent
InputModule::listenVKeyUp(KeyUpCallback func) const {
  return OnVUp.connect(func);
}

/*
*/
HEvent
InputModule::listenWKeyUp(KeyUpCallback func) const {
  return OnWUp.connect(func);
}

/*
*/
HEvent
InputModule::listenXKeyUp(KeyUpCallback func) const {
  return OnXUp.connect(func);
}

/*
*/
HEvent
InputModule::listenYKeyUp(KeyUpCallback func) const {
  return OnYUp.connect(func);
}

/*
*/
HEvent
InputModule::listenZKeyUp(KeyUpCallback func) const {
  return OnZUp.connect(func);
}
#pragma endregion ListenersKeyUp

#pragma region ListenersKeyPressed
/*
*/
HEvent
InputModule::listenAKeyPressed(KeyPressedCallback func) const{
  return OnAPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenBKeyPressed( KeyPressedCallback func ) const {
  return OnBPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenCKeyPressed(KeyPressedCallback func) const {
  return OnCPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenDKeyPressed(KeyPressedCallback func) const {
  return OnDPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenEKeyPressed(KeyPressedCallback func) const {
  return OnEPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenFKeyPressed(KeyPressedCallback func) const {
  return OnFPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenGKeyPressed(KeyPressedCallback func) const {
  return OnGPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenHKeyPressed(KeyPressedCallback func) const {
  return OnHPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenIKeyPressed(KeyPressedCallback func) const {
  return OnIPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenJKeyPressed(KeyPressedCallback func) const {
  return OnJPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenKKeyPressed(KeyPressedCallback func) const {
  return OnKPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenLKeyPressed(KeyPressedCallback func) const {
  return OnLPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenMKeyPressed(KeyPressedCallback func) const {
  return OnMPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenNKeyPressed(KeyPressedCallback func) const {
  return OnNPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenOKeyPressed(KeyPressedCallback func) const {
  return OnOPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenPKeyPressed(KeyPressedCallback func) const {
  return OnPPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenQKeyPressed(KeyPressedCallback func) const {
  return OnQPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenRKeyPressed(KeyPressedCallback func) const {
  return OnRPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenSKeyPressed(KeyPressedCallback func) const {
  return OnSPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenTKeyPressed(KeyPressedCallback func) const {
  return OnTPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenUKeyPressed(KeyPressedCallback func) const {
  return OnUPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenVKeyPressed(KeyPressedCallback func) const {
  return OnVPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenWKeyPressed(KeyPressedCallback func) const {
  return OnWPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenXKeyPressed(KeyPressedCallback func) const {
  return OnXPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenYKeyPressed(KeyPressedCallback func) const {
  return OnYPressed.connect(func);
}

/*
*/
HEvent
InputModule::listenZKeyPressed(KeyPressedCallback func) const {
  return OnZPressed.connect(func);
}
#pragma endregion ListenersKeyPressed

/*
*/
HEvent
InputModule::listenOnMouseMove(OnMouseMoveCallback func) const {
  return OnMouseMove.connect(func);
}

}


