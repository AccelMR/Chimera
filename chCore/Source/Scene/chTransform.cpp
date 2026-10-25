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
namespace {

constexpr int32 kDecimals = 3;

void
appendVector(String& output, const Vector3& vector, const FormatSpec& spec)
{
  StringUtils::appendFloat(output, vector.x, spec);
  output += ", ";
  StringUtils::appendFloat(output, vector.y, spec);
  output += ", ";
  StringUtils::appendFloat(output, vector.z, spec);
}

} // namespace

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
void
Transform::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDecimals);
  output += "Transform(position (";
  appendVector(output, m_localPosition, number);
  // Angles read better than the quaternion when debugging.
  output += "), rotation ";
  FormatSpec angleSpec = spec;
  angleSpec.width = 0;
  m_localRotation.toRotator().formatTo(output, angleSpec);
  output += ", scale (";
  appendVector(output, m_localScale, number);
  output += "))";
}

/*
 */
String
Transform::toString() const
{
  return StringUtils::toString(*this);
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
