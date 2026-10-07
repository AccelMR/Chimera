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
#include "chEventDispatcherManager.h"
#include "chLogger.h"
#include "chMath.h"

CH_LOG_DECLARE_STATIC(EditorCameraLog, All);

namespace chEngineSDK {

namespace {
constexpr Vector3 kStartPosition(-5.0f, 0.0f, 0.0f);
constexpr float kNearPlane = 0.1f;
constexpr float kFarPlane = 10000.0f;
constexpr float kFieldOfViewDegrees = 45.0f;
constexpr float kKeyMoveDistance = 0.01f;
constexpr float kWheelMoveDistance = 0.1f;
constexpr float kPanSpeed = 0.01f;
constexpr float kRotationSpeed = 0.1f;
// Keeps focus from putting the camera inside a point-sized object.
constexpr float kMinFocusRadius = 0.1f;
} // namespace

/*
 */
EditorCamera::EditorCamera(float viewportWidth, float viewportHeight)
 : m_camera(kStartPosition, Vector3::ZERO, viewportWidth, viewportHeight)
{
  m_camera.setProjectionType(CameraProjectionType::Perspective);
  m_camera.setFieldOfView(Radian(Degree(kFieldOfViewDegrees)));
  m_camera.setClipPlanes(kNearPlane, kFarPlane);
}

/*
 */
void
EditorCamera::bindInputEvents()
{
  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  m_keyDownListener = eventDispatcher.OnKeyDown.connect([this](const KeyBoardData& data) {
    if (!m_bFocused) {
      return;
    }
    switch (data.key) {
    case Key::W:
      m_camera.moveForward(kKeyMoveDistance);
      break;
    case Key::S:
      m_camera.moveForward(-kKeyMoveDistance);
      break;
    case Key::A:
      m_camera.moveRight(-kKeyMoveDistance);
      break;
    case Key::D:
      m_camera.moveRight(kKeyMoveDistance);
      break;
    case Key::Q:
      m_camera.moveUp(kKeyMoveDistance);
      break;
    case Key::E:
      m_camera.moveUp(-kKeyMoveDistance);
      break;
    case Key::R:
      m_camera.setPosition(kStartPosition);
      m_camera.lookAt(Vector3::ZERO);
      break;
    case Key::P: {
      const Vector3& position = m_camera.getPosition();
      CH_LOG_INFO(EditorCameraLog, "Camera position: ({0}, {1}, {2})", position.x,
                  position.y, position.z);
      break;
    }
    default:
      break;
    }
  });

  m_mouseWheelListener =
      eventDispatcher.OnMouseWheel.connect([this](const MouseWheelData& data) {
        if (m_bFocused && data.deltaY != 0) {
          m_camera.moveForward(data.deltaY * kWheelMoveDistance);
        }
      });

  m_mouseMoveListener = eventDispatcher.OnMouseMove.connect(
      [this, &eventDispatcher](const MouseMoveData& data) {
        if (!m_bFocused || (data.deltaX == 0 && data.deltaY == 0)) {
          return;
        }
        if (eventDispatcher.isMouseButtonDown(MouseButton::Middle)) {
          m_camera.pan(-data.deltaX * kPanSpeed, -data.deltaY * kPanSpeed);
        }
        if (eventDispatcher.isMouseButtonDown(MouseButton::Right)) {
          m_camera.rotate(data.deltaY * kRotationSpeed, data.deltaX * kRotationSpeed);
        }
      });
}

/*
 */
void
EditorCamera::unbindInputEvents()
{
  m_keyDownListener.disconnect();
  m_mouseWheelListener.disconnect();
  m_mouseMoveListener.disconnect();
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

} // namespace chEngineSDK
