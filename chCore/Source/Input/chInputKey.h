/************************************************************************/
/**
 * @file chInputKey.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  One id for every key, button and axis of every input device.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Every key, button and axis of the keyboard, the mouse and the gamepads, so a binding is
 * one value whatever device it reads. Keyboard keys are physical positions (scancodes): W
 * is the key left of E on any layout. Names are the identifiers below and are what
 * Input.ini stores, so renaming one breaks saved bindings; the numbers are never stored.
 * Axes that go up and down (stick Y, mouse Y) are positive upward.
 */
enum class InputKey : uint16 {
  None = 0,

  // Keyboard
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  Escape,
  Enter,
  Tab,
  Backspace,
  Space,
  CapsLock,
  Minus,
  Equals,
  LeftBracket,
  RightBracket,
  Backslash,
  NonUSBackslash,
  Semicolon,
  Apostrophe,
  Grave,
  Comma,
  Period,
  Slash,
  LeftShift,
  RightShift,
  LeftControl,
  RightControl,
  LeftAlt,
  RightAlt,
  LeftSuper,
  RightSuper,
  Menu,
  PrintScreen,
  ScrollLock,
  Pause,
  Insert,
  Delete,
  Home,
  End,
  PageUp,
  PageDown,
  Up,
  Down,
  Left,
  Right,
  NumLock,
  NumpadDivide,
  NumpadMultiply,
  NumpadMinus,
  NumpadPlus,
  NumpadEnter,
  NumpadDecimal,
  Numpad0,
  Numpad1,
  Numpad2,
  Numpad3,
  Numpad4,
  Numpad5,
  Numpad6,
  Numpad7,
  Numpad8,
  Numpad9,

  // Mouse buttons. The wheel ones are pressed and released in the frame the wheel turns.
  MouseLeft,
  MouseRight,
  MouseMiddle,
  MouseX1,
  MouseX2,
  MouseWheelUp,
  MouseWheelDown,
  MouseWheelLeft,
  MouseWheelRight,

  // Mouse axes: movement and wheel of this frame.
  MouseX,
  MouseY,
  MouseWheel,
  MouseWheelHorizontal,
  MouseDelta,

  // Gamepad buttons, named by position: South is A on Xbox and Cross on PlayStation.
  GamepadSouth,
  GamepadEast,
  GamepadWest,
  GamepadNorth,
  GamepadBack,
  GamepadGuide,
  GamepadStart,
  GamepadLeftStickButton,
  GamepadRightStickButton,
  GamepadLeftShoulder,
  GamepadRightShoulder,
  GamepadDPadUp,
  GamepadDPadDown,
  GamepadDPadLeft,
  GamepadDPadRight,
  GamepadMisc1,
  GamepadRightPaddle1,
  GamepadLeftPaddle1,
  GamepadRightPaddle2,
  GamepadLeftPaddle2,
  GamepadTouchpad,

  // Gamepad axes, -1 to 1 (triggers 0 to 1).
  GamepadLeftX,
  GamepadLeftY,
  GamepadRightX,
  GamepadRightY,
  GamepadLeftTrigger,
  GamepadRightTrigger,
  GamepadLeftStick,
  GamepadRightStick,

  COUNT
};

enum class InputDevice : uint8 { None, Keyboard, Mouse, Gamepad };

/**
 * What a key gives: a button is down or up, an axis gives one value, a 2D axis two.
 */
enum class InputKeyType : uint8 { None, Button, Axis1D, Axis2D };

/**
 * Family of a gamepad, which decides how its buttons are named to the player.
 */
enum class GamepadType : uint8 { Unknown, Standard, Xbox, PlayStation, Nintendo };

/**
 * Exists to answer questions about an InputKey without a table lookup in hot code (device,
 * type, index inside its group) and to turn keys into the names Input.ini stores.
 */
class CH_CORE_EXPORT InputKeyUtils
{
 public:
  NODISCARD static constexpr InputDevice
  getDevice(InputKey key) noexcept
  {
    if (key == InputKey::None || key >= InputKey::COUNT) {
      return InputDevice::None;
    }
    if (key < InputKey::MouseLeft) {
      return InputDevice::Keyboard;
    }
    if (key < InputKey::GamepadSouth) {
      return InputDevice::Mouse;
    }
    return InputDevice::Gamepad;
  }

  NODISCARD static constexpr InputKeyType
  getType(InputKey key) noexcept
  {
    if (key == InputKey::None || key >= InputKey::COUNT) {
      return InputKeyType::None;
    }
    if (key == InputKey::MouseDelta || key == InputKey::GamepadLeftStick
        || key == InputKey::GamepadRightStick) {
      return InputKeyType::Axis2D;
    }
    if ((key >= InputKey::MouseX && key < InputKey::MouseDelta)
        || (key >= InputKey::GamepadLeftX && key < InputKey::GamepadLeftStick)) {
      return InputKeyType::Axis1D;
    }
    return InputKeyType::Button;
  }

  /**
   * True for keys that live in the keyboard and mouse button set: every keyboard key and
   * the mouse buttons, wheel buttons included.
   */
  NODISCARD static constexpr bool
  isKeyboardOrMouseButton(InputKey key) noexcept
  {
    return key > InputKey::None && key < InputKey::MouseX;
  }

  NODISCARD static constexpr bool
  isGamepadButton(InputKey key) noexcept
  {
    return key >= InputKey::GamepadSouth && key < InputKey::GamepadLeftX;
  }

  NODISCARD static constexpr bool
  isGamepadAxis(InputKey key) noexcept
  {
    return key >= InputKey::GamepadLeftX && key < InputKey::GamepadLeftStick;
  }

  /**
   * Index of a gamepad button (0 for South) or axis (0 for LeftX) inside its group.
   */
  NODISCARD static constexpr uint32
  getGamepadButtonIndex(InputKey key) noexcept
  {
    return static_cast<uint32>(key) - static_cast<uint32>(InputKey::GamepadSouth);
  }

  NODISCARD static constexpr uint32
  getGamepadAxisIndex(InputKey key) noexcept
  {
    return static_cast<uint32>(key) - static_cast<uint32>(InputKey::GamepadLeftX);
  }

  NODISCARD static constexpr InputKey
  getGamepadButton(uint32 index) noexcept
  {
    return static_cast<InputKey>(static_cast<uint32>(InputKey::GamepadSouth) + index);
  }

  NODISCARD static constexpr InputKey
  getGamepadAxis(uint32 index) noexcept
  {
    return static_cast<InputKey>(static_cast<uint32>(InputKey::GamepadLeftX) + index);
  }

  /**
   * The identifier of the key ("Space", "GamepadSouth"); empty for None.
   */
  NODISCARD static StringView
  getName(InputKey key) noexcept;

  /**
   * The key with that name, ignoring case; None when no key has it.
   */
  NODISCARD static InputKey
  fromName(StringView name) noexcept;

  static constexpr uint32 KEY_COUNT = static_cast<uint32>(InputKey::COUNT);
  static constexpr uint32 GAMEPAD_BUTTON_COUNT = static_cast<uint32>(InputKey::GamepadLeftX)
                                                 - static_cast<uint32>(InputKey::GamepadSouth);
  static constexpr uint32 GAMEPAD_AXIS_COUNT = static_cast<uint32>(InputKey::GamepadLeftStick)
                                               - static_cast<uint32>(InputKey::GamepadLeftX);
};

} // namespace chEngineSDK
