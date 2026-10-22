/************************************************************************/
/**
 * @file chInputState.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  What every input device is doing in the current frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chInputKey.h"
#include "chVector2.h"

namespace chEngineSDK {

/**
 * Dead zones and thresholds used to read analog input, copied from the Input.* console
 * variables once per frame.
 */
struct InputAnalogSettings {
  // Part of a stick's travel, from its center, that reads as zero.
  float stickDeadZone = 0.15f;
  // Part of a trigger's travel, from rest, that reads as zero.
  float triggerDeadZone = 0.05f;
  // How far an axis must move to count as a button that is down.
  float buttonThreshold = 0.5f;
};

/**
 * Exists so game code asks for the state of a key instead of following events: whether
 * it is down, went down or up this frame, and what each axis reads. It is filled from the
 * platform's events (InputManager does it) and never talks to the platform itself, so it
 * can be tested by feeding it by hand.
 *
 * A key pressed and released inside one frame is seen as pressed and released that frame,
 * though it was never down when the frame was read. Between beginFixedStep() and
 * endFixedStep(), wasPressed and wasReleased give what happened since the last fixed step
 * instead, so a fixed step never misses a press and never sees one twice.
 */
class CH_CORE_EXPORT InputState
{
 public:
  struct Gamepad {
    bool bConnected = false;
    GamepadType type = GamepadType::Unknown;
    // One bit per button, then one per axis read as a button (getGamepadBit).
    uint32 down = 0;
    uint32 pressed = 0;
    uint32 released = 0;
    uint32 pressedSinceFixed = 0;
    uint32 releasedSinceFixed = 0;
    // Raw values, -1 to 1 and positive up; the dead zones are applied when read.
    Array<float, InputKeyUtils::GAMEPAD_AXIS_COUNT> axes{};
  };

  NODISCARD FORCEINLINE bool
  isDown(InputKey key, uint32 gamepadMask = ALL_GAMEPADS) const noexcept
  {
    if (InputKeyUtils::isKeyboardOrMouseButton(key)) {
      return m_down.test(static_cast<SIZE_T>(key));
    }
    return anyGamepadBit(key, gamepadMask, &Gamepad::down);
  }

  NODISCARD FORCEINLINE bool
  wasPressed(InputKey key, uint32 gamepadMask = ALL_GAMEPADS) const noexcept
  {
    if (InputKeyUtils::isKeyboardOrMouseButton(key)) {
      return (m_bInFixedStep ? m_pressedSinceFixed : m_pressed).test(static_cast<SIZE_T>(key));
    }
    return anyGamepadBit(key, gamepadMask,
                         m_bInFixedStep ? &Gamepad::pressedSinceFixed : &Gamepad::pressed);
  }

  NODISCARD FORCEINLINE bool
  wasReleased(InputKey key, uint32 gamepadMask = ALL_GAMEPADS) const noexcept
  {
    if (InputKeyUtils::isKeyboardOrMouseButton(key)) {
      return (m_bInFixedStep ? m_releasedSinceFixed : m_released)
          .test(static_cast<SIZE_T>(key));
    }
    return anyGamepadBit(key, gamepadMask,
                         m_bInFixedStep ? &Gamepad::releasedSinceFixed : &Gamepad::released);
  }

  /**
   * A button gives 0 or 1, an axis its value with the dead zones applied (of the gamepads
   * in the mask, the one that moves most). Mouse axes give this frame's movement. A 2D key
   * gives 0; read it with getAxis2D.
   */
  NODISCARD float
  getAxis(InputKey key, uint32 gamepadMask = ALL_GAMEPADS) const noexcept;

  /**
   * A 2D key gives both of its axes (MouseDelta positive right and up); any other key gives
   * getAxis in x and 0 in y.
   */
  NODISCARD Vector2
  getAxis2D(InputKey key, uint32 gamepadMask = ALL_GAMEPADS) const noexcept;

  /**
   * In the units of the window under the mouse, from its top left corner.
   */
  NODISCARD FORCEINLINE const Vector2&
  getMousePosition() const noexcept
  {
    return m_mousePosition;
  }

  /**
   * Movement of this frame in window units, positive right and down like positions.
   */
  NODISCARD FORCEINLINE const Vector2&
  getMouseDelta() const noexcept
  {
    return m_mouseDelta;
  }

  /**
   * Wheel steps of this frame, y positive away from the user, x positive right. Smooth
   * wheels and touchpads give parts of a step.
   */
  NODISCARD FORCEINLINE const Vector2&
  getMouseWheel() const noexcept
  {
    return m_mouseWheel;
  }

  NODISCARD FORCEINLINE const Gamepad&
  getGamepad(uint32 slot) const noexcept
  {
    CH_ASSERT(slot < MAX_GAMEPADS);
    return m_gamepads[slot];
  }

  /**
   * One bit per connected gamepad slot.
   */
  NODISCARD FORCEINLINE uint32
  getConnectedGamepads() const noexcept
  {
    return m_connectedGamepads;
  }

  /**
   * Slot of the gamepad used last (a button, an axis past the button threshold or its
   * connection), or NO_GAMEPAD.
   */
  NODISCARD FORCEINLINE int32
  getLastGamepad() const noexcept
  {
    return m_lastGamepad;
  }

  /**
   * First key, button or axis past the button threshold that went down this frame, from
   * any device; None if nothing did. Mouse movement does not count.
   */
  NODISCARD FORCEINLINE InputKey
  getFirstPressedKey() const noexcept
  {
    return m_firstPressedKey;
  }

  NODISCARD FORCEINLINE bool
  isInFixedStep() const noexcept
  {
    return m_bInFixedStep;
  }

  FORCEINLINE void
  setAnalogSettings(const InputAnalogSettings& settings) noexcept
  {
    m_analog = settings;
  }

  /**
   * Forgets what went down or up last frame and the mouse movement and wheel. Call it
   * before the frame's events are given.
   */
  void
  beginFrame() noexcept;

  void
  beginFixedStep() noexcept
  {
    m_bInFixedStep = true;
  }

  /**
   * Forgets what went down or up since the last fixed step.
   */
  void
  endFixedStep() noexcept;

  /**
   * Keyboard keys and mouse buttons.
   */
  void
  setKey(InputKey key, bool bDown) noexcept;

  void
  addMouseMove(float x, float y, float deltaX, float deltaY) noexcept;

  /**
   * Also presses and releases the wheel buttons for the directions it turned.
   */
  void
  addMouseWheel(float deltaX, float deltaY) noexcept;

  /**
   * A disconnected gamepad releases what it held and reads zero.
   */
  void
  setGamepadConnected(uint32 slot, bool bConnected, GamepadType type) noexcept;

  void
  setGamepadButton(uint32 slot, InputKey button, bool bDown) noexcept;

  void
  setGamepadAxis(uint32 slot, InputKey axis, float value) noexcept;

  /**
   * Releases every key and button and zeroes every axis, for when the window loses the
   * focus and stops getting the events that would release them.
   */
  void
  releaseAll() noexcept;

  /**
   * Bit of a gamepad button or axis in Gamepad's masks; 0 for other keys.
   */
  NODISCARD static constexpr uint32
  getGamepadBit(InputKey key) noexcept
  {
    if (InputKeyUtils::isGamepadButton(key)) {
      return 1u << InputKeyUtils::getGamepadButtonIndex(key);
    }
    if (InputKeyUtils::isGamepadAxis(key)) {
      return 1u << (InputKeyUtils::GAMEPAD_BUTTON_COUNT
                    + InputKeyUtils::getGamepadAxisIndex(key));
    }
    return 0;
  }

  static constexpr uint32 MAX_GAMEPADS = 8;
  static constexpr uint32 ALL_GAMEPADS = (1u << MAX_GAMEPADS) - 1;
  static constexpr int32 NO_GAMEPAD = -1;

 private:
  using KeyBits = BitSet<InputKeyUtils::KEY_COUNT>;

  NODISCARD FORCEINLINE bool
  anyGamepadBit(InputKey key, uint32 gamepadMask, uint32 Gamepad::* bits) const noexcept
  {
    const uint32 bit = getGamepadBit(key);
    // Not limited to connected slots: a gamepad unplugged this frame still reports what it
    // released, and a disconnected one has no other bits.
    const uint32 slots = gamepadMask & ALL_GAMEPADS;
    if (bit == 0 || slots == 0) {
      return false;
    }
    for (uint32 slot = 0; slot < MAX_GAMEPADS; ++slot) {
      if ((slots & (1u << slot)) && (m_gamepads[slot].*bits & bit)) {
        return true;
      }
    }
    return false;
  }

  void
  updateGamepadBit(uint32 slot, uint32 bit, bool bDown, InputKey key) noexcept;

  void
  tapKey(InputKey key) noexcept;

  NODISCARD Vector2
  readStick(const Gamepad& gamepad, uint32 xAxis) const noexcept;

  NODISCARD float
  readGamepadAxis(const Gamepad& gamepad, InputKey axis) const noexcept;

  KeyBits m_down;
  KeyBits m_pressed;
  KeyBits m_released;
  KeyBits m_pressedSinceFixed;
  KeyBits m_releasedSinceFixed;

  Vector2 m_mousePosition = Vector2::ZERO;
  Vector2 m_mouseDelta = Vector2::ZERO;
  Vector2 m_mouseWheel = Vector2::ZERO;

  Array<Gamepad, MAX_GAMEPADS> m_gamepads{};
  uint32 m_connectedGamepads = 0;
  int32 m_lastGamepad = NO_GAMEPAD;

  InputKey m_firstPressedKey = InputKey::None;
  InputAnalogSettings m_analog;
  bool m_bInFixedStep = false;
};

static_assert(InputKeyUtils::GAMEPAD_BUTTON_COUNT + InputKeyUtils::GAMEPAD_AXIS_COUNT <= 32,
              "Gamepad buttons and axes must fit the 32-bit masks of InputState");

} // namespace chEngineSDK
