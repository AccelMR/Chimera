/************************************************************************/
/**
 * @file chCamera.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief Camera with a view, a projection and their frustum.
 */
/************************************************************************/

#include "chCamera.h"

#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chRotator.h"
#include "chVector2.h"
#include "chVector4.h"

namespace chEngineSDK {
namespace {
// Orbiting stops short of straight up or down, where the view has no right axis and
// would flip.
constexpr float kMaxPitch = 89.0f;
} // namespace

/*
*/
Camera::Camera(const Vector3& position, const Vector3& target, float viewportWidth,
               float viewportHeight)
 : m_position(position),
   m_lookAtPoint(target),
   m_width(viewportWidth),
   m_height(viewportHeight)
{}

/*
*/
void
Camera::setRotation(const Quaternion& rotation)
{
  // The view is built from the look at point, so turning the camera moves that point.
  const float distance = (m_lookAtPoint - m_position).magnitude();
  const float lookDistance = distance > Math::SMALL_NUMBER ? distance : 1.0f;
  m_lookAtPoint = m_position + rotation.rotateVector(Vector3::FORWARD) * lookDistance;
  markViewDirty();
}

/*
*/
void
Camera::setRotator(const Rotator& rotator)
{
  setRotation(rotator.toQuaternion());
}

/*
*/
Quaternion
Camera::getRotation() const
{
  // The columns of a view matrix are the camera's right, up and forward axes in world
  // space. As rows forward, right, up they form the rotation that turns the world axes
  // into them.
  const Matrix4& view = getViewMatrix();
  return Quaternion(Matrix4(view[0][2], view[1][2], view[2][2], 0.0f,
                            view[0][0], view[1][0], view[2][0], 0.0f,
                            view[0][1], view[1][1], view[2][1], 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f));
}

/*
*/
Rotator
Camera::getRotator() const
{
  return getRotation().toRotator();
}

/*
*/
void
Camera::moveForward(float distance)
{
  // Read from the two points instead of the view matrix, so several moves in one frame
  // do not rebuild the view each time.
  const Vector3 offset = (m_lookAtPoint - m_position).getNormalized() * distance;
  m_position += offset;
  m_lookAtPoint += offset;
  markViewDirty();
}

/*
*/
void
Camera::moveRight(float distance)
{
  const Vector3 forward = (m_lookAtPoint - m_position).getNormalized();
  const Vector3 offset = Vector3::UP.cross(forward).getNormalized() * distance;
  m_position += offset;
  m_lookAtPoint += offset;
  markViewDirty();
}

/*
*/
void
Camera::moveUp(float distance)
{
  const Vector3 offset = Vector3::UP * distance;
  m_position += offset;
  m_lookAtPoint += offset;
  markViewDirty();
}

/*
*/
void
Camera::pan(float deltaX, float deltaY)
{
  const Vector3 forward = (m_lookAtPoint - m_position).getNormalized();
  const Vector3 right = Vector3::UP.cross(forward).getNormalized();
  const Vector3 up = forward.cross(right);
  const Vector3 offset = right * deltaX + up * deltaY;
  m_position += offset;
  m_lookAtPoint += offset;
  markViewDirty();
}

/*
*/
void
Camera::rotate(float pitchDegrees, float yawDegrees)
{
  const Vector3 toTarget = m_lookAtPoint - m_position;
  const float distance = toTarget.magnitude();
  if (distance <= Math::SMALL_NUMBER) {
    return;
  }
  const Vector3 forward = toTarget / distance;

  const float pitch =
      Math::clamp(Math::asin(forward.z).valueDegree() + pitchDegrees, -kMaxPitch, kMaxPitch);
  const float yaw = Math::atan2(forward.y, forward.x).valueDegree() + yawDegrees;

  // Positive pitch turns forward up and positive yaw turns it right.
  float sinPitch, cosPitch, sinYaw, cosYaw;
  Math::sinCos(Degree(pitch).valueRadian(), sinPitch, cosPitch);
  Math::sinCos(Degree(yaw).valueRadian(), sinYaw, cosYaw);
  const Vector3 newForward(cosPitch * cosYaw, cosPitch * sinYaw, sinPitch);

  m_position = m_lookAtPoint - newForward * distance;
  markViewDirty();
}

/*
*/
Vector3
Camera::getForwardVector() const
{
  const Matrix4& view = getViewMatrix();
  return {view[0][2], view[1][2], view[2][2]};
}

/*
*/
Vector3
Camera::getRightVector() const
{
  const Matrix4& view = getViewMatrix();
  return {view[0][0], view[1][0], view[2][0]};
}

/*
*/
Vector3
Camera::getUpVector() const
{
  const Matrix4& view = getViewMatrix();
  return {view[0][1], view[1][1], view[2][1]};
}

/*
*/
bool
Camera::worldToScreenPoint(const Vector3& worldPoint, Vector2& outScreenPoint) const
{
  const Vector4 clip = getViewProjectionMatrix().transformPosition(worldPoint);
  if (clip.w <= Math::SMALL_NUMBER) {
    return false;
  }

  // Normalized device coordinates have Y up; the screen has Y down.
  const float inverseW = 1.0f / clip.w;
  outScreenPoint.x = (clip.x * inverseW + 1.0f) * 0.5f;
  outScreenPoint.y = (1.0f - clip.y * inverseW) * 0.5f;
  return true;
}

/*
*/
Ray
Camera::screenToWorldRay(const Vector2& screenPoint) const
{
  const float ndcX = screenPoint.x * 2.0f - 1.0f;
  const float ndcY = (1.0f - screenPoint.y) * 2.0f - 1.0f;
  const Matrix4 inverse = getViewProjectionMatrix().getInverse();

  // Depth 0 is the near plane and 1 the far plane.
  const Vector4 nearPoint = inverse.transformVector4(Vector4(ndcX, ndcY, 0.0f, 1.0f));
  const Vector4 farPoint = inverse.transformVector4(Vector4(ndcX, ndcY, 1.0f, 1.0f));
  const Vector3 origin(nearPoint.x / nearPoint.w, nearPoint.y / nearPoint.w,
                       nearPoint.z / nearPoint.w);
  const Vector3 end(farPoint.x / farPoint.w, farPoint.y / farPoint.w,
                    farPoint.z / farPoint.w);
  return Ray(origin, (end - origin).getNormalized());
}

/*
*/
void
Camera::updateView() const
{
  m_viewMatrix = LookAtMatrix(m_position, m_lookAtPoint, Vector3::UP);
  m_viewDirty = false;
}

/*
*/
void
Camera::updateProjection() const
{
  if (m_projectionType == CameraProjectionType::Perspective) {
    m_projectionMatrix =
        PerspectiveMatrix(m_fieldOfView * 0.5f, m_width, m_height, m_nearClip, m_farClip);
  }
  else {
    m_projectionMatrix = OrthographicMatrix(m_orthographicSize * getAspectRatio(),
                                            m_orthographicSize, m_nearClip, m_farClip);
  }
  m_projectionDirty = false;
}

/*
*/
void
Camera::updateViewProjection() const
{
  m_viewProjectionMatrix = getViewMatrix() * getProjectionMatrix();
  m_frustum = Frustum(m_viewProjectionMatrix);
  m_viewProjectionDirty = false;
}
} // namespace chEngineSDK
