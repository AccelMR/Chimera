/************************************************************************/
/**
 * @file chTransform.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  Transform component for handling position, rotation and scale.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chVector3.h"
#include "chQuaternion.h"
#include "chMatrix4.h"

namespace chEngineSDK {

class CH_CORE_EXPORT Transform
{
 public:
  /**
   * Default constructor
   */
  Transform();

  /**
   * Constructor with initial values
   *
   * @param position Initial position
   * @param rotation Initial rotation
   * @param scale Initial scale
   */
  Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);

  /**
   * Set the local position
   *
   * @param position New local position
   */
  void
  setLocalPosition(const Vector3& position);

  /**
   * Get the local position
   *
   * @return Local position
   */
  NODISCARD const Vector3&
  getLocalPosition() const { return m_localPosition; }

  /**
   * Set the local rotation
   *
   * @param rotation New local rotation
   */
  void
  setLocalRotation(const Quaternion& rotation);

  /**
   * Get the local rotation
   *
   * @return Local rotation
   */
  NODISCARD const Quaternion&
  getLocalRotation() const { return m_localRotation; }

  /**
   * Set the local scale
   *
   * @param scale New local scale
   */
  void
  setLocalScale(const Vector3& scale);

  /**
   * Get the local scale
   *
   * @return Local scale
   */
  NODISCARD const Vector3&
  getLocalScale() const { return m_localScale; }

  /**
   * Set the world position
   *
   * @param position New world position
   */
  void
  setWorldPosition(const Vector3& position);

  /**
   * Get the world position
   *
   * @return World position
   */
  NODISCARD Vector3
  getWorldPosition() const;

  /**
   * Set the world rotation
   *
   * @param rotation New world rotation
   */
  void
  setWorldRotation(const Quaternion& rotation);

  /**
   * Get the world rotation
   *
   * @return World rotation
   */
  NODISCARD Quaternion
  getWorldRotation() const;

  /**
   * Set the world scale
   *
   * @param scale New world scale
   */
  void
  setWorldScale(const Vector3& scale);

  /**
   * Get the world scale
   *
   * @return World scale
   */
  NODISCARD Vector3
  getWorldScale() const;

  /**
   * Get the local transformation matrix
   *
   * @return Local transformation matrix
   */
  NODISCARD Matrix4
  getLocalMatrix() const;

  /**
   * Get the world transformation matrix
   *
   * @return World transformation matrix
   */
  NODISCARD Matrix4
  getWorldMatrix() const;

  /**
   * Set the parent transform
   *
   * @param parent Parent transform
   */
  void
  setParent(Transform* parent) { m_parent = parent; }

  /**
   * Get the parent transform
   *
   * @return Parent transform or nullptr if no parent
   */
  NODISCARD Transform*
  getParent() const { return m_parent; }

  /**
   * Update the transform
   */
  void
  update();

 private:
  Vector3 m_localPosition;
  Quaternion m_localRotation;
  Vector3 m_localScale;

  Transform* m_parent = nullptr;

  bool m_dirty = true;
  Matrix4 m_localMatrix;
  Matrix4 m_worldMatrix;
};

} // namespace chEngineSDK
