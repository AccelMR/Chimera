/************************************************************************/
/**
 * @file chDisplayEventHandleSDL.h
 * @author AccelMR
 * @date 2025/04/06
 * @brief SDL specific implementation of DisplaySurface event handle.
 * @details This file uses external library to handle events.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"
#include "chEventDispatcherManager.h"

#ifdef CH_CROSS_WINDOW
#include "CrossWindow/CrossWindow.h"

#if USING(CH_PLATFORM_LINUX)
#include "chXCBGlobals.h"
using namespace chEngineSDK::XCBGlobals;
#endif //USING(CH_PLATFORM_LINUX)

namespace chCrossWindowHelpers {
FORCEINLINE chEngineSDK::chKeyBoard::Key
translateKey(::xwin::Key key) {
  // This works because the enum values are the same in Chimera and CrossWindow.
  return static_cast<chEngineSDK::chKeyBoard::Key>(key);
}

FORCEINLINE chEngineSDK::KeyBoardState
translateKeyState(::xwin::ButtonState state) {
  switch (state) {
    case ::xwin::ButtonState::Pressed:
      return chEngineSDK::KeyBoardState::PRESSED;
    case ::xwin::ButtonState::Released:
      return chEngineSDK::KeyBoardState::RELEASED;
    default:
      return chEngineSDK::KeyBoardState::NONE;
  }
}

FORCEINLINE uint16_t
translateKeyModifier(::xwin::ModifierState modifier) {
  uint16_t result = 0;
  if (modifier.ctrl) {
    result |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LCTRL);
  }
  if (modifier.alt) {
    result |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LALT);
  }
  if (modifier.shift) {
    result |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LSHIFT);
  }
  if (modifier.meta) {
    result |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LMETA);
  }
  return result;
}

#if USING(CH_PLATFORM_LINUX)
static std::unordered_map<xcb_keycode_t, chEngineSDK::chKeyBoard::Key> activeKeys;

FORCEINLINE void
processActiveKeys(std::function<void(chEngineSDK::KeyBoardData)> addEvent) {
  auto& dispatcher = chEngineSDK::EventDispatcherManager::instance();

  uint16_t modifiers = 0;

  // Verificar el estado actual de los modificadores
  if (dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::LShift) ||
      dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::RShift)) {
      modifiers |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LSHIFT);
  }

  if (dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::LControl) ||
      dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::RControl)) {
      modifiers |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LCTRL);
  }

  if (dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::LAlt) ||
      dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::RAlt)) {
      modifiers |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LALT);
  }

  if (dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::LMod) ||
      dispatcher.isKeyDown(chEngineSDK::chKeyBoard::Key::RMod)) {
      modifiers |= static_cast<uint16_t>(chEngineSDK::KeyBoardModifier::LMETA);
  }

  for (const auto& [keycode, key] : activeKeys) {
      if (dispatcher.isKeyDown(key)) {
          chEngineSDK::KeyBoardData keyData;
          keyData.key = key;
          keyData.state = chEngineSDK::KeyBoardState::PRESSED;
          keyData.modifiers = modifiers;
          addEvent(keyData);
      }
  }
}

FORCEINLINE chEngineSDK::chKeyBoard::Key
translateXCBKey(xcb_keycode_t keycode, xcb_key_symbols_t* keysyms) {
  xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);

  switch (keysym) {
      case XK_Escape: return chEngineSDK::chKeyBoard::Key::Escape;

      // Números
      case XK_1: case XK_exclam: return chEngineSDK::chKeyBoard::Key::Num1;
      case XK_2: case XK_at: return chEngineSDK::chKeyBoard::Key::Num2;
      case XK_3: case XK_numbersign: return chEngineSDK::chKeyBoard::Key::Num3;
      case XK_4: case XK_dollar: return chEngineSDK::chKeyBoard::Key::Num4;
      case XK_5: case XK_percent: return chEngineSDK::chKeyBoard::Key::Num5;
      case XK_6: case XK_asciicircum: return chEngineSDK::chKeyBoard::Key::Num6;
      case XK_7: case XK_ampersand: return chEngineSDK::chKeyBoard::Key::Num7;
      case XK_8: case XK_asterisk: return chEngineSDK::chKeyBoard::Key::Num8;
      case XK_9: case XK_parenleft: return chEngineSDK::chKeyBoard::Key::Num9;
      case XK_0: case XK_parenright: return chEngineSDK::chKeyBoard::Key::Num0;

      // Símbolos
      case XK_minus: case XK_underscore: return chEngineSDK::chKeyBoard::Key::Minus;
      case XK_equal: case XK_plus: return chEngineSDK::chKeyBoard::Key::Equals;
      case XK_BackSpace: return chEngineSDK::chKeyBoard::Key::Back;
      case XK_Tab: return chEngineSDK::chKeyBoard::Key::Tab;

      // Letras
      case XK_q: case XK_Q: return chEngineSDK::chKeyBoard::Key::Q;
      case XK_w: case XK_W: return chEngineSDK::chKeyBoard::Key::W;
      case XK_e: case XK_E: return chEngineSDK::chKeyBoard::Key::E;
      case XK_r: case XK_R: return chEngineSDK::chKeyBoard::Key::R;
      case XK_t: case XK_T: return chEngineSDK::chKeyBoard::Key::T;
      case XK_y: case XK_Y: return chEngineSDK::chKeyBoard::Key::Y;
      case XK_u: case XK_U: return chEngineSDK::chKeyBoard::Key::U;
      case XK_i: case XK_I: return chEngineSDK::chKeyBoard::Key::I;
      case XK_o: case XK_O: return chEngineSDK::chKeyBoard::Key::O;
      case XK_p: case XK_P: return chEngineSDK::chKeyBoard::Key::P;
      case XK_bracketleft: case XK_braceleft: return chEngineSDK::chKeyBoard::Key::LBracket;
      case XK_bracketright: case XK_braceright: return chEngineSDK::chKeyBoard::Key::RBracket;
      case XK_Return: return chEngineSDK::chKeyBoard::Key::Enter;
      case XK_Control_L: return chEngineSDK::chKeyBoard::Key::LControl;

      case XK_a: case XK_A: return chEngineSDK::chKeyBoard::Key::A;
      case XK_s: case XK_S: return chEngineSDK::chKeyBoard::Key::S;
      case XK_d: case XK_D: return chEngineSDK::chKeyBoard::Key::D;
      case XK_f: case XK_F: return chEngineSDK::chKeyBoard::Key::F;
      case XK_g: case XK_G: return chEngineSDK::chKeyBoard::Key::G;
      case XK_h: case XK_H: return chEngineSDK::chKeyBoard::Key::H;
      case XK_j: case XK_J: return chEngineSDK::chKeyBoard::Key::J;
      case XK_k: case XK_K: return chEngineSDK::chKeyBoard::Key::K;
      case XK_l: case XK_L: return chEngineSDK::chKeyBoard::Key::L;

      case XK_semicolon: case XK_colon: return chEngineSDK::chKeyBoard::Key::Semicolon;
      case XK_apostrophe: case XK_quotedbl: return chEngineSDK::chKeyBoard::Key::Apostrophe;
      case XK_grave: case XK_asciitilde: return chEngineSDK::chKeyBoard::Key::Grave;
      case XK_Shift_L: return chEngineSDK::chKeyBoard::Key::LShift;
      case XK_backslash: case XK_bar: return chEngineSDK::chKeyBoard::Key::Backslash;

      case XK_z: case XK_Z: return chEngineSDK::chKeyBoard::Key::Z;
      case XK_x: case XK_X: return chEngineSDK::chKeyBoard::Key::X;
      case XK_c: case XK_C: return chEngineSDK::chKeyBoard::Key::C;
      case XK_v: case XK_V: return chEngineSDK::chKeyBoard::Key::V;
      case XK_b: case XK_B: return chEngineSDK::chKeyBoard::Key::B;
      case XK_n: case XK_N: return chEngineSDK::chKeyBoard::Key::N;
      case XK_m: case XK_M: return chEngineSDK::chKeyBoard::Key::M;

      case XK_comma: case XK_less: return chEngineSDK::chKeyBoard::Key::Comma;
      case XK_period: case XK_greater: return chEngineSDK::chKeyBoard::Key::Period;
      case XK_slash: case XK_question: return chEngineSDK::chKeyBoard::Key::Slash;
      case XK_Shift_R: return chEngineSDK::chKeyBoard::Key::RShift;
      case XK_KP_Multiply: return chEngineSDK::chKeyBoard::Key::Multiply;
      case XK_Alt_L: return chEngineSDK::chKeyBoard::Key::LAlt;
      case XK_space: return chEngineSDK::chKeyBoard::Key::Space;
      case XK_Caps_Lock: return chEngineSDK::chKeyBoard::Key::Capital;

      // Teclas de función
      case XK_F1: return chEngineSDK::chKeyBoard::Key::F1;
      case XK_F2: return chEngineSDK::chKeyBoard::Key::F2;
      case XK_F3: return chEngineSDK::chKeyBoard::Key::F3;
      case XK_F4: return chEngineSDK::chKeyBoard::Key::F4;
      case XK_F5: return chEngineSDK::chKeyBoard::Key::F5;
      case XK_F6: return chEngineSDK::chKeyBoard::Key::F6;
      case XK_F7: return chEngineSDK::chKeyBoard::Key::F7;
      case XK_F8: return chEngineSDK::chKeyBoard::Key::F8;
      case XK_F9: return chEngineSDK::chKeyBoard::Key::F9;
      case XK_F10: return chEngineSDK::chKeyBoard::Key::F10;
      case XK_Num_Lock: return chEngineSDK::chKeyBoard::Key::Numlock;
      case XK_Scroll_Lock: return chEngineSDK::chKeyBoard::Key::Scroll;

      // Teclado numérico
      case XK_KP_7: case XK_KP_Home: return chEngineSDK::chKeyBoard::Key::Numpad7;
      case XK_KP_8: case XK_KP_Up: return chEngineSDK::chKeyBoard::Key::Numpad8;
      case XK_KP_9: case XK_KP_Page_Up: return chEngineSDK::chKeyBoard::Key::Numpad9;
      case XK_KP_Subtract: return chEngineSDK::chKeyBoard::Key::Subtract;
      case XK_KP_4: case XK_KP_Left: return chEngineSDK::chKeyBoard::Key::Numpad4;
      case XK_KP_5: return chEngineSDK::chKeyBoard::Key::Numpad5;
      case XK_KP_6: case XK_KP_Right: return chEngineSDK::chKeyBoard::Key::Numpad6;
      case XK_KP_Add: return chEngineSDK::chKeyBoard::Key::Add;
      case XK_KP_1: case XK_KP_End: return chEngineSDK::chKeyBoard::Key::Numpad1;
      case XK_KP_2: case XK_KP_Down: return chEngineSDK::chKeyBoard::Key::Numpad2;
      case XK_KP_3: case XK_KP_Page_Down: return chEngineSDK::chKeyBoard::Key::Numpad3;
      case XK_KP_0: case XK_KP_Insert: return chEngineSDK::chKeyBoard::Key::Numpad0;
      case XK_KP_Decimal: case XK_KP_Delete: return chEngineSDK::chKeyBoard::Key::Decimal;

      case XK_F11: return chEngineSDK::chKeyBoard::Key::F11;
      case XK_F12: return chEngineSDK::chKeyBoard::Key::F12;
      case XK_KP_Enter: return chEngineSDK::chKeyBoard::Key::Numpadenter;
      case XK_Control_R: return chEngineSDK::chKeyBoard::Key::RControl;
      case XK_KP_Divide: return chEngineSDK::chKeyBoard::Key::Divide;
      case XK_Print: case XK_Sys_Req: return chEngineSDK::chKeyBoard::Key::sysrq;
      case XK_Alt_R: return chEngineSDK::chKeyBoard::Key::RAlt;
      case XK_Pause: case XK_Break: return chEngineSDK::chKeyBoard::Key::Pause;

      // Teclas de navegación
      case XK_Home: return chEngineSDK::chKeyBoard::Key::Home;
      case XK_Up: return chEngineSDK::chKeyBoard::Key::Up;
      case XK_Page_Up: return chEngineSDK::chKeyBoard::Key::PgUp;
      case XK_Left: return chEngineSDK::chKeyBoard::Key::Left;
      case XK_Right: return chEngineSDK::chKeyBoard::Key::Right;
      case XK_End: return chEngineSDK::chKeyBoard::Key::End;
      case XK_Down: return chEngineSDK::chKeyBoard::Key::Down;
      case XK_Page_Down: return chEngineSDK::chKeyBoard::Key::PgDn;
      case XK_Insert: return chEngineSDK::chKeyBoard::Key::Insert;
      case XK_Delete: return chEngineSDK::chKeyBoard::Key::Del;

      // Teclas Windows
      case XK_Super_L: return chEngineSDK::chKeyBoard::Key::LMod;
      case XK_Super_R: return chEngineSDK::chKeyBoard::Key::RMod;
      case XK_Menu: return chEngineSDK::chKeyBoard::Key::Apps;

      default: return chEngineSDK::chKeyBoard::Key::KeysMax;
  }
}
#endif // USING(CH_PLATFORM_LINUX)
} // namespace chCrossWindowHelpers
using namespace chCrossWindowHelpers;

namespace chEngineSDK{

/*
*/
DisplayEventHandle::DisplayEventHandle(uint32 width, uint32 height) :
  m_previousWidth(width),
  m_previousHeight(height) {
#if USING(CH_PLATFORM_WIN32)
  m_platformPtr = new ::xwin::EventQueue();
#elif USING(CH_PLATFORM_LINUX)
  m_platformPtr = nullptr;
  m_resizeBound.setCallback([this](uint32 width, uint32 height) {
    addEvent(PlatformEventType::Resize, ResizeData(width, height));
  });
#endif //USING(CH_PLATFORM_LINUX)
}

/*
*/
DisplayEventHandle::~DisplayEventHandle() {

#if USING(CH_PLATFORM_WIN32)
  if (m_platformPtr) {
    delete m_platformPtr;
    m_platformPtr = nullptr;
  }
#elif USING(CH_PLATFORM_LINUX)
  cleanupXCBKeySymbols();
  m_resizeBound.stopDebounce();

#endif //USING(CH_PLATFORM_LINUX)

}

/*
*/
PlatformPtr
DisplayEventHandle::getPlatformPtr() {
  return m_platformPtr;
}

/*
*/
void
DisplayEventHandle::update() {
#if USING(CH_PLATFORM_WIN32)
  if (!m_platformPtr) {
    return;
  }

  m_platformPtr->update();

  while (!m_platformPtr->empty()) {
    const ::xwin::Event event = m_platformPtr->front();
    if (event.type == ::xwin::EventType::MouseMove ||
        event.type == ::xwin::EventType::Focus) {
      m_platformPtr->pop();
      continue;
    }

    switch (event.type) {

      case ::xwin::EventType::MouseInput:
      break;

      case ::xwin::EventType::MouseMove:
        addEvent(PlatformEventType::MouseMove, MouseMoveData(
                 event.data.mouseMove.x,
                 event.data.mouseMove.y,
                 event.data.mouseMove.screenX,
                 event.data.mouseMove.screenY,
                 event.data.mouseMove.deltaX,
                 event.data.mouseMove.deltaY));
      break;

      case ::xwin::EventType::Keyboard:
        addEvent(PlatformEventType::Keyboard, KeyBoardData(
                translateKey(event.data.keyboard. key),
                translateKeyState(event.data.keyboard.state),
                translateKeyModifier(event.data.keyboard.modifiers)));
      break;

      case ::xwin::EventType::Close:
        addEvent(PlatformEventType::Close);
      break;

      default:
      break;
    }

    m_platformPtr->pop();
  }
#elif USING(CH_PLATFORM_LINUX)

  auto getModifier = [](xcb_key_press_event_t* keyEvent) -> uint16{
    uint16 modifiers = 0;
    if (keyEvent->state & XCB_MOD_MASK_SHIFT){
      modifiers |= static_cast<uint16>(KeyBoardModifier::LSHIFT);
    }
    if (keyEvent->state & XCB_MOD_MASK_CONTROL){
      modifiers |= static_cast<uint16>(KeyBoardModifier::LCTRL);
    }
    if (keyEvent->state & XCB_MOD_MASK_1) {
      modifiers |= static_cast<uint16>(KeyBoardModifier::LALT);
    }
    if (keyEvent->state & XCB_MOD_MASK_4) {
      modifiers |= static_cast<uint16>(KeyBoardModifier::LMETA);
    }

    return modifiers;
  };


  xcb_connection_t* connection = getXCBConnection();
  if (connection) {
    xcb_key_symbols_t* keySymbols = getXCBKeySymbols();
    if (keySymbols) {
      xcb_generic_event_t* event;
      while ((event = xcb_poll_for_event(connection))) {
        uint8_t event_type = event->response_type & ~0x80;

        switch (event_type) {

          case XCB_KEY_PRESS: {
            xcb_key_press_event_t* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);
            chKeyBoard::Key key = translateXCBKey(keyEvent->detail, keySymbols);

            activeKeys[keyEvent->detail] = key;

            auto& dispatcher = EventDispatcherManager::instance();
            if (!dispatcher.isKeyDown(key)) {
                KeyBoardData keyData;
                keyData.key = key;
                keyData.state = KeyBoardState::DOWN;
                keyData.modifiers = getModifier(keyEvent);

                addEvent(PlatformEventType::Keyboard, keyData);
            }
            break;
        }

        case XCB_KEY_RELEASE: {
            xcb_key_release_event_t* keyEvent = reinterpret_cast<xcb_key_release_event_t*>(event);

            auto it = activeKeys.find(keyEvent->detail);
            if (it != activeKeys.end()) {
                chKeyBoard::Key key = it->second;
                activeKeys.erase(
                  it);

                KeyBoardData keyData;
                keyData.key = key;
                keyData.state = KeyBoardState::UP;
                keyData.modifiers = getModifier(keyEvent);

                addEvent(PlatformEventType::Keyboard, keyData);
            }
            break;
        }


          case XCB_CLIENT_MESSAGE: {
            //xcb_client_message_event_t* clientMsg = reinterpret_cast<xcb_client_message_event_t*>(event);
            addEvent(PlatformEventType::Close);
            break;
          }

          case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t* buttonEvent = reinterpret_cast<xcb_button_press_event_t*>(event);

            if (buttonEvent->detail == 4 || buttonEvent->detail == 5) {
              int32 deltaY = (buttonEvent->detail == 4) ? 1 : -1;

              addEvent(PlatformEventType::MouseWheel, MouseWheelData(
                deltaY,
                static_cast<uint32>(buttonEvent->event_x),
                static_cast<uint32>(buttonEvent->event_y)
              ));
            }
            else {
              MouseButton button;
              switch (buttonEvent->detail) {
                case 1: button = MouseButton::Left; break;
                case 2: button = MouseButton::Middle; break;
                case 3: button = MouseButton::Right; break;
                default: button = MouseButton::MouseButtonsMax; break;
              }
              addEvent(PlatformEventType::MouseButton, MouseButtonData(
                button,
                MouseState::Down,
                static_cast<uint32>(buttonEvent->event_x),
                static_cast<uint32>(buttonEvent->event_y)
              ));
            }
            break;
          }

          case XCB_BUTTON_RELEASE: {
            xcb_button_release_event_t* buttonEvent = reinterpret_cast<xcb_button_release_event_t*>(event);

            MouseButton button;
            switch (buttonEvent->detail) {
              case 1: button = MouseButton::Left; break;
              case 2: button = MouseButton::Middle; break;
              case 3: button = MouseButton::Right; break;
              case 4: button = MouseButton::MouseButton4; break;
              case 5: button = MouseButton::MouseButton5; break;
              default: continue; // Ignorar botones desconocidos
            }

            // Solo procesar eventos reales de botón (ignorar rueda del mouse)
            if (buttonEvent->detail < 4) {
              addEvent(PlatformEventType::MouseButton, MouseButtonData(
                       button,
                       MouseState::Up,
                       static_cast<uint32>(buttonEvent->event_x),
                       static_cast<uint32>(buttonEvent->event_y)));
            }
            break;
          }

          case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t* motionEvent = reinterpret_cast<xcb_motion_notify_event_t*>(event);

            int32 currentX = static_cast<int32>(motionEvent->event_x);
            int32 currentY = static_cast<int32>(motionEvent->event_y);

            int32 deltaX = 0;
            int32 deltaY = 0;

            if (!m_firstMouseMove) {
              deltaX = currentX - m_previousMouseX;
              deltaY = currentY - m_previousMouseY;
            } else {
              m_firstMouseMove = false;
            }

            m_previousMouseX = currentX;
            m_previousMouseY = currentY;

            addEvent(PlatformEventType::MouseMove, MouseMoveData(
                     static_cast<uint32>(currentX),
                     static_cast<uint32>(currentY),
                     static_cast<uint32>(motionEvent->root_x),
                     static_cast<uint32>(motionEvent->root_y),
                     deltaX, deltaY));
            break;
          }

          case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t* configEvent = reinterpret_cast<xcb_configure_notify_event_t*>(event);
            if (configEvent->width != m_previousWidth || configEvent->height != m_previousHeight) {
              m_previousWidth = configEvent->width;
              m_previousHeight = configEvent->height;
              // addEvent(PlatformEventType::Resize, ResizeData{
              //   static_cast<uint32>(configEvent->width),
              //   static_cast<uint32>(configEvent->height)});
              m_resizeBound.onResize(
                static_cast<uint32>(configEvent->width),
                static_cast<uint32>(configEvent->height));
            }
            break;
          }
        }

        free(event);
      }
    }
  }

  processActiveKeys([&](KeyBoardData keyData) {
    addEvent(PlatformEventType::Keyboard, keyData);
  });
#endif //USING(CH_PLATFORM_LINUX)

}

} // namespace chEngineSDK
#endif //CH_CROSS_WINDOW
