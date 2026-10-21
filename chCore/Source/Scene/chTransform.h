/************************************************************************/
/**
 * @file chTransform.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  Position, rotation and scale of a GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chMatrix4.h"
#include "chQuaternion.h"
#include "chVector3.h"

namespace chEngineSDK {

/**
 * Exists so a GameObject keeps its placement relative to its parent and the scene knows
 * which world matrices to rebuild. Setters only mark it dirty; Scene::updateTransforms
 * rebuilds the world matrix of every dirty object (and of its children) once per frame, so
 * several changes in a frame cost one rebuild.
 */
class CH_CORE_EXPORT Transform
{
 public:
  Transform() = default;

  void
  setLocalPosition(const Vector3& position);

  NODISCARD FORCEINLINE const Vector3&
  getLocalPosition() const { return m_localPosition; }

  void
  setLocalRotation(const Quaternion& rotation);

  NODISCARD FORCEINLINE const Quaternion&
  getLocalRotation() const { return m_localRotation; }

  void
  setLocalScale(const Vector3& scale);

  NODISCARD FORCEINLINE const Vector3&
  getLocalScale() const { return m_localScale; }

  NODISCARD Matrix4
  getLocalMatrix() const;

  /**
   * As of the last Scene::updateTransforms.
   */
  NODISCARD FORCEINLINE const Matrix4&
  getWorldMatrix() const { return m_worldMatrix; }

  NODISCARD Vector3
  getWorldPosition() const;

  NODISCARD FORCEINLINE bool
  isDirty() const { return m_dirty; }

 private:
  friend class GameObject;

  void
  updateWorldMatrix(const Matrix4& parentWorldMatrix);

  Vector3 m_localPosition = Vector3::ZERO;
  Quaternion m_localRotation = Quaternion::IDENTITY;
  Vector3 m_localScale = Vector3::UNIT;
  Matrix4 m_worldMatrix = Matrix4::IDENTITY;
  bool m_dirty = true;
};

} // namespace chEngineSDK
