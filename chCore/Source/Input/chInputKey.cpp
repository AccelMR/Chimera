/************************************************************************/
/**
 * @file chInputKey.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  One id for every key, button and axis of every input device.
 */
/************************************************************************/
#include "chInputKey.h"

#include "chStringUtils.h"

namespace chEngineSDK {

namespace {
// In the order of InputKey, None first.
constexpr StringView kKeyNames[] = {
    "",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "Num0",
    "Num1",
    "Num2",
    "Num3",
    "Num4",
    "Num5",
    "Num6",
    "Num7",
    "Num8",
    "Num9",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "Escape",
    "Enter",
    "Tab",
    "Backspace",
    "Space",
    "CapsLock",
    "Minus",
    "Equals",
    "LeftBracket",
    "RightBracket",
    "Backslash",
    "NonUSBackslash",
    "Semicolon",
    "Apostrophe",
    "Grave",
    "Comma",
    "Period",
    "Slash",
    "LeftShift",
    "RightShift",
    "LeftControl",
    "RightControl",
    "LeftAlt",
    "RightAlt",
    "LeftSuper",
    "RightSuper",
    "Menu",
    "PrintScreen",
    "ScrollLock",
    "Pause",
    "Insert",
    "Delete",
    "Home",
    "End",
    "PageUp",
    "PageDown",
    "Up",
    "Down",
    "Left",
    "Right",
    "NumLock",
    "NumpadDivide",
    "NumpadMultiply",
    "NumpadMinus",
    "NumpadPlus",
    "NumpadEnter",
    "NumpadDecimal",
    "Numpad0",
    "Numpad1",
    "Numpad2",
    "Numpad3",
    "Numpad4",
    "Numpad5",
    "Numpad6",
    "Numpad7",
    "Numpad8",
    "Numpad9",
    "MouseLeft",
    "MouseRight",
    "MouseMiddle",
    "MouseX1",
    "MouseX2",
    "MouseWheelUp",
    "MouseWheelDown",
    "MouseWheelLeft",
    "MouseWheelRight",
    "MouseX",
    "MouseY",
    "MouseWheel",
    "MouseWheelHorizontal",
    "MouseDelta",
    "GamepadSouth",
    "GamepadEast",
    "GamepadWest",
    "GamepadNorth",
    "GamepadBack",
    "GamepadGuide",
    "GamepadStart",
    "GamepadLeftStickButton",
    "GamepadRightStickButton",
    "GamepadLeftShoulder",
    "GamepadRightShoulder",
    "GamepadDPadUp",
    "GamepadDPadDown",
    "GamepadDPadLeft",
    "GamepadDPadRight",
    "GamepadMisc1",
    "GamepadRightPaddle1",
    "GamepadLeftPaddle1",
    "GamepadRightPaddle2",
    "GamepadLeftPaddle2",
    "GamepadTouchpad",
    "GamepadLeftX",
    "GamepadLeftY",
    "GamepadRightX",
    "GamepadRightY",
    "GamepadLeftTrigger",
    "GamepadRightTrigger",
    "GamepadLeftStick",
    "GamepadRightStick",
};
static_assert(sizeof(kKeyNames) / sizeof(kKeyNames[0]) == InputKeyUtils::KEY_COUNT,
              "Every InputKey needs a name");
} // namespace

/*
 */
StringView
InputKeyUtils::getName(InputKey key) noexcept
{
  const uint32 index = static_cast<uint32>(key);
  return index < KEY_COUNT ? kKeyNames[index] : StringView();
}

/*
 */
InputKey
InputKeyUtils::fromName(StringView name) noexcept
{
  if (name.empty()) {
    return InputKey::None;
  }
  // Only read while loading bindings, so a plain search is enough.
  for (uint32 i = 1; i < KEY_COUNT; ++i) {
    if (StringUtils::equalsIgnoreCase(kKeyNames[i], name)) {
      return static_cast<InputKey>(i);
    }
  }
  return InputKey::None;
}

} // namespace chEngineSDK
