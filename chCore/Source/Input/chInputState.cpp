/************************************************************************/
/**
 * @file chInputState.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  What every input device is doing in the current frame.
 */
/************************************************************************/
#include "chInputState.h"

#include "chMath.h"

namespace chEngineSDK {

namespace {
constexpr uint32 kLeftXAxis = InputKeyUtils::getGamepadAxisIndex(InputKey::GamepadLeftX);
constexpr uint32 kRightXAxis = InputKeyUtils::getGamepadAxisIndex(InputKey::GamepadRightX);

/*
 * Stretches what is left past the dead zone back to the whole range, so a stick reads
 * small values just past it instead of jumping to the dead zone's size.
 */
NODISCARD float
removeDeadZone(float value, float deadZone) noexcept
{
  if (value <= deadZone) {
    return 0.0f;
  }
  return Math::min((value - deadZone) / (1.0f - deadZone), 1.0f);
}
} // namespace

/*
 */
float
InputState::getAxis(InputKey key, uint32 gamepadMask) const noexcept
{
  switch (key) {
  case InputKey::MouseX:
    return m_mouseDelta.x;
  case InputKey::MouseY:
    return -m_mouseDelta.y;
  case InputKey::MouseWheel:
    return m_mouseWheel.y;
  case InputKey::MouseWheelHorizontal:
    return m_mouseWheel.x;
  default:
    break;
  }

  switch (InputKeyUtils::getType(key)) {
  case InputKeyType::Button:
    return isDown(key, gamepadMask) ? 1.0f : 0.0f;
  case InputKeyType::Axis1D:
    break;
  default:
    return 0.0f;
  }

  float result = 0.0f;
  const uint32 slots = gamepadMask & m_connectedGamepads;
  for (uint32 slot = 0; slot < MAX_GAMEPADS; ++slot) {
    if (slots & (1u << slot)) {
      const float value = readGamepadAxis(m_gamepads[slot], key);
      if (Math::abs(value) > Math::abs(result)) {
        result = value;
      }
    }
  }
  return result;
}

/*
 */
Vector2
InputState::getAxis2D(InputKey key, uint32 gamepadMask) const noexcept
{
  if (key == InputKey::MouseDelta) {
    return Vector2(m_mouseDelta.x, -m_mouseDelta.y);
  }
  if (key != InputKey::GamepadLeftStick && key != InputKey::GamepadRightStick) {
    return Vector2(getAxis(key, gamepadMask), 0.0f);
  }

  const uint32 xAxis = key == InputKey::GamepadLeftStick ? kLeftXAxis : kRightXAxis;
  Vector2 result = Vector2::ZERO;
  float resultLength = 0.0f;
  const uint32 slots = gamepadMask & m_connectedGamepads;
  for (uint32 slot = 0; slot < MAX_GAMEPADS; ++slot) {
    if (slots & (1u << slot)) {
      const Vector2 value = readStick(m_gamepads[slot], xAxis);
      const float length = value.sqrMagnitude();
      if (length > resultLength) {
        result = value;
        resultLength = length;
      }
    }
  }
  return result;
}

/*
 */
void
InputState::beginFrame() noexcept
{
  m_pressed.reset();
  m_released.reset();
  m_mouseDelta = Vector2::ZERO;
  m_mouseWheel = Vector2::ZERO;
  for (Gamepad& gamepad : m_gamepads) {
    gamepad.pressed = 0;
    gamepad.released = 0;
  }
  m_firstPressedKey = InputKey::None;
}

/*
 */
void
InputState::endFixedStep() noexcept
{
  m_bInFixedStep = false;
  m_pressedSinceFixed.reset();
  m_releasedSinceFixed.reset();
  for (Gamepad& gamepad : m_gamepads) {
    gamepad.pressedSinceFixed = 0;
    gamepad.releasedSinceFixed = 0;
  }
}

/*
 */
void
InputState::setKey(InputKey key, bool bDown) noexcept
{
  if (!InputKeyUtils::isKeyboardOrMouseButton(key)) {
    return;
  }
  const SIZE_T index = static_cast<SIZE_T>(key);
  if (m_down.test(index) == bDown) {
    return;
  }

  m_down.set(index, bDown);
  if (bDown) {
    m_pressed.set(index);
    m_pressedSinceFixed.set(index);
    if (m_firstPressedKey == InputKey::None) {
      m_firstPressedKey = key;
    }
  }
  else {
    m_released.set(index);
    m_releasedSinceFixed.set(index);
  }
}

/*
 */
void
InputState::tapKey(InputKey key) noexcept
{
  const SIZE_T index = static_cast<SIZE_T>(key);
  m_pressed.set(index);
  m_released.set(index);
  m_pressedSinceFixed.set(index);
  m_releasedSinceFixed.set(index);
  if (m_firstPressedKey == InputKey::None) {
    m_firstPressedKey = key;
  }
}

/*
 */
void
InputState::addMouseMove(float x, float y, float deltaX, float deltaY) noexcept
{
  m_mousePosition = Vector2(x, y);
  m_mouseDelta.x += deltaX;
  m_mouseDelta.y += deltaY;
}

/*
 */
void
InputState::addMouseWheel(float deltaX, float deltaY) noexcept
{
  m_mouseWheel.x += deltaX;
  m_mouseWheel.y += deltaY;
  if (deltaY > 0.0f) {
    tapKey(InputKey::MouseWheelUp);
  }
  else if (deltaY < 0.0f) {
    tapKey(InputKey::MouseWheelDown);
  }
  if (deltaX > 0.0f) {
    tapKey(InputKey::MouseWheelRight);
  }
  else if (deltaX < 0.0f) {
    tapKey(InputKey::MouseWheelLeft);
  }
}

/*
 */
void
InputState::setGamepadConnected(uint32 slot, bool bConnected, GamepadType type) noexcept
{
  if (slot >= MAX_GAMEPADS) {
    return;
  }

  Gamepad& gamepad = m_gamepads[slot];
  if (!bConnected) {
    gamepad.released |= gamepad.down;
    gamepad.releasedSinceFixed |= gamepad.down;
    gamepad.down = 0;
    gamepad.axes.fill(0.0f);
    gamepad.bConnected = false;
    m_connectedGamepads &= ~(1u << slot);
    if (m_lastGamepad == static_cast<int32>(slot)) {
      m_lastGamepad = NO_GAMEPAD;
    }
    return;
  }

  gamepad.bConnected = true;
  gamepad.type = type;
  m_connectedGamepads |= 1u << slot;
  m_lastGamepad = static_cast<int32>(slot);
}

/*
 */
void
InputState::updateGamepadBit(uint32 slot, uint32 bit, bool bDown, InputKey key) noexcept
{
  Gamepad& gamepad = m_gamepads[slot];
  if (((gamepad.down & bit) != 0) == bDown) {
    return;
  }

  if (bDown) {
    gamepad.down |= bit;
    gamepad.pressed |= bit;
    gamepad.pressedSinceFixed |= bit;
    m_lastGamepad = static_cast<int32>(slot);
    if (m_firstPressedKey == InputKey::None) {
      m_firstPressedKey = key;
    }
  }
  else {
    gamepad.down &= ~bit;
    gamepad.released |= bit;
    gamepad.releasedSinceFixed |= bit;
  }
}

/*
 */
void
InputState::setGamepadButton(uint32 slot, InputKey button, bool bDown) noexcept
{
  if (slot >= MAX_GAMEPADS || !m_gamepads[slot].bConnected
      || !InputKeyUtils::isGamepadButton(button)) {
    return;
  }
  updateGamepadBit(slot, getGamepadBit(button), bDown, button);
}

/*
 */
void
InputState::setGamepadAxis(uint32 slot, InputKey axis, float value) noexcept
{
  if (slot >= MAX_GAMEPADS || !m_gamepads[slot].bConnected
      || !InputKeyUtils::isGamepadAxis(axis)) {
    return;
  }
  m_gamepads[slot].axes[InputKeyUtils::getGamepadAxisIndex(axis)] =
      Math::clamp(value, -1.0f, 1.0f);
  updateGamepadBit(slot, getGamepadBit(axis), Math::abs(value) >= m_analog.buttonThreshold,
                   axis);
}

/*
 */
void
InputState::releaseAll() noexcept
{
  m_released |= m_down;
  m_releasedSinceFixed |= m_down;
  m_down.reset();
  for (Gamepad& gamepad : m_gamepads) {
    gamepad.released |= gamepad.down;
    gamepad.releasedSinceFixed |= gamepad.down;
    gamepad.down = 0;
    gamepad.axes.fill(0.0f);
  }
}

/*
 * The dead zone is a circle, so a stick pushed along a diagonal is not cut on one axis
 * before the other.
 */
Vector2
InputState::readStick(const Gamepad& gamepad, uint32 xAxis) const noexcept
{
  const Vector2 raw(gamepad.axes[xAxis], gamepad.axes[xAxis + 1]);
  const float length = raw.magnitude();
  if (length <= m_analog.stickDeadZone) {
    return Vector2::ZERO;
  }
  return raw * (removeDeadZone(length, m_analog.stickDeadZone) / length);
}

/*
 */
float
InputState::readGamepadAxis(const Gamepad& gamepad, InputKey axis) const noexcept
{
  switch (axis) {
  case InputKey::GamepadLeftX:
    return readStick(gamepad, kLeftXAxis).x;
  case InputKey::GamepadLeftY:
    return readStick(gamepad, kLeftXAxis).y;
  case InputKey::GamepadRightX:
    return readStick(gamepad, kRightXAxis).x;
  case InputKey::GamepadRightY:
    return readStick(gamepad, kRightXAxis).y;
  case InputKey::GamepadLeftTrigger:
  case InputKey::GamepadRightTrigger:
    return removeDeadZone(gamepad.axes[InputKeyUtils::getGamepadAxisIndex(axis)],
                          m_analog.triggerDeadZone);
  default:
    return 0.0f;
  }
}

} // namespace chEngineSDK
