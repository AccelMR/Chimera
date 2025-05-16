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
#include "chPlane.h"
#include "chVector2.h"

namespace chEngineSDK {
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
Camera::setRotation(const Quaternion& rotation) {
  m_rotation = rotation;
  calculateViewMatrix();
  extractFrustumPlanes();
}

/*
*/
void
Camera::setRotator(const Rotator& rotator) {
  m_rotation = rotator.toQuaternion();
  calculateViewMatrix();
  extractFrustumPlanes();
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

  Vector3 forward = (target - m_position).getNormalized();

  Vector3 right = upVector.cross(forward).getNormalized();

  Vector3 up = forward.cross(right).getNormalized();

  m_viewMatrix = LookAtMatrix(m_position, target, upVector);

  Matrix4 rotationMatrix(
      Vector4(right.x, up.x, forward.x, 0.0f),
      Vector4(right.y, up.y, forward.y, 0.0f),
      Vector4(right.z, up.z, forward.z, 0.0f),
      Vector4(0.0f, 0.0f, 0.0f, 1.0f)
  );

  m_rotation = rotationMatrix.toQuaternion();

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
Camera::rotate(float pitch, float yaw, float roll) {
  CH_PAMRAMETER_UNUSED(roll);
  Vector3 viewDirection = m_lookAtPoint - m_position;
  float distance = viewDirection.magnitude();

  Rotator yawRotator(0.0f, yaw, 0.0f);
  Matrix4 yawMatrix = RotationMatrix(yawRotator);
  Vector4 temp = yawMatrix.transformVector(viewDirection);
  viewDirection = Vector3(temp.x, temp.y, temp.z);

  Vector3 forward = viewDirection.getNormalized();
  Vector3 right = Vector3::UP.cross(forward).getNormalized();

  Rotator pitchRotator(pitch, 0.0f, 0.0f);
  Matrix4 pitchMatrix = RotationMatrix(pitchRotator);

  Vector4 temp2 = pitchMatrix.transformVector(viewDirection);
  viewDirection = Vector3(temp2.x, temp2.y, temp2.z);

  forward = viewDirection.getNormalized();

  m_position = m_lookAtPoint - (forward * distance);

  m_viewMatrix = LookAtMatrix(m_position, m_lookAtPoint, Vector3::UP);

  right = Vector3::UP.cross(forward).getNormalized();
  Vector3 up = forward.cross(right).getNormalized();

  Matrix4 rotationMatrix(
      Vector4(right.x, up.x, forward.x, 0.0f), Vector4(right.y, up.y, forward.y, 0.0f),
      Vector4(right.z, up.z, forward.z, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  m_rotation = rotationMatrix.toQuaternion();

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
Camera::calculateViewMatrix() {
  m_viewMatrix = LookAtMatrix(m_position, m_lookAtPoint, Vector3::UP);
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
Camera::calculateOrthographicMatrix() {
  // Calculate orthographic view bounds
  float halfHeight = m_orthographicSize;
  float halfWidth = halfHeight * getAspectRatio();

  // Create orthographic projection matrix
  // We'll have to implement a similar matrix by hand since there's no
  // OrthographicMatrix class in the engine

  float fRange = 1.0f / (m_farClip - m_nearClip);

  m_projectionMatrix = Matrix4(Vector4(1.0f / halfWidth, 0.0f, 0.0f, 0.0f),
                               Vector4(0.0f, 1.0f / halfHeight, 0.0f, 0.0f),
                               Vector4(0.0f, 0.0f, fRange, 0.0f),
                               Vector4(0.0f, 0.0f, -fRange * m_nearClip, 1.0f));
}

/*
*/
void
Camera::extractFrustumPlanes() {
  // Get the combined view-projection matrix
  Matrix4 viewProj = getViewProjectionMatrix();

  // Left plane
  m_frustumPlanes[0] = Plane(viewProj[0][3] + viewProj[0][0],
                             viewProj[1][3] + viewProj[1][0],
                             viewProj[2][3] + viewProj[2][0],
                             viewProj[3][3] + viewProj[3][0]);

  // Right plane
  m_frustumPlanes[1] = Plane(viewProj[0][3] - viewProj[0][0],
                             viewProj[1][3] - viewProj[1][0],
                             viewProj[2][3] - viewProj[2][0],
                             viewProj[3][3] - viewProj[3][0]);

  // Bottom plane
  m_frustumPlanes[2] = Plane(viewProj[0][3] + viewProj[0][1],
                             viewProj[1][3] + viewProj[1][1],
                             viewProj[2][3] + viewProj[2][1],
                             viewProj[3][3] + viewProj[3][1]);

  // Top plane
  m_frustumPlanes[3] = Plane(viewProj[0][3] - viewProj[0][1],
                             viewProj[1][3] - viewProj[1][1],
                             viewProj[2][3] - viewProj[2][1],
                             viewProj[3][3] - viewProj[3][1]);

  // Near plane
  m_frustumPlanes[4] = Plane(viewProj[0][3] + viewProj[0][2],
                             viewProj[1][3] + viewProj[1][2],
                             viewProj[2][3] + viewProj[2][2],
                             viewProj[3][3] + viewProj[3][2]);

  // Far plane
  m_frustumPlanes[5] = Plane(viewProj[0][3] - viewProj[0][2],
                             viewProj[1][3] - viewProj[1][2],
                             viewProj[2][3] - viewProj[2][2],
                             viewProj[3][3] - viewProj[3][2]);

  // Normalize all planes
  for (int i = 0; i < 6; ++i) {
    float length = Math::sqrt(m_frustumPlanes[i].x * m_frustumPlanes[i].x +
                              m_frustumPlanes[i].y * m_frustumPlanes[i].y +
                              m_frustumPlanes[i].z * m_frustumPlanes[i].z);

    m_frustumPlanes[i].x /= length;
    m_frustumPlanes[i].y /= length;
    m_frustumPlanes[i].z /= length;
    m_frustumPlanes[i].w /= length;
  }
}

/*
*/
bool
Camera::isPointInFrustum(const Vector3& point) const {
  // Test against all 6 frustum planes
  for (int i = 0; i < 6; ++i) {
    if (m_frustumPlanes[i].planeDot(point) < 0) {
      return false;
    }
  }

  return true;
}

/*
*/
bool
Camera::isSphereInFrustum(const Vector3& center, float radius) const {
  // Test against all 6 frustum planes
  for (int i = 0; i < 6; ++i) {
    float distance = m_frustumPlanes[i].planeDot(center);
    if (distance < -radius) {
      return false;
    }
  }

  return true;
}

/*
*/
bool
Camera::isBoxInFrustum(const AABox& box) const {
  // For each plane
  for (int i = 0; i < 6; ++i) {
    // Calculate the box's positive vertex (the vertex furthest in the direction of the normal)
    Vector3 positiveVertex = box.minPoint;

    if (m_frustumPlanes[i].x >= 0) {
      positiveVertex.x = box.maxPoint.x;
    }

    if (m_frustumPlanes[i].y >= 0) {
      positiveVertex.y = box.maxPoint.y;
    }

    if (m_frustumPlanes[i].z >= 0) {
      positiveVertex.z = box.maxPoint.z;
    }

    // If the positive vertex is outside the plane, the box is outside the frustum
    if (m_frustumPlanes[i].planeDot(positiveVertex) < 0) {
      return false;
    }
  }

  return true;
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
  ndcPos.z = -1.0f;  // Near plane
  ndcPos.w = 1.0f;

  // Get the inverse view-projection matrix
  Matrix4 invViewProj = (m_projectionMatrix * m_viewMatrix).getInverse();

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
