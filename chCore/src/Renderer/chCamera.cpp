/************************************************************************/
/**
 * @file chCamera.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 */
/************************************************************************/

#include "chCamera.h"
#include "chBox.h"
#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chShapeOverlap.h"
#include "chVector2.h"

namespace chEngineSDK {
namespace {

// The columns of a view matrix are the camera's right, up and forward axes in world space.
// As rows forward, right, up they form the rotation that turns the world axes into them.
Quaternion
rotationFromView(const Matrix4& view)
{
  return Matrix4(view[0][2], view[1][2], view[2][2], 0.0f,
                 view[0][0], view[1][0], view[2][0], 0.0f,
                 view[0][1], view[1][1], view[2][1], 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f)
      .toQuaternion();
}

} // namespace

/*
*/
Camera::Camera()
  : m_position(Vector3::ZERO),
    m_rotation(Quaternion::IDENTITY),
    m_fieldOfView(Degree(60.0f)),
    m_nearClip(0.1f),
    m_farClip(1000.0f),
    m_orthographicSize(5.0f),
    m_width(800.0f),
    m_height(600.0f),
    m_projectionType(CameraProjectionType::Perspective) {
  updateMatrices();
}

/*
*/
Camera::Camera(const Vector3& position,
               const Vector3& target,
               float viewPortWidth,
               float viewPortHeight,
               const Vector3& upVector)
  : m_position(position),
    m_rotation(Quaternion::IDENTITY),
    m_fieldOfView(Degree(60.0f)),
    m_nearClip(0.1f),
    m_farClip(1000.0f),
    m_orthographicSize(5.0f),
    m_width(viewPortWidth),
    m_height(viewPortHeight),
    m_projectionType(CameraProjectionType::Perspective) {
  lookAt(target, upVector);
}

/*
*/
void
Camera::setPosition(const Vector3& position) {
  m_position = position;
  calculateViewMatrix();
  extractFrustumPlanes();
}

/*
*/
void
Camera::setRotation(const Quaternion& rotation)
{
  // The view is built from the look at point, so turning the camera moves that point.
  // Roll is lost because the view always keeps world up.
  const float distance = (m_lookAtPoint - m_position).magnitude();
  const float lookDistance = distance > Math::SMALL_NUMBER ? distance : 1.0f;
  m_lookAtPoint = m_position + rotation.rotateVector(Vector3::FORWARD) * lookDistance;
  calculateViewMatrix();
  extractFrustumPlanes();
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
Rotator
Camera::getRotator() const {
  return m_rotation.toRotator();
}

/*
*/
void
Camera::lookAt(const Vector3& target, const Vector3& upVector) {
  m_lookAtPoint = target;
  m_viewMatrix = LookAtMatrix(m_position, target, upVector);
  m_rotation = rotationFromView(m_viewMatrix);
  extractFrustumPlanes();
}

/*
*/
void
Camera::setFieldOfView(Radian fov) {
  m_fieldOfView = fov;
  calculatePerspectiveMatrix();
  extractFrustumPlanes();
}

/*
*/
void
Camera::setViewportSize(float width, float height) {
  m_width = width;
  m_height = height;

  if (m_projectionType == CameraProjectionType::Perspective) {
    calculatePerspectiveMatrix();
  }
  else {
    calculateOrthographicMatrix();
  }

  extractFrustumPlanes();
}

/*
*/
void
Camera::setClipPlanes(float nearPlane, float farPlane) {
  m_nearClip = nearPlane;
  m_farClip = farPlane;

  if (m_projectionType == CameraProjectionType::Perspective) {
    calculatePerspectiveMatrix();
  }
  else {
    calculateOrthographicMatrix();
  }

  extractFrustumPlanes();
}

/*
*/
void
Camera::moveForward(float distance) {
  Vector3 direction = (m_lookAtPoint - m_position).getNormalized();
  m_position += direction * distance;
  m_lookAtPoint += direction * distance;
  calculateViewMatrix();
}

/*
*/
void
Camera::moveRight(float distance) {
  Vector3 forward = (m_lookAtPoint - m_position).getNormalized();
  Vector3 right = Vector3::UP.cross(forward).getNormalized();
  m_position += right * distance;
  m_lookAtPoint += right * distance;
  calculateViewMatrix();
}

/*
*/
void
Camera::moveUp(float distance) {
  m_position += Vector3::UP * distance;
  m_lookAtPoint += Vector3::UP * distance;
  calculateViewMatrix();
}

/*
*/
void
Camera::pan(float deltaX, float deltaY) {
  Vector3 forward = (m_lookAtPoint - m_position).getNormalized();
  Vector3 right = Vector3::UP.cross(forward).getNormalized();
  Vector3 up = forward.cross(right).getNormalized();
  Vector3 panOffset = (right * deltaX) + (up * deltaY);

  m_position += panOffset;
  m_lookAtPoint += panOffset;

  calculateViewMatrix();
}

/*
*/
void
Camera::rotate(float pitch, float yaw, float roll)
{
  CH_PARAMETER_UNUSED(roll);
  const Vector3 toTarget = m_lookAtPoint - m_position;
  const float distance = toTarget.magnitude();
  const Vector3 forward = toTarget / distance;

  // Orbits on a sphere around the look at point. Pitch stops short of straight up or
  // down, where the view has no right axis and would flip.
  constexpr float kMaxPitch = 89.0f;
  const float currentPitch = Math::asin(Math::clamp(forward.z, -1.0f, 1.0f)).valueDegree();
  const float currentYaw = Math::atan2(forward.y, forward.x).valueDegree();
  const Rotator orbit(Math::clamp(currentPitch + pitch, -kMaxPitch, kMaxPitch),
                      currentYaw + yaw, 0.0f);

  const Vector4 rotated = RotationMatrix(orbit).transformVector(Vector3::FORWARD);
  const Vector3 newForward(rotated.x, rotated.y, rotated.z);
  m_position = m_lookAtPoint - newForward * distance;

  calculateViewMatrix();
  extractFrustumPlanes();
}

/*
*/
void
Camera::updateMatrices() {
  calculateViewMatrix();

  if (m_projectionType == CameraProjectionType::Perspective) {
    calculatePerspectiveMatrix();
  }
  else {
    calculateOrthographicMatrix();
  }

  extractFrustumPlanes();
}

/*
*/
Vector3
Camera::getForwardVector() const {
  return (m_lookAtPoint - m_position).getNormalized();
}

/*
*/
Vector3 Camera::getRightVector() const {
  return m_rotation.rotateVector(Vector3::RIGHT);
}

/*
*/
Vector3 Camera::getUpVector() const {
  return m_rotation.rotateVector(Vector3::UP);
}

/*
*/
void
Camera::calculateViewMatrix()
{
  m_viewMatrix = LookAtMatrix(m_position, m_lookAtPoint, Vector3::UP);
  m_rotation = rotationFromView(m_viewMatrix);
}

/*
*/
void
Camera::calculatePerspectiveMatrix() {
  m_projectionMatrix = PerspectiveMatrix(Radian(m_fieldOfView * 0.5f),
                                         m_width,
                                         m_height,
                                         m_nearClip,
                                         m_farClip);
}

/*
*/
void
Camera::calculateOrthographicMatrix()
{
  const float halfHeight = m_orthographicSize;
  const float halfWidth = halfHeight * getAspectRatio();
  m_projectionMatrix = OrthographicMatrix(halfWidth, halfHeight, m_nearClip, m_farClip);
}

/*
*/
void
Camera::extractFrustumPlanes()
{
  m_frustum = Frustum(getViewProjectionMatrix());
}

/*
*/
bool
Camera::isPointInFrustum(const Vector3& point) const
{
  return ShapeOverlap::frustumPoint(m_frustum, point);
}

/*
*/
bool
Camera::isSphereInFrustum(const Vector3& center, float radius) const
{
  return ShapeOverlap::frustumSphere(m_frustum, Sphere(center, radius));
}

/*
*/
bool
Camera::isBoxInFrustum(const AABox& box) const
{
  return ShapeOverlap::frustumBox(m_frustum, box);
}

/*
*/
Vector2
Camera::worldToScreenPoint(const Vector3& worldPos) const {
  // Transform to clip space
  Vector4 viewPos = m_viewMatrix.transformPosition(worldPos);
  Vector4 clipPos = m_projectionMatrix.transformPosition(Vector3(viewPos.x, viewPos.y, viewPos.z));

  // Perspective divide to get NDC coordinates
  clipPos.x /= clipPos.w;
  clipPos.y /= clipPos.w;

  // Map to screen coordinates (0-1 range)
  Vector2 screenPos;
  screenPos.x = (clipPos.x + 1.0f) * 0.5f;
  screenPos.y = (1.0f - clipPos.y) * 0.5f;  // Y is flipped

  return screenPos;
}

/*
*/
void
Camera::screenToWorldRay(const Vector2& screenPos, Vector3& rayOrigin, Vector3& rayDirection) const {
  // Convert screen position to NDC space (-1 to 1)
  Vector4 ndcPos;

  ndcPos.x = screenPos.x * 2.0f - 1.0f;
  ndcPos.y = (1.0f - screenPos.y) * 2.0f - 1.0f; // Flip Y
  ndcPos.z = 0.0f;  // Near plane
  ndcPos.w = 1.0f;

  // Get the inverse view-projection matrix
  Matrix4 invViewProj = getViewProjectionMatrix().getInverse();

  // Transform to world space
  Vector4 worldPosNear = invViewProj.transformVector4(ndcPos);
  worldPosNear.x /= worldPosNear.w;
  worldPosNear.y /= worldPosNear.w;
  worldPosNear.z /= worldPosNear.w;

  // Repeat for far plane
  ndcPos.z = 1.0f;
  Vector4 worldPosFar = invViewProj.transformVector4(ndcPos);
  worldPosFar.x /= worldPosFar.w;
  worldPosFar.y /= worldPosFar.w;
  worldPosFar.z /= worldPosFar.w;

  // Set ray origin and direction
  rayOrigin = Vector3(worldPosNear.x, worldPosNear.y, worldPosNear.z);
  rayDirection = Vector3(worldPosFar.x - worldPosNear.x,
                         worldPosFar.y - worldPosNear.y,
                         worldPosFar.z - worldPosNear.z);
  rayDirection.normalize();
}
} // namespace chEngineSDK
