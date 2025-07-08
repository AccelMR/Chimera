/************************************************************************/
/**
 * @file chDisplayEventHandleSDL3.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @brief SDL3 specific implementation of DisplayEventHandle with Mouse & Keyboard support.
 * @details Optimized SDL3 event handling for Chimera Engine architecture.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"

#if USING(CH_DISPLAY_SDL3)
#include "chEventDispatcherManager.h"
#include <SDL3/SDL.h>

namespace chSDLHelpers {
using namespace chEngineSDK;

FORCEINLINE chEngineSDK::chKeyBoard::Key
translateSDLKey(SDL_Scancode scancode) {
  using Key = chEngineSDK::chKeyBoard::Key;

  switch (scancode) {
  // Numbers
  case SDL_SCANCODE_1:
    return Key::Num1;
  case SDL_SCANCODE_2:
    return Key::Num2;
  case SDL_SCANCODE_3:
    return Key::Num3;
  case SDL_SCANCODE_4:
    return Key::Num4;
  case SDL_SCANCODE_5:
    return Key::Num5;
  case SDL_SCANCODE_6:
    return Key::Num6;
  case SDL_SCANCODE_7:
    return Key::Num7;
  case SDL_SCANCODE_8:
    return Key::Num8;
  case SDL_SCANCODE_9:
    return Key::Num9;
  case SDL_SCANCODE_0:
    return Key::Num0;

  // Letters
  case SDL_SCANCODE_A:
    return Key::A;
  case SDL_SCANCODE_B:
    return Key::B;
  case SDL_SCANCODE_C:
    return Key::C;
  case SDL_SCANCODE_D:
    return Key::D;
  case SDL_SCANCODE_E:
    return Key::E;
  case SDL_SCANCODE_F:
    return Key::F;
  case SDL_SCANCODE_G:
    return Key::G;
  case SDL_SCANCODE_H:
    return Key::H;
  case SDL_SCANCODE_I:
    return Key::I;
  case SDL_SCANCODE_J:
    return Key::J;
  case SDL_SCANCODE_K:
    return Key::K;
  case SDL_SCANCODE_L:
    return Key::L;
  case SDL_SCANCODE_M:
    return Key::M;
  case SDL_SCANCODE_N:
    return Key::N;
  case SDL_SCANCODE_O:
    return Key::O;
  case SDL_SCANCODE_P:
    return Key::P;
  case SDL_SCANCODE_Q:
    return Key::Q;
  case SDL_SCANCODE_R:
    return Key::R;
  case SDL_SCANCODE_S:
    return Key::S;
  case SDL_SCANCODE_T:
    return Key::T;
  case SDL_SCANCODE_U:
    return Key::U;
  case SDL_SCANCODE_V:
    return Key::V;
  case SDL_SCANCODE_W:
    return Key::W;
  case SDL_SCANCODE_X:
    return Key::X;
  case SDL_SCANCODE_Y:
    return Key::Y;
  case SDL_SCANCODE_Z:
    return Key::Z;

  // Special keys
  case SDL_SCANCODE_ESCAPE:
    return Key::Escape;
  case SDL_SCANCODE_SPACE:
    return Key::Space;
  case SDL_SCANCODE_RETURN:
    return Key::Enter;
  case SDL_SCANCODE_BACKSPACE:
    return Key::Back;
  case SDL_SCANCODE_TAB:
    return Key::Tab;
  case SDL_SCANCODE_CAPSLOCK:
    return Key::Capital;

  // Modifiers
  case SDL_SCANCODE_LSHIFT:
    return Key::LShift;
  case SDL_SCANCODE_RSHIFT:
    return Key::RShift;
  case SDL_SCANCODE_LCTRL:
    return Key::LControl;
  case SDL_SCANCODE_RCTRL:
    return Key::RControl;
  case SDL_SCANCODE_LALT:
    return Key::LAlt;
  case SDL_SCANCODE_RALT:
    return Key::RAlt;
  case SDL_SCANCODE_LGUI:
    return Key::LMod;
  case SDL_SCANCODE_RGUI:
    return Key::RMod;

  // Function keys
  case SDL_SCANCODE_F1:
    return Key::F1;
  case SDL_SCANCODE_F2:
    return Key::F2;
  case SDL_SCANCODE_F3:
    return Key::F3;
  case SDL_SCANCODE_F4:
    return Key::F4;
  case SDL_SCANCODE_F5:
    return Key::F5;
  case SDL_SCANCODE_F6:
    return Key::F6;
  case SDL_SCANCODE_F7:
    return Key::F7;
  case SDL_SCANCODE_F8:
    return Key::F8;
  case SDL_SCANCODE_F9:
    return Key::F9;
  case SDL_SCANCODE_F10:
    return Key::F10;
  case SDL_SCANCODE_F11:
    return Key::F11;
  case SDL_SCANCODE_F12:
    return Key::F12;

  // Arrow keys
  case SDL_SCANCODE_UP:
    return Key::Up;
  case SDL_SCANCODE_DOWN:
    return Key::Down;
  case SDL_SCANCODE_LEFT:
    return Key::Left;
  case SDL_SCANCODE_RIGHT:
    return Key::Right;

  // Navigation
  case SDL_SCANCODE_HOME:
    return Key::Home;
  case SDL_SCANCODE_END:
    return Key::End;
  case SDL_SCANCODE_PAGEUP:
    return Key::PgUp;
  case SDL_SCANCODE_PAGEDOWN:
    return Key::PgDn;
  case SDL_SCANCODE_INSERT:
    return Key::Insert;
  case SDL_SCANCODE_DELETE:
    return Key::Del;

  // Numpad
  case SDL_SCANCODE_KP_0:
    return Key::Numpad0;
  case SDL_SCANCODE_KP_1:
    return Key::Numpad1;
  case SDL_SCANCODE_KP_2:
    return Key::Numpad2;
  case SDL_SCANCODE_KP_3:
    return Key::Numpad3;
  case SDL_SCANCODE_KP_4:
    return Key::Numpad4;
  case SDL_SCANCODE_KP_5:
    return Key::Numpad5;
  case SDL_SCANCODE_KP_6:
    return Key::Numpad6;
  case SDL_SCANCODE_KP_7:
    return Key::Numpad7;
  case SDL_SCANCODE_KP_8:
    return Key::Numpad8;
  case SDL_SCANCODE_KP_9:
    return Key::Numpad9;
  case SDL_SCANCODE_KP_ENTER:
    return Key::Numpadenter;
  case SDL_SCANCODE_KP_PLUS:
    return Key::Add;
  case SDL_SCANCODE_KP_MINUS:
    return Key::Subtract;
  case SDL_SCANCODE_KP_MULTIPLY:
    return Key::Multiply;
  case SDL_SCANCODE_KP_DIVIDE:
    return Key::Divide;
  case SDL_SCANCODE_KP_PERIOD:
    return Key::Decimal;

  // Symbols
  case SDL_SCANCODE_MINUS:
    return Key::Minus;
  case SDL_SCANCODE_EQUALS:
    return Key::Equals;
  case SDL_SCANCODE_LEFTBRACKET:
    return Key::LBracket;
  case SDL_SCANCODE_RIGHTBRACKET:
    return Key::RBracket;
  case SDL_SCANCODE_BACKSLASH:
    return Key::Backslash;
  case SDL_SCANCODE_SEMICOLON:
    return Key::Semicolon;
  case SDL_SCANCODE_APOSTROPHE:
    return Key::Apostrophe;
  case SDL_SCANCODE_GRAVE:
    return Key::Grave;
  case SDL_SCANCODE_COMMA:
    return Key::Comma;
  case SDL_SCANCODE_PERIOD:
    return Key::Period;
  case SDL_SCANCODE_SLASH:
    return Key::Slash;

  // Lock keys
  case SDL_SCANCODE_NUMLOCKCLEAR:
    return Key::Numlock;
  case SDL_SCANCODE_SCROLLLOCK:
    return Key::Scroll;
  case SDL_SCANCODE_PAUSE:
    return Key::Pause;
  case SDL_SCANCODE_PRINTSCREEN:
    return Key::sysrq;

  default:
    return Key::KeysMax;
  }
}

FORCEINLINE chEngineSDK::MouseButton
translateSDLMouseButton(uint8 button) {
  using MouseButton = chEngineSDK::MouseButton;

  switch (button) {
  case SDL_BUTTON_LEFT:
    return MouseButton::Left;
  case SDL_BUTTON_MIDDLE:
    return MouseButton::Middle;
  case SDL_BUTTON_RIGHT:
    return MouseButton::Right;
  case SDL_BUTTON_X1:
    return MouseButton::MouseButton4;
  case SDL_BUTTON_X2:
    return MouseButton::MouseButton5;
  default:
    return MouseButton::MouseButtonsMax;
  }
}

FORCEINLINE uint16
getSDLKeyboardModifiers(SDL_Keymod modifiers) {
  using KeyBoardModifier = chEngineSDK::KeyBoardModifier;

  uint16 result = 0;

  if (modifiers & SDL_KMOD_LSHIFT) {
    result |= static_cast<uint16>(KeyBoardModifier::LSHIFT);
  }
  if (modifiers & SDL_KMOD_LCTRL) {
    result |= static_cast<uint16>(KeyBoardModifier::LCTRL);
  }
  if (modifiers & SDL_KMOD_LALT) {
    result |= static_cast<uint16>(KeyBoardModifier::LALT);
  }
  if (modifiers & SDL_KMOD_LGUI) {
    result |= static_cast<uint16>(KeyBoardModifier::LMETA);
  }

  return result;
}

} // namespace chSDLHelpers

using namespace chSDLHelpers;

namespace chEngineSDK {

/*
 */
void
DisplayEventHandle::update() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      addEvent(PlatformEventType::Close);
      break;

    case SDL_EVENT_WINDOW_RESIZED:
      addEvent(PlatformEventType::Resize, std::move(ResizeData{static_cast<uint32>(event.window.data1),
                                                     static_cast<uint32>(event.window.data2)}));

      break;

    case SDL_EVENT_KEY_DOWN: {
      chKeyBoard::Key key = translateSDLKey(event.key.scancode);
      if (key != chKeyBoard::Key::KeysMax) {
        auto& dispatcher = EventDispatcherManager::instance();

        // Check if key is already down to handle key repeat
        if (!dispatcher.isKeyDown(key)) {
          KeyBoardData keyData{key, KeyBoardState::DOWN,
                               getSDLKeyboardModifiers(event.key.mod)};
          addEvent(PlatformEventType::Keyboard, std::move(keyData));
        }
        else {
          // Key is being held down (repeat)
          KeyBoardData keyData{key, KeyBoardState::PRESSED,
                               getSDLKeyboardModifiers(event.key.mod)};
          addEvent(PlatformEventType::Keyboard, std::move(keyData));
        }
      }
      break;
    }

    case SDL_EVENT_KEY_UP: {
      chKeyBoard::Key key = translateSDLKey(event.key.scancode);
      if (key != chKeyBoard::Key::KeysMax) {
        KeyBoardData keyData{key, KeyBoardState::UP, getSDLKeyboardModifiers(event.key.mod)};
        addEvent(PlatformEventType::Keyboard, std::move(keyData));
      }
      break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
      MouseButton button = translateSDLMouseButton(event.button.button);
      if (button != MouseButton::MouseButtonsMax) {
        auto& dispatcher = EventDispatcherManager::instance();

        if (!dispatcher.isMouseButtonDown(button)) {
          MouseButtonData buttonData{button, MouseState::Down, static_cast<uint32>(event.button.x),
                                     static_cast<uint32>(event.button.y)};
          addEvent(PlatformEventType::MouseButton, std::move(buttonData));
        }
        else {
          // Button is being held down (repeat)
          MouseButtonData buttonData{button, MouseState::Pressed, static_cast<uint32>(event.button.x),
                                     static_cast<uint32>(event.button.y)};
          addEvent(PlatformEventType::MouseButton, std::move(buttonData));
        }
      }
      break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP: {
      MouseButton button = translateSDLMouseButton(event.button.button);
      if (button != MouseButton::MouseButtonsMax) {
        MouseButtonData buttonData{button, MouseState::Up, static_cast<uint32>(event.button.x),
                                   static_cast<uint32>(event.button.y)};
        addEvent(PlatformEventType::MouseButton, std::move(buttonData));
      }
      break;
    }

    case SDL_EVENT_MOUSE_MOTION: {
      MouseMoveData moveData{
          static_cast<uint32>(event.motion.x),   static_cast<uint32>(event.motion.y),
          static_cast<uint32>(event.motion.x), // SDL doesn't provide global coordinates
                                               // directly
          static_cast<uint32>(event.motion.y), // You might need to calculate these separately
          static_cast<int32>(event.motion.xrel), static_cast<int32>(event.motion.yrel)};
      addEvent(PlatformEventType::MouseMove, std::move(moveData));
      break;
    }

    case SDL_EVENT_MOUSE_WHEEL: {
      // Get current mouse position for wheel events
      float mouseX, mouseY;
      SDL_GetMouseState(&mouseX, &mouseY);

      MouseWheelData wheelData{static_cast<int32>(event.wheel.y), // Vertical scroll
                               static_cast<uint32>(mouseX), static_cast<uint32>(mouseY)};
      addEvent(PlatformEventType::MouseWheel, std::move(wheelData));
      break;
    }

    default:
      // Handle other events if necessary
      break;
    }
  }
}

} // namespace chEngineSDK

#endif // USING(CH_DISPLAY_SDL3)
