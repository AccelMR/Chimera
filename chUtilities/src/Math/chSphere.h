/************************************************************************/
/**
 * @file chSphere.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief Sphere in 3D space.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <type_traits>

#include "chMath.h"
#include "chVector3.h"

namespace chEngineSDK {
/**
 * Holds a sphere, the bounds whose overlap tests cost one squared distance and that
 * stay the same when the object rotates. For overlap tests see ShapeOverlap.
 */
class Sphere
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Sphere() = default;

  FORCEINLINE constexpr
  Sphere(const Vector3& inCenter, float inRadius) noexcept;

  /**
   * Sphere centered on the box around the points that holds every point. Not the
   * smallest possible sphere, but close and cheap. No points gives radius zero at the
   * origin.
   */
  FORCEINLINE explicit
  Sphere(const Vector<Vector3>& points) noexcept;

 public:
  Vector3 center;
  float radius;
};

static_assert(std::is_trivially_copyable_v<Sphere>);
static_assert(sizeof(Sphere) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Sphere::Sphere(const Vector3& inCenter, float inRadius) noexcept
 : center(inCenter),
   radius(inRadius)
{}

/*
 */
FORCEINLINE
Sphere::Sphere(const Vector<Vector3>& points) noexcept
 : center(Vector3::ZERO),
   radius(0.0f)
{
  if (points.empty()) {
    return;
  }

  Vector3 minPoint = points[0];
  Vector3 maxPoint = points[0];
  for (const Vector3& point : points) {
    minPoint.x = Math::min(minPoint.x, point.x);
    minPoint.y = Math::min(minPoint.y, point.y);
    minPoint.z = Math::min(minPoint.z, point.z);
    maxPoint.x = Math::max(maxPoint.x, point.x);
    maxPoint.y = Math::max(maxPoint.y, point.y);
    maxPoint.z = Math::max(maxPoint.z, point.z);
  }
  center = (minPoint + maxPoint) * 0.5f;

  float maxSquareDistance = 0.0f;
  for (const Vector3& point : points) {
    maxSquareDistance = Math::max(maxSquareDistance, point.sqrDistance(center));
  }

  // A little larger, so the farthest point stays inside after the rounding of sqrt.
  radius = Math::sqrt(maxSquareDistance) * 1.001f;
}
} // namespace chEngineSDK
