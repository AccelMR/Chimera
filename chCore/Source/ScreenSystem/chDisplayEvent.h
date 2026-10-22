/************************************************************************/
/**
 * @file chDisplayEvent.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief
 *  Events of the windows and input devices, the same on every platform.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chFlags.h"
#include "chInputKey.h"

namespace chEngineSDK {

enum class KeyModifier : uint16 {
  None = 0,
  LeftShift = 1 << 0,
  RightShift = 1 << 1,
  LeftControl = 1 << 2,
  RightControl = 1 << 3,
  LeftAlt = 1 << 4,
  RightAlt = 1 << 5,
  LeftSuper = 1 << 6,
  RightSuper = 1 << 7,
  CapsLock = 1 << 8,
  NumLock = 1 << 9,
};
CH_FLAGS(KeyModifier);

struct CloseEvent {
};

/**
 * Size of the main window's drawable area, in pixels.
 */
struct ResizeEvent {
  uint32 width;
  uint32 height;
};

/**
 * The main window got or lost the keyboard focus.
 */
struct FocusEvent {
  bool bFocused;
};

/**
 * A keyboard key went down or up. Repeats of a held key are not sent.
 */
struct KeyEvent {
  InputKey key;
  bool bDown;
  KeyModifierFlags modifiers;

  NODISCARD constexpr bool
  hasShift() const noexcept
  {
    return modifiers.isSetAny(KeyModifier::LeftShift | KeyModifier::RightShift);
  }

  NODISCARD constexpr bool
  hasControl() const noexcept
  {
    return modifiers.isSetAny(KeyModifier::LeftControl | KeyModifier::RightControl);
  }

  NODISCARD constexpr bool
  hasAlt() const noexcept
  {
    return modifiers.isSetAny(KeyModifier::LeftAlt | KeyModifier::RightAlt);
  }
};

/**
 * Positions are in the units of the window under the mouse, from its top left corner.
 */
struct MouseButtonEvent {
  InputKey button;
  bool bDown;
  float x;
  float y;
};

/**
 * Deltas are kept below a pixel and are given also while the cursor is locked.
 */
struct MouseMoveEvent {
  float x;
  float y;
  float deltaX;
  float deltaY;
};

/**
 * Positive y turns away from the user, positive x to the right.
 */
struct MouseWheelEvent {
  float deltaX;
  float deltaY;
};

/**
 * slot is the gamepad's place in InputState, kept while it stays connected.
 */
struct GamepadConnectionEvent {
  uint8 slot;
  bool bConnected;
  GamepadType type;
};

struct GamepadButtonEvent {
  uint8 slot;
  InputKey button;
  bool bDown;
};

/**
 * value is -1 to 1, positive up and right; triggers 0 to 1.
 */
struct GamepadAxisEvent {
  uint8 slot;
  InputKey axis;
  float value;
};

/**
 * Every type is trivially copyable, so a frame's events live in one reused vector.
 */
using DisplayEvent =
    Variant<CloseEvent, ResizeEvent, FocusEvent, KeyEvent, MouseButtonEvent, MouseMoveEvent,
            MouseWheelEvent, GamepadConnectionEvent, GamepadButtonEvent, GamepadAxisEvent>;

static_assert(std::is_trivially_copyable_v<DisplayEvent>);

} // namespace chEngineSDK
