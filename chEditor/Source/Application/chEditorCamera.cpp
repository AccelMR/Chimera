/************************************************************************/
/**
 * @file chEditorCamera.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Camera of the editor viewport, moved with the mouse and keyboard.
 */
/************************************************************************/
#include "chEditorCamera.h"

#include "chBox.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chInputManager.h"
#include "chMath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

namespace {
ConsoleVariable<float> g_cvarCameraSpeed("Editor.CameraSpeed", 5.0f,
                                         "Units per second the viewport camera flies.",
                                         "CameraSpeed");
ConsoleVariable<float>
    g_cvarCameraLookSensitivity("Editor.CameraLookSensitivity", 0.15f,
                                "Degrees the viewport camera turns per pixel the mouse moves.",
                                "LookSensitivity");
ConsoleVariable<float> g_cvarCameraSprintMultiplier(
    "Editor.CameraSprintMultiplier", 3.0f,
    "How many times faster the viewport camera flies while Shift is held.",
    "SprintMultiplier");
ConsoleVariable<float>
    g_cvarCameraFieldOfView("Editor.CameraFieldOfView", 90.0f,
                            "Horizontal field of view of the viewport camera, in degrees.",
                            "FieldOfView");
ConsoleVariable<float> g_cvarCameraNearPlane("Editor.CameraNearPlane", 0.1f,
                                             "Near clip plane of the viewport camera.",
                                             "NearPlane");
ConsoleVariable<float> g_cvarCameraFarPlane("Editor.CameraFarPlane", 10000.0f,
                                            "Far clip plane of the viewport camera.",
                                            "FarPlane");

constexpr Vector3 kStartPosition(-5.0f, 0.0f, 0.0f);
// Keeps the look at point from collapsing onto the camera while turning.
constexpr float kMinLookAtDistance = 0.1f;
// Each wheel step while flying multiplies or divides the speed by this.
constexpr float kWheelSpeedStep = 1.2f;
// Outside a flight a wheel step moves this fraction of a second of flight.
constexpr float kWheelMoveSeconds = 0.1f;
constexpr float kPanSpeed = 0.01f;
// Looking straight up or down would flip the view, because world Z is always up.
constexpr float kMaxPitchDegrees = 89.0f;
// Keeps focus from putting the camera inside a point-sized object.
constexpr float kMinFocusRadius = 0.1f;

/*
 */
void
saveSetting(StringView key, float value)
{
  ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
  EngineConfig::setUserValue("Editor", "Editor", key, StringUtils::toChars(buffer, value));
}
} // namespace

/*
 */
EditorCamera::EditorCamera(float viewportWidth, float viewportHeight)
 : m_camera(kStartPosition, Vector3::ZERO, viewportWidth, viewportHeight)
{
  m_camera.setProjectionType(CameraProjectionType::Perspective);
  // A config file may hold values out of range: the camera gets them clamped, and the
  // variables keep what the file says until the user edits them.
  applyProjection(g_cvarCameraFieldOfView.get(), g_cvarCameraNearPlane.get(),
                  g_cvarCameraFarPlane.get());
}

/*
 */
void
EditorCamera::update(float deltaTime)
{
  const InputState& input = InputManager::instance().getState();
  const bool bRightButton = input.isDown(InputKey::MouseRight);
  bool bFlightStarted = false;
  if (!m_bFlying && bRightButton && m_bFocused) {
    m_bFlying = true;
    bFlightStarted = true;
  }
  else if (m_bFlying && !bRightButton) {
    m_bFlying = false;
    if (m_bSpeedChanged) {
      m_bSpeedChanged = false;
      saveSettings();
    }
  }

  const float wheelSteps = input.getMouseWheel().y;
  if (wheelSteps != 0.0f) {
    if (m_bFlying) {
      setMoveSpeed(getMoveSpeed() * Math::pow(kWheelSpeedStep, wheelSteps));
      m_bSpeedChanged = true;
    }
    else if (m_bFocused) {
      m_camera.moveForward(wheelSteps * getMoveSpeed() * kWheelMoveSeconds);
    }
  }

  const Vector2& mouseDelta = input.getMouseDelta();
  if (m_bFlying) {
    // The click that starts the flight must not turn the camera.
    if (!bFlightStarted) {
      const float sensitivity = getLookSensitivity();
      // Moving the mouse down looks down.
      look(mouseDelta.x * sensitivity, -mouseDelta.y * sensitivity);
    }
    fly(deltaTime);
  }
  else if (m_bFocused && input.isDown(InputKey::MouseMiddle)) {
    m_camera.pan(-mouseDelta.x * kPanSpeed, -mouseDelta.y * kPanSpeed);
  }
}

/*
 */
void
EditorCamera::look(float yawDegrees, float pitchDegrees)
{
  if (yawDegrees == 0.0f && pitchDegrees == 0.0f) {
    return;
  }

  // Turns around the camera itself; the look at point keeps its distance, so focus and
  // panning still work around it afterwards.
  const Vector3& position = m_camera.getPosition();
  const Vector3 toLookAt = m_camera.getLookAt() - position;
  const float distance = Math::max(toLookAt.magnitude(), kMinLookAtDistance);
  const Vector3 forward = toLookAt * (1.0f / distance);

  const float yaw = Math::atan2(forward.y, forward.x).valueDegree() + yawDegrees;
  const float pitch = Math::clamp(Math::asin(forward.z).valueDegree() + pitchDegrees,
                                  -kMaxPitchDegrees, kMaxPitchDegrees);
  const float cosPitch = Math::cos(Degree(pitch));
  const Vector3 newForward(cosPitch * Math::cos(Degree(yaw)),
                           cosPitch * Math::sin(Degree(yaw)), Math::sin(Degree(pitch)));
  m_camera.lookAt(position + newForward * distance);
}

/*
 */
void
EditorCamera::fly(float deltaTime)
{
  const InputState& input = InputManager::instance().getState();
  const auto axis = [](bool bPositive, bool bNegative) {
    return (bPositive ? 1.0f : 0.0f) - (bNegative ? 1.0f : 0.0f);
  };
  const float forwardAxis = axis(input.isDown(InputKey::W), input.isDown(InputKey::S));
  const float rightAxis = axis(input.isDown(InputKey::D), input.isDown(InputKey::A));
  const float upAxis = axis(input.isDown(InputKey::E), input.isDown(InputKey::Q));
  if (forwardAxis == 0.0f && rightAxis == 0.0f && upAxis == 0.0f) {
    return;
  }

  // Forward follows the view, pitch included, so looking up and pressing W climbs. E and Q
  // stay on the world's up axis. The camera never rolls, so its right is level.
  Vector3 direction = m_camera.getForwardVector() * forwardAxis
                      + m_camera.getRightVector() * rightAxis + Vector3::UP * upAxis;
  // Normalized so a diagonal is not faster than a single key.
  if (!direction.normalize()) {
    return;
  }
  const bool bSprint = input.isDown(InputKey::LeftShift) || input.isDown(InputKey::RightShift);
  const float distance = getMoveSpeed() * deltaTime * (bSprint ? getSprintMultiplier() : 1.0f);
  const Vector3 offset = direction * distance;

  m_camera.setPosition(m_camera.getPosition() + offset);
  m_camera.lookAt(m_camera.getLookAt() + offset);
}

/*
 */
void
EditorCamera::setViewportSize(float width, float height)
{
  m_camera.setViewportSize(width, height);
}

/*
 */
void
EditorCamera::focus(const AABox& bounds)
{
  // The field of view is horizontal, so on a wide viewport the vertical one decides how
  // far the camera must be for the sphere around the bounds to fit.
  const Radian halfHorizontal = m_camera.getFieldOfView() * 0.5f;
  const Radian halfVertical =
      Math::atan(Math::tan(halfHorizontal) / m_camera.getAspectRatio());
  const Radian halfAngle = halfVertical < halfHorizontal ? halfVertical : halfHorizontal;

  const float radius = Math::max(bounds.getExtent().magnitude(), kMinFocusRadius);
  const float distance = radius / Math::sin(halfAngle);
  const Vector3 center = bounds.getCenter();
  m_camera.setPosition(center - m_camera.getForwardVector() * distance);
  m_camera.lookAt(center);
}

/*
 */
void
EditorCamera::setView(const Vector3& position, const Vector3& lookAt)
{
  m_camera.setPosition(position);
  m_camera.lookAt(lookAt);
}

/*
 */
float
EditorCamera::getMoveSpeed() const
{
  return g_cvarCameraSpeed.get();
}

/*
 */
void
EditorCamera::setMoveSpeed(float speed)
{
  g_cvarCameraSpeed.set(Math::clamp(speed, MIN_MOVE_SPEED, MAX_MOVE_SPEED),
                        ConsoleVariableSource::Console);
}

/*
 */
float
EditorCamera::getLookSensitivity() const
{
  return g_cvarCameraLookSensitivity.get();
}

/*
 */
void
EditorCamera::setLookSensitivity(float degreesPerPixel)
{
  g_cvarCameraLookSensitivity.set(degreesPerPixel, ConsoleVariableSource::Console);
}

/*
 */
float
EditorCamera::getSprintMultiplier() const
{
  return g_cvarCameraSprintMultiplier.get();
}

/*
 */
void
EditorCamera::setSprintMultiplier(float multiplier)
{
  g_cvarCameraSprintMultiplier.set(multiplier, ConsoleVariableSource::Console);
}

/*
 */
float
EditorCamera::getFieldOfView() const
{
  return m_camera.getFieldOfView().valueDegree();
}

/*
 */
void
EditorCamera::setFieldOfView(float degrees)
{
  applyProjection(degrees, getNearPlane(), getFarPlane());
  g_cvarCameraFieldOfView.set(getFieldOfView(), ConsoleVariableSource::Console);
}

/*
 */
float
EditorCamera::getNearPlane() const
{
  return m_camera.getNearClipPlane();
}

/*
 */
float
EditorCamera::getFarPlane() const
{
  return m_camera.getFarClipPlane();
}

/*
 */
void
EditorCamera::setClipPlanes(float nearPlane, float farPlane)
{
  applyProjection(getFieldOfView(), nearPlane, farPlane);
  g_cvarCameraNearPlane.set(getNearPlane(), ConsoleVariableSource::Console);
  g_cvarCameraFarPlane.set(getFarPlane(), ConsoleVariableSource::Console);
}

/*
 */
void
EditorCamera::applyProjection(float fieldOfViewDegrees, float nearPlane, float farPlane)
{
  const float fieldOfView =
      Math::clamp(fieldOfViewDegrees, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);
  const float nearClip =
      Math::clamp(nearPlane, MIN_NEAR_PLANE, MAX_FAR_PLANE / MIN_FAR_TO_NEAR_RATIO);
  const float farClip = Math::clamp(farPlane, nearClip * MIN_FAR_TO_NEAR_RATIO, MAX_FAR_PLANE);
  m_camera.setFieldOfView(Radian(Degree(fieldOfView)));
  m_camera.setClipPlanes(nearClip, farClip);
}

/*
 */
void
EditorCamera::resetSettings()
{
  setMoveSpeed(g_cvarCameraSpeed.getDefault());
  setLookSensitivity(g_cvarCameraLookSensitivity.getDefault());
  setSprintMultiplier(g_cvarCameraSprintMultiplier.getDefault());
  setFieldOfView(g_cvarCameraFieldOfView.getDefault());
  setClipPlanes(g_cvarCameraNearPlane.getDefault(), g_cvarCameraFarPlane.getDefault());
  saveSettings();
}

/*
 */
void
EditorCamera::saveSettings() const
{
  saveSetting("CameraSpeed", getMoveSpeed());
  saveSetting("CameraLookSensitivity", getLookSensitivity());
  saveSetting("CameraSprintMultiplier", getSprintMultiplier());
  saveSetting("CameraFieldOfView", getFieldOfView());
  saveSetting("CameraNearPlane", getNearPlane());
  saveSetting("CameraFarPlane", getFarPlane());
}

} // namespace chEngineSDK
