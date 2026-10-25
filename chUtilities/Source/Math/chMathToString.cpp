/************************************************************************/
/**
 * @file chMathToString.cpp
 * @author AccelMR
 * @date 2026/10/25
 * @brief formatTo and toString of the math classes, used by logs and script tostring.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chAngle.h"
#include "chBox.h"
#include "chBox2D.h"
#include "chCapsule.h"
#include "chFrustum.h"
#include "chLinearColor.h"
#include "chMatrix4.h"
#include "chOrientedBox.h"
#include "chPlane.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chRotator.h"
#include "chSphere.h"
#include "chSphereBoxBounds.h"
#include "chStringUtils.h"
#include "chVector2.h"
#include "chVector2I.h"
#include "chVector3.h"
#include "chVector3I.h"
#include "chVector4.h"

namespace chEngineSDK {
namespace {

constexpr int32 kDefaultDecimals = 3;
constexpr int32 kQuaternionDecimals = 4;
constexpr int32 kDegreeDecimals = 2;

void
appendList(String& output, const FormatSpec& spec, std::initializer_list<float> values)
{
  bool bFirst = true;
  for (const float value : values) {
    if (!bFirst) {
      output += ", ";
    }
    StringUtils::appendFloat(output, value, spec);
    bFirst = false;
  }
}

// Angles computed from a quaternion are often exactly -0, which would print as -0.00.
float
withoutNegativeZero(const Degree angle)
{
  return angle.valueDegree() + 0.0f;
}

} // namespace

/*
 */
void
Vector2::formatTo(String& output, const FormatSpec& spec) const
{
  output += "Vector2(";
  appendList(output, spec.getComponentSpec(kDefaultDecimals), {x, y});
  output += ')';
}

/*
 */
String
Vector2::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Vector3::formatTo(String& output, const FormatSpec& spec) const
{
  output += "Vector3(";
  appendList(output, spec.getComponentSpec(kDefaultDecimals), {x, y, z});
  output += ')';
}

/*
 */
String
Vector3::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Vector4::formatTo(String& output, const FormatSpec& spec) const
{
  output += "Vector4(";
  appendList(output, spec.getComponentSpec(kDefaultDecimals), {x, y, z, w});
  output += ')';
}

/*
 */
String
Vector4::toString() const
{
  return StringUtils::toString(*this);
}

/*
 * Integer vectors ignore the precision of the spec.
 */
void
Vector2I::formatTo(String& output, const FormatSpec& spec) const
{
  FormatSpec number = spec;
  number.width = 0;
  output += "Vector2I(";
  StringUtils::appendInteger(output, x, number);
  output += ", ";
  StringUtils::appendInteger(output, y, number);
  output += ')';
}

/*
 */
String
Vector2I::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Vector3I::formatTo(String& output, const FormatSpec& spec) const
{
  FormatSpec number = spec;
  number.width = 0;
  output += "Vector3I(";
  StringUtils::appendInteger(output, x, number);
  output += ", ";
  StringUtils::appendInteger(output, y, number);
  output += ", ";
  StringUtils::appendInteger(output, z, number);
  output += ')';
}

/*
 */
String
Vector3I::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Radian::formatTo(String& output, const FormatSpec& spec) const
{
  StringUtils::appendFloat(output, m_radian, spec.getComponentSpec(kQuaternionDecimals));
  output += " rad";
}

/*
 */
String
Radian::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Degree::formatTo(String& output, const FormatSpec& spec) const
{
  StringUtils::appendFloat(output, m_degree, spec.getComponentSpec(kDegreeDecimals));
  output += " deg";
}

/*
 */
String
Degree::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Quaternion::formatTo(String& output, const FormatSpec& spec) const
{
  output += "Quaternion(";
  appendList(output, spec.getComponentSpec(kQuaternionDecimals), {x, y, z, w});
  output += ')';
}

/*
 */
String
Quaternion::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Rotator::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDegreeDecimals);
  output += "Rotator(pitch ";
  StringUtils::appendFloat(output, withoutNegativeZero(pitch), number);
  output += ", yaw ";
  StringUtils::appendFloat(output, withoutNegativeZero(yaw), number);
  output += ", roll ";
  StringUtils::appendFloat(output, withoutNegativeZero(roll), number);
  output += ')';
}

/*
 */
String
Rotator::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Matrix4::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Matrix4(";
  for (int32 row = 0; row < 4; ++row) {
    output += row == 0 ? "[" : ", [";
    const float* values = m_data[row];
    appendList(output, number, {values[0], values[1], values[2], values[3]});
    output += ']';
  }
  output += ')';
}

/*
 */
String
Matrix4::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
LinearColor::formatTo(String& output, const FormatSpec& spec) const
{
  output += "LinearColor(";
  appendList(output, spec.getComponentSpec(kDefaultDecimals), {r, g, b, a});
  output += ')';
}

/*
 */
String
LinearColor::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Plane::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Plane(normal (";
  appendList(output, number, {normal.x, normal.y, normal.z});
  output += "), w ";
  StringUtils::appendFloat(output, w, number);
  output += ')';
}

/*
 */
String
Plane::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
AABox::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "AABox(min (";
  appendList(output, number, {minPoint.x, minPoint.y, minPoint.z});
  output += "), max (";
  appendList(output, number, {maxPoint.x, maxPoint.y, maxPoint.z});
  output += "))";
}

/*
 */
String
AABox::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Sphere::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Sphere(center (";
  appendList(output, number, {center.x, center.y, center.z});
  output += "), radius ";
  StringUtils::appendFloat(output, radius, number);
  output += ')';
}

/*
 */
String
Sphere::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
SphereBoxBounds::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "SphereBoxBounds(center (";
  appendList(output, number, {center.x, center.y, center.z});
  output += "), extent (";
  appendList(output, number, {boxExtent.x, boxExtent.y, boxExtent.z});
  output += "), radius ";
  StringUtils::appendFloat(output, sphereRadius, number);
  output += ')';
}

/*
 */
String
SphereBoxBounds::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Box2D::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Box2D(min (";
  appendList(output, number, {minPoint.x, minPoint.y});
  output += "), max (";
  appendList(output, number, {maxPoint.x, maxPoint.y});
  output += "))";
}

/*
 */
String
Box2D::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Ray::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Ray(origin (";
  appendList(output, number, {origin.x, origin.y, origin.z});
  output += "), direction (";
  appendList(output, number, {direction.x, direction.y, direction.z});
  output += "))";
}

/*
 */
String
Ray::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
OrientedBox::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "OrientedBox(center (";
  appendList(output, number, {center.x, center.y, center.z});
  output += "), extent (";
  appendList(output, number, {extent.x, extent.y, extent.z});
  // Angles read better than the quaternion when debugging.
  output += "), rotation ";
  FormatSpec angleSpec = spec;
  angleSpec.width = 0;
  rotation.toRotator().formatTo(output, angleSpec);
  output += ')';
}

/*
 */
String
OrientedBox::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Capsule::formatTo(String& output, const FormatSpec& spec) const
{
  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Capsule(start (";
  appendList(output, number, {start.x, start.y, start.z});
  output += "), end (";
  appendList(output, number, {end.x, end.y, end.z});
  output += "), radius ";
  StringUtils::appendFloat(output, radius, number);
  output += ')';
}

/*
 */
String
Capsule::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
void
Frustum::formatTo(String& output, const FormatSpec& spec) const
{
  static constexpr const ANSICHAR* kSideNames[] = {"left", "right", "bottom", "top", "near",
                                                   "far"};
  static_assert(sizeof(kSideNames) / sizeof(kSideNames[0]) ==
                static_cast<SIZE_T>(FrustumSide::Count));

  const FormatSpec number = spec.getComponentSpec(kDefaultDecimals);
  output += "Frustum(";
  for (SIZE_T side = 0; side < static_cast<SIZE_T>(FrustumSide::Count); ++side) {
    const Plane& plane = planes[side];
    if (side > 0) {
      output += ", ";
    }
    output += kSideNames[side];
    output += " (";
    appendList(output, number, {plane.normal.x, plane.normal.y, plane.normal.z, plane.w});
    output += ')';
  }
  output += ')';
}

/*
 */
String
Frustum::toString() const
{
  return StringUtils::toString(*this);
}

} // namespace chEngineSDK
