/************************************************************************/
/**
 * @file chDisplayEventHandleSDL3.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @brief
 *  SDL3 events and input devices: turns SDL events into DisplayEvents and answers what
 *  the input system asks of the platform.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"

#if USING(CH_DISPLAY_SDL3)
#include <SDL3/SDL.h>

#include "chDisplaySurface.h"
#include "chInputPlatform.h"

namespace chEngineSDK {

namespace {
struct ScancodeKey {
  SDL_Scancode scancode;
  InputKey key;
};

constexpr ScancodeKey kScancodeKeys[] = {
    {SDL_SCANCODE_A, InputKey::A},
    {SDL_SCANCODE_B, InputKey::B},
    {SDL_SCANCODE_C, InputKey::C},
    {SDL_SCANCODE_D, InputKey::D},
    {SDL_SCANCODE_E, InputKey::E},
    {SDL_SCANCODE_F, InputKey::F},
    {SDL_SCANCODE_G, InputKey::G},
    {SDL_SCANCODE_H, InputKey::H},
    {SDL_SCANCODE_I, InputKey::I},
    {SDL_SCANCODE_J, InputKey::J},
    {SDL_SCANCODE_K, InputKey::K},
    {SDL_SCANCODE_L, InputKey::L},
    {SDL_SCANCODE_M, InputKey::M},
    {SDL_SCANCODE_N, InputKey::N},
    {SDL_SCANCODE_O, InputKey::O},
    {SDL_SCANCODE_P, InputKey::P},
    {SDL_SCANCODE_Q, InputKey::Q},
    {SDL_SCANCODE_R, InputKey::R},
    {SDL_SCANCODE_S, InputKey::S},
    {SDL_SCANCODE_T, InputKey::T},
    {SDL_SCANCODE_U, InputKey::U},
    {SDL_SCANCODE_V, InputKey::V},
    {SDL_SCANCODE_W, InputKey::W},
    {SDL_SCANCODE_X, InputKey::X},
    {SDL_SCANCODE_Y, InputKey::Y},
    {SDL_SCANCODE_Z, InputKey::Z},
    {SDL_SCANCODE_0, InputKey::Num0},
    {SDL_SCANCODE_1, InputKey::Num1},
    {SDL_SCANCODE_2, InputKey::Num2},
    {SDL_SCANCODE_3, InputKey::Num3},
    {SDL_SCANCODE_4, InputKey::Num4},
    {SDL_SCANCODE_5, InputKey::Num5},
    {SDL_SCANCODE_6, InputKey::Num6},
    {SDL_SCANCODE_7, InputKey::Num7},
    {SDL_SCANCODE_8, InputKey::Num8},
    {SDL_SCANCODE_9, InputKey::Num9},
    {SDL_SCANCODE_F1, InputKey::F1},
    {SDL_SCANCODE_F2, InputKey::F2},
    {SDL_SCANCODE_F3, InputKey::F3},
    {SDL_SCANCODE_F4, InputKey::F4},
    {SDL_SCANCODE_F5, InputKey::F5},
    {SDL_SCANCODE_F6, InputKey::F6},
    {SDL_SCANCODE_F7, InputKey::F7},
    {SDL_SCANCODE_F8, InputKey::F8},
    {SDL_SCANCODE_F9, InputKey::F9},
    {SDL_SCANCODE_F10, InputKey::F10},
    {SDL_SCANCODE_F11, InputKey::F11},
    {SDL_SCANCODE_F12, InputKey::F12},
    {SDL_SCANCODE_F13, InputKey::F13},
    {SDL_SCANCODE_F14, InputKey::F14},
    {SDL_SCANCODE_F15, InputKey::F15},
    {SDL_SCANCODE_F16, InputKey::F16},
    {SDL_SCANCODE_F17, InputKey::F17},
    {SDL_SCANCODE_F18, InputKey::F18},
    {SDL_SCANCODE_F19, InputKey::F19},
    {SDL_SCANCODE_F20, InputKey::F20},
    {SDL_SCANCODE_F21, InputKey::F21},
    {SDL_SCANCODE_F22, InputKey::F22},
    {SDL_SCANCODE_F23, InputKey::F23},
    {SDL_SCANCODE_F24, InputKey::F24},
    {SDL_SCANCODE_ESCAPE, InputKey::Escape},
    {SDL_SCANCODE_RETURN, InputKey::Enter},
    {SDL_SCANCODE_TAB, InputKey::Tab},
    {SDL_SCANCODE_BACKSPACE, InputKey::Backspace},
    {SDL_SCANCODE_SPACE, InputKey::Space},
    {SDL_SCANCODE_CAPSLOCK, InputKey::CapsLock},
    {SDL_SCANCODE_MINUS, InputKey::Minus},
    {SDL_SCANCODE_EQUALS, InputKey::Equals},
    {SDL_SCANCODE_LEFTBRACKET, InputKey::LeftBracket},
    {SDL_SCANCODE_RIGHTBRACKET, InputKey::RightBracket},
    {SDL_SCANCODE_BACKSLASH, InputKey::Backslash},
    {SDL_SCANCODE_NONUSBACKSLASH, InputKey::NonUSBackslash},
    {SDL_SCANCODE_SEMICOLON, InputKey::Semicolon},
    {SDL_SCANCODE_APOSTROPHE, InputKey::Apostrophe},
    {SDL_SCANCODE_GRAVE, InputKey::Grave},
    {SDL_SCANCODE_COMMA, InputKey::Comma},
    {SDL_SCANCODE_PERIOD, InputKey::Period},
    {SDL_SCANCODE_SLASH, InputKey::Slash},
    {SDL_SCANCODE_LSHIFT, InputKey::LeftShift},
    {SDL_SCANCODE_RSHIFT, InputKey::RightShift},
    {SDL_SCANCODE_LCTRL, InputKey::LeftControl},
    {SDL_SCANCODE_RCTRL, InputKey::RightControl},
    {SDL_SCANCODE_LALT, InputKey::LeftAlt},
    {SDL_SCANCODE_RALT, InputKey::RightAlt},
    {SDL_SCANCODE_LGUI, InputKey::LeftSuper},
    {SDL_SCANCODE_RGUI, InputKey::RightSuper},
    {SDL_SCANCODE_APPLICATION, InputKey::Menu},
    {SDL_SCANCODE_PRINTSCREEN, InputKey::PrintScreen},
    {SDL_SCANCODE_SCROLLLOCK, InputKey::ScrollLock},
    {SDL_SCANCODE_PAUSE, InputKey::Pause},
    {SDL_SCANCODE_INSERT, InputKey::Insert},
    {SDL_SCANCODE_DELETE, InputKey::Delete},
    {SDL_SCANCODE_HOME, InputKey::Home},
    {SDL_SCANCODE_END, InputKey::End},
    {SDL_SCANCODE_PAGEUP, InputKey::PageUp},
    {SDL_SCANCODE_PAGEDOWN, InputKey::PageDown},
    {SDL_SCANCODE_UP, InputKey::Up},
    {SDL_SCANCODE_DOWN, InputKey::Down},
    {SDL_SCANCODE_LEFT, InputKey::Left},
    {SDL_SCANCODE_RIGHT, InputKey::Right},
    {SDL_SCANCODE_NUMLOCKCLEAR, InputKey::NumLock},
    {SDL_SCANCODE_KP_DIVIDE, InputKey::NumpadDivide},
    {SDL_SCANCODE_KP_MULTIPLY, InputKey::NumpadMultiply},
    {SDL_SCANCODE_KP_MINUS, InputKey::NumpadMinus},
    {SDL_SCANCODE_KP_PLUS, InputKey::NumpadPlus},
    {SDL_SCANCODE_KP_ENTER, InputKey::NumpadEnter},
    {SDL_SCANCODE_KP_PERIOD, InputKey::NumpadDecimal},
    {SDL_SCANCODE_KP_0, InputKey::Numpad0},
    {SDL_SCANCODE_KP_1, InputKey::Numpad1},
    {SDL_SCANCODE_KP_2, InputKey::Numpad2},
    {SDL_SCANCODE_KP_3, InputKey::Numpad3},
    {SDL_SCANCODE_KP_4, InputKey::Numpad4},
    {SDL_SCANCODE_KP_5, InputKey::Numpad5},
    {SDL_SCANCODE_KP_6, InputKey::Numpad6},
    {SDL_SCANCODE_KP_7, InputKey::Numpad7},
    {SDL_SCANCODE_KP_8, InputKey::Numpad8},
    {SDL_SCANCODE_KP_9, InputKey::Numpad9},
};

// SDL numbers gamepad buttons and axes in the order of InputKey, so they convert by offset.
static_assert(SDL_GAMEPAD_BUTTON_SOUTH == 0 && SDL_GAMEPAD_BUTTON_TOUCHPAD == 20
              && InputKeyUtils::GAMEPAD_BUTTON_COUNT == 21);
static_assert(
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT
    == static_cast<int32>(InputKeyUtils::getGamepadButtonIndex(InputKey::GamepadDPadRight)));
static_assert(SDL_GAMEPAD_AXIS_LEFTX == 0 && SDL_GAMEPAD_AXIS_RIGHT_TRIGGER == 5
              && InputKeyUtils::GAMEPAD_AXIS_COUNT == 6);

/*
 * Built once, so translating a key is one array read per event.
 */
NODISCARD InputKey
translateScancode(SDL_Scancode scancode) noexcept
{
  static const Array<InputKey, SDL_SCANCODE_COUNT> table = [] {
    Array<InputKey, SDL_SCANCODE_COUNT> result{};
    for (const ScancodeKey& entry : kScancodeKeys) {
      result[entry.scancode] = entry.key;
    }
    return result;
  }();
  return static_cast<uint32>(scancode) < SDL_SCANCODE_COUNT ? table[scancode] : InputKey::None;
}

NODISCARD InputKey
translateMouseButton(uint8 button) noexcept
{
  switch (button) {
  case SDL_BUTTON_LEFT:
    return InputKey::MouseLeft;
  case SDL_BUTTON_MIDDLE:
    return InputKey::MouseMiddle;
  case SDL_BUTTON_RIGHT:
    return InputKey::MouseRight;
  case SDL_BUTTON_X1:
    return InputKey::MouseX1;
  case SDL_BUTTON_X2:
    return InputKey::MouseX2;
  default:
    return InputKey::None;
  }
}

NODISCARD KeyModifierFlags
translateModifiers(SDL_Keymod modifiers) noexcept
{
  struct ModifierPair {
    SDL_Keymod sdl;
    KeyModifier engine;
  };
  constexpr ModifierPair kModifiers[] = {
      {SDL_KMOD_LSHIFT, KeyModifier::LeftShift},  {SDL_KMOD_RSHIFT, KeyModifier::RightShift},
      {SDL_KMOD_LCTRL, KeyModifier::LeftControl}, {SDL_KMOD_RCTRL, KeyModifier::RightControl},
      {SDL_KMOD_LALT, KeyModifier::LeftAlt},      {SDL_KMOD_RALT, KeyModifier::RightAlt},
      {SDL_KMOD_LGUI, KeyModifier::LeftSuper},    {SDL_KMOD_RGUI, KeyModifier::RightSuper},
      {SDL_KMOD_CAPS, KeyModifier::CapsLock},     {SDL_KMOD_NUM, KeyModifier::NumLock},
  };

  KeyModifierFlags result;
  for (const ModifierPair& pair : kModifiers) {
    if (modifiers & pair.sdl) {
      result.set(pair.engine);
    }
  }
  return result;
}

NODISCARD GamepadType
translateGamepadType(SDL_GamepadType type) noexcept
{
  switch (type) {
  case SDL_GAMEPAD_TYPE_STANDARD:
    return GamepadType::Standard;
  case SDL_GAMEPAD_TYPE_XBOX360:
  case SDL_GAMEPAD_TYPE_XBOXONE:
    return GamepadType::Xbox;
  case SDL_GAMEPAD_TYPE_PS3:
  case SDL_GAMEPAD_TYPE_PS4:
  case SDL_GAMEPAD_TYPE_PS5:
    return GamepadType::PlayStation;
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
    return GamepadType::Nintendo;
  default:
    return GamepadType::Unknown;
  }
}

/*
 * SDL gives -32768 to 32767; sticks point Y down, the engine up.
 */
NODISCARD float
translateAxisValue(uint8 axis, int16 value) noexcept
{
  const float normalized = static_cast<float>(value) / 32767.0f;
  const float clamped = normalized < -1.0f ? -1.0f : normalized;
  return (axis == SDL_GAMEPAD_AXIS_LEFTY || axis == SDL_GAMEPAD_AXIS_RIGHTY) ? -clamped
                                                                             : clamped;
}

struct GamepadSlot {
  SDL_JoystickID id = 0;
  SDL_Gamepad* gamepad = nullptr;
};

NODISCARD Array<GamepadSlot, InputState::MAX_GAMEPADS>&
getGamepadSlots()
{
  static Array<GamepadSlot, InputState::MAX_GAMEPADS> slots{};
  return slots;
}

NODISCARD int32
findGamepadSlot(SDL_JoystickID id) noexcept
{
  const Array<GamepadSlot, InputState::MAX_GAMEPADS>& slots = getGamepadSlots();
  for (uint32 i = 0; i < InputState::MAX_GAMEPADS; ++i) {
    if (slots[i].gamepad && slots[i].id == id) {
      return static_cast<int32>(i);
    }
  }
  return InputState::NO_GAMEPAD;
}
} // namespace

/*
 */
void
DisplayEventHandle::update()
{
  m_events.clear();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    m_updateInjection(Any(static_cast<const SDL_Event*>(&event)));
    switch (event.type) {
    case SDL_EVENT_QUIT:
      m_events.emplace_back(CloseEvent{});
      break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      if (event.window.windowID == m_mainWindowId) {
        m_events.emplace_back(CloseEvent{});
      }
      break;

    // Pixels, not the window size in screen units: the swap chain and render targets
    // are sized in pixels, and the two differ on screens scaled for high DPI. Only the
    // main window counts; the windows ImGui opens resize through its own hooks.
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      if (event.window.windowID == m_mainWindowId) {
        m_events.emplace_back(ResizeEvent{.width = static_cast<uint32>(event.window.data1),
                                          .height = static_cast<uint32>(event.window.data2)});
      }
      break;

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      if (event.window.windowID == m_mainWindowId) {
        m_events.emplace_back(
            FocusEvent{.bFocused = event.type == SDL_EVENT_WINDOW_FOCUS_GAINED});
      }
      break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      const InputKey key = translateScancode(event.key.scancode);
      if (key != InputKey::None && !event.key.repeat) {
        m_events.emplace_back(KeyEvent{.key = key,
                                       .bDown = event.key.down,
                                       .modifiers = translateModifiers(event.key.mod)});
      }
    } break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      const InputKey button = translateMouseButton(event.button.button);
      if (button != InputKey::None) {
        m_events.emplace_back(MouseButtonEvent{.button = button,
                                               .bDown = event.button.down,
                                               .x = event.button.x,
                                               .y = event.button.y});
      }
    } break;

    case SDL_EVENT_MOUSE_MOTION:
      m_events.emplace_back(MouseMoveEvent{.x = event.motion.x,
                                           .y = event.motion.y,
                                           .deltaX = event.motion.xrel,
                                           .deltaY = event.motion.yrel});
      break;

    case SDL_EVENT_MOUSE_WHEEL: {
      const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
      m_events.emplace_back(MouseWheelEvent{.deltaX = event.wheel.x * direction,
                                            .deltaY = event.wheel.y * direction});
    } break;

    case SDL_EVENT_GAMEPAD_ADDED: {
      if (findGamepadSlot(event.gdevice.which) != InputState::NO_GAMEPAD) {
        break;
      }
      Array<GamepadSlot, InputState::MAX_GAMEPADS>& slots = getGamepadSlots();
      uint32 slot = 0;
      while (slot < InputState::MAX_GAMEPADS && slots[slot].gamepad) {
        ++slot;
      }
      if (slot == InputState::MAX_GAMEPADS) {
        CH_LOG_WARNING(InputSystem, "A gamepad was connected, but {0} are already in use.",
                       InputState::MAX_GAMEPADS);
        break;
      }
      SDL_Gamepad* gamepad = SDL_OpenGamepad(event.gdevice.which);
      if (!gamepad) {
        CH_LOG_WARNING(InputSystem, "Could not open a gamepad: {0}", SDL_GetError());
        break;
      }
      slots[slot] = {.id = event.gdevice.which, .gamepad = gamepad};
      const char* name = SDL_GetGamepadName(gamepad);
      CH_LOG_INFO(InputSystem, "Gamepad {0} connected: {1}", slot, name ? name : "unknown");
      m_events.emplace_back(
          GamepadConnectionEvent{.slot = static_cast<uint8>(slot),
                                 .bConnected = true,
                                 .type = translateGamepadType(SDL_GetGamepadType(gamepad))});
    } break;

    case SDL_EVENT_GAMEPAD_REMOVED: {
      const int32 slot = findGamepadSlot(event.gdevice.which);
      if (slot == InputState::NO_GAMEPAD) {
        break;
      }
      GamepadSlot& entry = getGamepadSlots()[slot];
      SDL_CloseGamepad(entry.gamepad);
      entry = GamepadSlot{};
      CH_LOG_INFO(InputSystem, "Gamepad {0} disconnected.", slot);
      m_events.emplace_back(GamepadConnectionEvent{.slot = static_cast<uint8>(slot),
                                                   .bConnected = false,
                                                   .type = GamepadType::Unknown});
    } break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
      const int32 slot = findGamepadSlot(event.gbutton.which);
      if (slot != InputState::NO_GAMEPAD
          && event.gbutton.button < InputKeyUtils::GAMEPAD_BUTTON_COUNT) {
        m_events.emplace_back(
            GamepadButtonEvent{.slot = static_cast<uint8>(slot),
                               .button = InputKeyUtils::getGamepadButton(event.gbutton.button),
                               .bDown = event.gbutton.down});
      }
    } break;

    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
      const int32 slot = findGamepadSlot(event.gaxis.which);
      if (slot != InputState::NO_GAMEPAD
          && event.gaxis.axis < InputKeyUtils::GAMEPAD_AXIS_COUNT) {
        m_events.emplace_back(GamepadAxisEvent{
            .slot = static_cast<uint8>(slot),
            .axis = InputKeyUtils::getGamepadAxis(event.gaxis.axis),
            .value = translateAxisValue(event.gaxis.axis, event.gaxis.value)});
      }
    } break;

    default:
      break;
    }
  }
}

/*
 */
void
InputPlatform::setCursorMode(DisplaySurface& window, CursorMode mode)
{
  SDL_Window* handle = window.getPlatformHandler();
  if (!handle) {
    return;
  }
  SDL_SetWindowRelativeMouseMode(handle, mode == CursorMode::Locked);
  if (mode == CursorMode::Normal) {
    SDL_ShowCursor();
  }
  else {
    SDL_HideCursor();
  }
}

/*
 */
bool
InputPlatform::rumbleGamepad(uint32 slot, float lowFrequency, float highFrequency,
                             uint32 milliseconds)
{
  if (slot >= InputState::MAX_GAMEPADS) {
    return false;
  }
  SDL_Gamepad* gamepad = getGamepadSlots()[slot].gamepad;
  return gamepad
         && SDL_RumbleGamepad(gamepad, static_cast<uint16>(lowFrequency * 65535.0f),
                              static_cast<uint16>(highFrequency * 65535.0f), milliseconds);
}

/*
 */
String
InputPlatform::getKeyboardKeyName(InputKey key)
{
  for (const ScancodeKey& entry : kScancodeKeys) {
    if (entry.key == key) {
      const char* name =
          SDL_GetKeyName(SDL_GetKeyFromScancode(entry.scancode, SDL_KMOD_NONE, false));
      return name ? String(name) : String();
    }
  }
  return String();
}

/*
 */
void
InputPlatform::shutDown()
{
  for (GamepadSlot& slot : getGamepadSlots()) {
    if (slot.gamepad) {
      SDL_CloseGamepad(slot.gamepad);
    }
    slot = GamepadSlot{};
  }
}

} // namespace chEngineSDK

#endif // USING(CH_DISPLAY_SDL3)
