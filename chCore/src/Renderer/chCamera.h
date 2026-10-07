/************************************************************************/
/**
 * @file chCamera.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief Camera with a view, a projection and their frustum.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAngle.h"
#include "chFrustum.h"
#include "chMatrix4.h"
#include "chVector3.h"

namespace chEngineSDK {

enum class CameraProjectionType
{
  Perspective,
  Orthographic
};

/**
 * Holds where a camera is, where it looks and how it projects, and gives the matrices and
 * frustum that rendering and culling need.
 *
 * Setters only store the value; the view, the projection, view * projection and the
 * frustum are rebuilt by the first getter that needs them, so any number of changes in a
 * frame costs one rebuild. World Z is always up: the camera never rolls.
 */
class CH_CORE_EXPORT Camera
{
 public:
  Camera() = default;

  Camera(const Vector3& position, const Vector3& target, float viewportWidth,
         float viewportHeight);

  FORCEINLINE void
  setPosition(const Vector3& position)
  {
    m_position = position;
    markViewDirty();
  }

  NODISCARD FORCEINLINE const Vector3&
  getPosition() const
  {
    return m_position;
  }

  FORCEINLINE void
  lookAt(const Vector3& target)
  {
    m_lookAtPoint = target;
    markViewDirty();
  }

  NODISCARD FORCEINLINE const Vector3&
  getLookAt() const
  {
    return m_lookAtPoint;
  }

  /**
   * Turns the camera in place by moving the look at point. Roll is dropped.
   */
  void
  setRotation(const Quaternion& rotation);

  void
  setRotator(const Rotator& rotator);

  /**
   * Computed from the view on each call; the camera does not store a rotation.
   */
  NODISCARD Quaternion
  getRotation() const;

  NODISCARD Rotator
  getRotator() const;

  FORCEINLINE void
  setFieldOfView(Radian fieldOfView)
  {
    m_fieldOfView = fieldOfView;
    markProjectionDirty();
  }

  /**
   * The horizontal field of view.
   */
  NODISCARD FORCEINLINE Radian
  getFieldOfView() const
  {
    return m_fieldOfView;
  }

  FORCEINLINE void
  setViewportSize(float width, float height)
  {
    m_width = width;
    m_height = height;
    markProjectionDirty();
  }

  NODISCARD FORCEINLINE float
  getAspectRatio() const
  {
    return m_width / m_height;
  }

  FORCEINLINE void
  setClipPlanes(float nearPlane, float farPlane)
  {
    m_nearClip = nearPlane;
    m_farClip = farPlane;
    markProjectionDirty();
  }

  NODISCARD FORCEINLINE float
  getNearClipPlane() const
  {
    return m_nearClip;
  }

  NODISCARD FORCEINLINE float
  getFarClipPlane() const
  {
    return m_farClip;
  }

  FORCEINLINE void
  setProjectionType(CameraProjectionType type)
  {
    m_projectionType = type;
    markProjectionDirty();
  }

  NODISCARD FORCEINLINE CameraProjectionType
  getProjectionType() const
  {
    return m_projectionType;
  }

  /**
   * Half the visible height of the orthographic projection, in world units.
   */
  FORCEINLINE void
  setOrthographicSize(float halfHeight)
  {
    m_orthographicSize = halfHeight;
    markProjectionDirty();
  }

  NODISCARD FORCEINLINE float
  getOrthographicSize() const
  {
    return m_orthographicSize;
  }

  /**
   * Moves the camera and its look at point along the view direction.
   */
  void
  moveForward(float distance);

  void
  moveRight(float distance);

  /**
   * Moves along world up, not the view's up.
   */
  void
  moveUp(float distance);

  /**
   * Moves the camera and its look at point across the view.
   */
  void
  pan(float deltaX, float deltaY);

  /**
   * Orbits around the look at point by these degrees. Pitch stops at 89 degrees up or
   * down, where the view would flip.
   */
  void
  rotate(float pitchDegrees, float yawDegrees);

  NODISCARD FORCEINLINE const Matrix4&
  getViewMatrix() const
  {
    if (m_viewDirty) {
      updateView();
    }
    return m_viewMatrix;
  }

  NODISCARD FORCEINLINE const Matrix4&
  getProjectionMatrix() const
  {
    if (m_projectionDirty) {
      updateProjection();
    }
    return m_projectionMatrix;
  }

  NODISCARD FORCEINLINE const Matrix4&
  getViewProjectionMatrix() const
  {
    if (m_viewProjectionDirty) {
      updateViewProjection();
    }
    return m_viewProjectionMatrix;
  }

  NODISCARD FORCEINLINE const Frustum&
  getFrustum() const
  {
    if (m_viewProjectionDirty) {
      updateViewProjection();
    }
    return m_frustum;
  }

  /**
   * The view's axes in world space, read from the view matrix.
   */
  NODISCARD Vector3
  getForwardVector() const;

  NODISCARD Vector3
  getRightVector() const;

  NODISCARD Vector3
  getUpVector() const;

  /**
   * Where a world point lands on screen, from (0, 0) at the top left to (1, 1) at the
   * bottom right. Returns false for a point behind the camera.
   */
  NODISCARD bool
  worldToScreenPoint(const Vector3& worldPoint, Vector2& outScreenPoint) const;

  /**
   * The ray from the near plane through a screen point given as in worldToScreenPoint.
   */
  NODISCARD Ray
  screenToWorldRay(const Vector2& screenPoint) const;

 private:
  FORCEINLINE void
  markViewDirty()
  {
    m_viewDirty = true;
    m_viewProjectionDirty = true;
  }

  FORCEINLINE void
  markProjectionDirty()
  {
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
  }

  void
  updateView() const;

  void
  updateProjection() const;

  void
  updateViewProjection() const;

  Vector3 m_position = Vector3::ZERO;
  Vector3 m_lookAtPoint = Vector3::FORWARD;
  Radian m_fieldOfView = Radian(Degree(60.0f));
  float m_nearClip = 0.1f;
  float m_farClip = 1000.0f;
  float m_orthographicSize = 5.0f;
  float m_width = 800.0f;
  float m_height = 600.0f;
  CameraProjectionType m_projectionType = CameraProjectionType::Perspective;

  // Built on demand by the getters above.
  mutable bool m_viewDirty = true;
  mutable bool m_projectionDirty = true;
  mutable bool m_viewProjectionDirty = true;
  mutable Matrix4 m_viewMatrix;
  mutable Matrix4 m_projectionMatrix;
  mutable Matrix4 m_viewProjectionMatrix;
  mutable Frustum m_frustum;
};
} // namespace chEngineSDK
