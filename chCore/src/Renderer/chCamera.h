/************************************************************************/
/**
 * @file chCamera.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"
#include "chVector3.h"
#include "chQuaternion.h"
#include "chMatrix4.h"
#include "chRadian.h"
#include "chDegree.h"
#include "chRotator.h"

namespace chEngineSDK {

/**
 * Enum to specify the projection type used by the camera
 */
enum class CameraProjectionType {
  Perspective,
  Orthographic
};

/**
 * Class that represents a camera in 3D space
 */
class CH_UTILITY_EXPORT Camera
{
public:
  /**
   * Default constructor
   */
  Camera();

  /**
   * Construct a camera with specific position and target
   * 
   * @param position Initial camera position
   * @param target Point the camera will look at
   * @param upVector Up direction for the camera
   */
  Camera(const Vector3& position, 
         const Vector3& target,
         float viewPortWidth,
         float viewPortHeight,
         const Vector3& upVector = Vector3::UP);

  /**
   * Destructor
   */
  ~Camera() = default;

  /**
   * Set camera position in world space
   * 
   * @param position New position vector
   */
  void 
  setPosition(const Vector3& position);

  /**
   * Get camera position in world space
   * 
   * @return Current camera position
   */
  NODISCARD FORCEINLINE const Vector3& 
  getPosition() const { return m_position; }

  /**
   * Set camera rotation as quaternion
   * 
   * @param rotation New rotation quaternion
   */
  FORCEINLINE void 
  setRotation(const Quaternion& rotation);

  /**
   * Get camera rotation as quaternion
   * 
   * @return Current camera rotation
   */
  NODISCARD FORCEINLINE const Quaternion& 
  getRotation() const { return m_rotation; }

  /**
   * Set camera rotation using Euler angles
   * 
   * @param rotator Rotation angles (pitch, yaw, roll)
   */
  FORCEINLINE void 
  setRotator(const Rotator& rotator);

  /**
   * Get camera rotation as Euler angles
   * 
   * @return Current rotation as Rotator
   */
  NODISCARD FORCEINLINE Rotator 
  getRotator() const;

  /**
   * Set the camera to look at a specific target
   * 
   * @param target Point to look at
   * @param upVector Up direction (default: world up)
   */
  void 
  lookAt(const Vector3& target, const Vector3& upVector = Vector3::UP);

  /**
   * Set the camera's field of view (for perspective projection)
   * 
   * @param fov Field of view in degrees
   */
  void 
  setFieldOfView(Radian fov);

  /**
   * Get the camera's field of view in degrees
   * 
   * @return Current field of view
   */
  NODISCARD FORCEINLINE float 
  getFieldOfView() const { return m_fieldOfView.valueDegree(); }

  /**
   * Set the camera's viewport size
   * 
   * @param width Width of the viewport
   * @param height Height of the viewport
   */
  void
  setViewportSize(float width, float height);

  /**
   * Get the camera's aspect ratio
   * 
   * @return Current aspect ratio
   */
  NODISCARD FORCEINLINE float 
  getAspectRatio() const { return m_width / m_height; }

  /**
   * Set the near and far clip planes
   * 
   * @param nearPlane Distance to near clip plane
   * @param farPlane Distance to far clip plane
   */
  void 
  setClipPlanes(float nearPlane, float farPlane);

  /**
   * Get the distance to the near clip plane
   * 
   * @return Near clip distance
   */
  NODISCARD FORCEINLINE float 
  getNearClipPlane() const { return m_nearClip; }

  /**
   * Get the distance to the far clip plane
   * 
   * @return Far clip distance
   */
  NODISCARD FORCEINLINE float 
  getFarClipPlane() const { return m_farClip; }

  /**
   * Set the projection type (perspective or orthographic)
   * 
   * @param type Projection type to use
   */
  FORCEINLINE void 
  setProjectionType(CameraProjectionType type) { m_projectionType = type; }

  /**
   * Get the current projection type
   * 
   * @return Current projection type
   */
  NODISCARD FORCEINLINE CameraProjectionType 
  getProjectionType() const { return m_projectionType; }

  /**
   * Set orthographic size for orthographic projection
   * 
   * @param size Size of the orthographic viewport
   */
  FORCEINLINE void 
  setOrthographicSize(float size) { m_orthographicSize = size; }

  /**
   * Get the orthographic size
   * 
   * @return Current orthographic size
   */
  NODISCARD FORCEINLINE float 
  getOrthographicSize() const { return m_orthographicSize; }

  /**
   * Move the camera forward by the specified distance
   * 
   * @param distance Distance to move
   */
  void 
  moveForward(float distance);

  /**
   * Move the camera right by the specified distance
   * 
   * @param distance Distance to move
   */
  void 
  moveRight(float distance);

  /**
   * Move the camera up by the specified distance
   * 
   * @param distance Distance to move
   */
  void 
  moveUp(float distance);

  /**
   * Pan the camera by the specified delta values
   * 
   * @param deltaX Change in X direction
   * @param deltaY Change in Y direction
   */
  void 
  pan(float deltaX, float deltaY);

  /**
   * Rotate the camera by the specified Euler angles
   * 
   * @param pitch Pitch angle in degrees
   * @param yaw Yaw angle in degrees
   * @param roll Roll angle in degrees
   */
  void 
  rotate(float pitch, float yaw, float roll);

  /**
   * Update the camera's view and projection matrices
   * Call this after changing camera parameters
   */
  void 
  updateMatrices();

  NODISCARD FORCEINLINE Vector3
  getLookAt() const { return m_lookAtPoint; }

  FORCEINLINE void
  setLookAtPoint(const Vector3& lookAt) { m_lookAtPoint = lookAt; }

  /**
   * Get the view matrix (world to camera space)
   * 
   * @return View matrix
   */
  NODISCARD FORCEINLINE const Matrix4& 
  getViewMatrix() const { return m_viewMatrix; }

  /**
   * Get the projection matrix (camera to clip space)
   * 
   * @return Projection matrix
   */
  NODISCARD FORCEINLINE const Matrix4& 
  getProjectionMatrix() const { return m_projectionMatrix; }

  /**
   * Get the combined view-projection matrix
   * 
   * @return View-projection matrix
   */
  NODISCARD FORCEINLINE Matrix4 
  getViewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }

  /**
   * Get the forward direction vector
   * 
   * @return Forward direction (normalized)
   */
  NODISCARD FORCEINLINE Vector3 
  getForwardVector() const;

  /**
   * Get the right direction vector
   * 
   * @return Right direction (normalized)
   */
  NODISCARD FORCEINLINE Vector3 
  getRightVector() const;

  /**
   * Get the up direction vector
   * 
   * @return Up direction (normalized)
   */
  NODISCARD FORCEINLINE Vector3 
  getUpVector() const;

  /**
   * Check if a point is in the camera's view frustum
   * 
   * @param point Point to check
   * @return True if the point is in the frustum
   */
  NODISCARD bool 
  isPointInFrustum(const Vector3& point) const;

  /**
   * Check if a sphere is in the camera's view frustum
   * 
   * @param center Sphere center
   * @param radius Sphere radius
   * @return True if the sphere is at least partially in the frustum
   */
  NODISCARD bool 
  isSphereInFrustum(const Vector3& center, float radius) const;

  /**
   * Check if a box is in the camera's view frustum
   * 
   * @param box Axis-aligned bounding box
   * @return True if the box is at least partially in the frustum
   */
  NODISCARD bool 
  isBoxInFrustum(const AABox& box) const;

  /**
   * Convert a world space position to screen space coordinates
   * 
   * @param worldPos Position in world space
   * @return Vector2 with screen coordinates (x,y in range 0-1)
   */
  NODISCARD Vector2 
  worldToScreenPoint(const Vector3& worldPos) const;

  /**
   * Unproject a screen position to a world space ray
   * 
   * @param screenPos Screen position (x,y in range 0-1)
   * @param rayOrigin [out] Ray origin
   * @param rayDirection [out] Ray direction
   */
  void 
  screenToWorldRay(const Vector2& screenPos, Vector3& rayOrigin, Vector3& rayDirection) const;

private:
  /**
   * Calculate perspective projection matrix
   */
  void 
  calculatePerspectiveMatrix();

  /**
   * Calculate orthographic projection matrix
   */
  void 
  calculateOrthographicMatrix();

  /**
   * Calculate view matrix
   */
  void 
  calculateViewMatrix();

  /**
   * Extract frustum planes from view-projection matrix
   */
  void 
  extractFrustumPlanes();

private:
  // Position and orientation
  Vector3 m_position;
  Quaternion m_rotation;

  // Projection parameters
  Radian m_fieldOfView;
  float m_nearClip;
  float m_farClip;
  float m_orthographicSize;
  float m_width;
  float m_height;
  CameraProjectionType m_projectionType;
  Vector3 m_lookAtPoint;

  // Cached matrices
  Matrix4 m_viewMatrix;
  Matrix4 m_projectionMatrix;

  // Frustum planes (left, right, top, bottom, near, far)
  Array<Plane, 6> m_frustumPlanes;
};
} // namespace chEngineSDK