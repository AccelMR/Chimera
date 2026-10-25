/************************************************************************/
/**
 * @file chTransform.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Position, rotation and scale of a GameObject.
 */
/************************************************************************/
#include "chTransform.h"

#include "chMatrixHelpers.h"
#include "chRotator.h"
#include "chStringUtils.h"

namespace chEngineSDK {

/*
 */
void
Transform::setLocalPosition(const Vector3& position)
{
  m_localPosition = position;
  m_dirty = true;
}

/*
 */
void
Transform::setLocalRotation(const Quaternion& rotation)
{
  m_localRotation = rotation;
  m_dirty = true;
}

/*
 */
void
Transform::setLocalScale(const Vector3& scale)
{
  m_localScale = scale;
  m_dirty = true;
}

/*
 */
Matrix4
Transform::getLocalMatrix() const
{
  return ScaleRotationTranslationMatrix(m_localScale, m_localRotation, m_localPosition);
}

/*
 */
Vector3
Transform::getWorldPosition() const
{
  return Vector3(m_worldMatrix[3][0], m_worldMatrix[3][1], m_worldMatrix[3][2]);
}

/*
 */
String
Transform::toString() const
{
  // Angles read better than the quaternion when debugging.
  return StringUtils::format("Transform(position ({:.3f}, {:.3f}, {:.3f}), rotation {}, "
                             "scale ({:.3f}, {:.3f}, {:.3f}))",
                             m_localPosition.x, m_localPosition.y, m_localPosition.z,
                             m_localRotation.toRotator(), m_localScale.x, m_localScale.y,
                             m_localScale.z);
}

/*
 */
void
Transform::updateWorldMatrix(const Matrix4& parentWorldMatrix)
{
  // Row vectors: the local transform applies first, then the parent's.
  m_worldMatrix = getLocalMatrix() * parentWorldMatrix;
  m_dirty = false;
}

} // namespace chEngineSDK
