/************************************************************************/
/**
 * @file chMathToString.cpp
 * @author AccelMR
 * @date 2026/10/25
 * @brief toString of the math classes, used by logs and script tostring.
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

// Angles computed from a quaternion are often exactly -0, which would print as -0.00.
float
withoutNegativeZero(const Degree angle)
{
  return angle.valueDegree() + 0.0f;
}

} // namespace

/*
 */
String
Vector2::toString() const
{
  return StringUtils::format("Vector2({:.3f}, {:.3f})", x, y);
}

/*
 */
String
Vector3::toString() const
{
  return StringUtils::format("Vector3({:.3f}, {:.3f}, {:.3f})", x, y, z);
}

/*
 */
String
Vector4::toString() const
{
  return StringUtils::format("Vector4({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w);
}

/*
 */
String
Vector2I::toString() const
{
  return StringUtils::format("Vector2I({}, {})", x, y);
}

/*
 */
String
Vector3I::toString() const
{
  return StringUtils::format("Vector3I({}, {}, {})", x, y, z);
}

/*
 */
String
Radian::toString() const
{
  return StringUtils::format("{:.4f} rad", m_radian);
}

/*
 */
String
Degree::toString() const
{
  return StringUtils::format("{:.2f} deg", m_degree);
}

/*
 */
String
Quaternion::toString() const
{
  return StringUtils::format("Quaternion({:.4f}, {:.4f}, {:.4f}, {:.4f})", x, y, z, w);
}

/*
 */
String
Rotator::toString() const
{
  return StringUtils::format("Rotator(pitch {:.2f}, yaw {:.2f}, roll {:.2f})",
                             withoutNegativeZero(pitch), withoutNegativeZero(yaw),
                             withoutNegativeZero(roll));
}

/*
 */
String
Matrix4::toString() const
{
  return StringUtils::format("Matrix4([{:.3f}, {:.3f}, {:.3f}, {:.3f}], "
                             "[{:.3f}, {:.3f}, {:.3f}, {:.3f}], "
                             "[{:.3f}, {:.3f}, {:.3f}, {:.3f}], "
                             "[{:.3f}, {:.3f}, {:.3f}, {:.3f}])",
                             m_data[0][0], m_data[0][1], m_data[0][2], m_data[0][3],
                             m_data[1][0], m_data[1][1], m_data[1][2], m_data[1][3],
                             m_data[2][0], m_data[2][1], m_data[2][2], m_data[2][3],
                             m_data[3][0], m_data[3][1], m_data[3][2], m_data[3][3]);
}

/*
 */
String
LinearColor::toString() const
{
  return StringUtils::format("LinearColor({:.3f}, {:.3f}, {:.3f}, {:.3f})", r, g, b, a);
}

/*
 */
String
Plane::toString() const
{
  return StringUtils::format("Plane(normal ({:.3f}, {:.3f}, {:.3f}), w {:.3f})", normal.x,
                             normal.y, normal.z, w);
}

/*
 */
String
AABox::toString() const
{
  return StringUtils::format("AABox(min ({:.3f}, {:.3f}, {:.3f}), "
                             "max ({:.3f}, {:.3f}, {:.3f}))",
                             minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y,
                             maxPoint.z);
}

/*
 */
String
Sphere::toString() const
{
  return StringUtils::format("Sphere(center ({:.3f}, {:.3f}, {:.3f}), radius {:.3f})",
                             center.x, center.y, center.z, radius);
}

/*
 */
String
SphereBoxBounds::toString() const
{
  return StringUtils::format("SphereBoxBounds(center ({:.3f}, {:.3f}, {:.3f}), "
                             "extent ({:.3f}, {:.3f}, {:.3f}), radius {:.3f})",
                             center.x, center.y, center.z, boxExtent.x, boxExtent.y,
                             boxExtent.z, sphereRadius);
}

/*
 */
String
Box2D::toString() const
{
  return StringUtils::format("Box2D(min ({:.3f}, {:.3f}), max ({:.3f}, {:.3f}))", minPoint.x,
                             minPoint.y, maxPoint.x, maxPoint.y);
}

/*
 */
String
Ray::toString() const
{
  return StringUtils::format("Ray(origin ({:.3f}, {:.3f}, {:.3f}), "
                             "direction ({:.3f}, {:.3f}, {:.3f}))",
                             origin.x, origin.y, origin.z, direction.x, direction.y,
                             direction.z);
}

/*
 */
String
OrientedBox::toString() const
{
  // Angles read better than the quaternion when debugging.
  const Rotator angles = rotation.toRotator();
  return StringUtils::format("OrientedBox(center ({:.3f}, {:.3f}, {:.3f}), "
                             "extent ({:.3f}, {:.3f}, {:.3f}), "
                             "rotation (pitch {:.2f}, yaw {:.2f}, roll {:.2f}))",
                             center.x, center.y, center.z, extent.x, extent.y, extent.z,
                             withoutNegativeZero(angles.pitch),
                             withoutNegativeZero(angles.yaw),
                             withoutNegativeZero(angles.roll));
}

/*
 */
String
Capsule::toString() const
{
  return StringUtils::format("Capsule(start ({:.3f}, {:.3f}, {:.3f}), "
                             "end ({:.3f}, {:.3f}, {:.3f}), radius {:.3f})",
                             start.x, start.y, start.z, end.x, end.y, end.z, radius);
}

/*
 */
String
Frustum::toString() const
{
  const Plane& left = planes[static_cast<SIZE_T>(FrustumSide::Left)];
  const Plane& right = planes[static_cast<SIZE_T>(FrustumSide::Right)];
  const Plane& bottom = planes[static_cast<SIZE_T>(FrustumSide::Bottom)];
  const Plane& top = planes[static_cast<SIZE_T>(FrustumSide::Top)];
  const Plane& nearPlane = planes[static_cast<SIZE_T>(FrustumSide::Near)];
  const Plane& farPlane = planes[static_cast<SIZE_T>(FrustumSide::Far)];
  return StringUtils::format(
      "Frustum(left ({:.3f}, {:.3f}, {:.3f}, {:.3f}), right ({:.3f}, {:.3f}, {:.3f}, {:.3f}), "
      "bottom ({:.3f}, {:.3f}, {:.3f}, {:.3f}), top ({:.3f}, {:.3f}, {:.3f}, {:.3f}), "
      "near ({:.3f}, {:.3f}, {:.3f}, {:.3f}), far ({:.3f}, {:.3f}, {:.3f}, {:.3f}))",
      left.normal.x, left.normal.y, left.normal.z, left.w, right.normal.x, right.normal.y,
      right.normal.z, right.w, bottom.normal.x, bottom.normal.y, bottom.normal.z, bottom.w,
      top.normal.x, top.normal.y, top.normal.z, top.w, nearPlane.normal.x,
      nearPlane.normal.y, nearPlane.normal.z, nearPlane.w, farPlane.normal.x,
      farPlane.normal.y, farPlane.normal.z, farPlane.w);
}

} // namespace chEngineSDK
