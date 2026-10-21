/************************************************************************/
/**
 * @file chPlane.h
 * @author AccelMR
 * @date 2022/06/09
 * @brief Infinite plane in 3D space.
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

#include "chVector3.h"

namespace chEngineSDK {
/**
 * Holds a plane as the points p where normal . p = w. This differs from the common
 * normal . p + d = 0: w is -d. With a unit normal, w is the distance from the origin
 * along the normal.
 *
 * The normal is a member, not a base class, so a plane is never taken for a point.
 */
class Plane
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Plane() = default;

  FORCEINLINE constexpr
  Plane(const Vector3& inNormal, float inW) noexcept;

  FORCEINLINE constexpr
  Plane(float normalX, float normalY, float normalZ, float inW) noexcept;

  /**
   * Plane through point with the given normal, which should be unit length.
   */
  FORCEINLINE constexpr
  Plane(const Vector3& point, const Vector3& inNormal) noexcept;

  /**
   * Plane through three points; the normal is (p2 - p1) x (p3 - p1), made unit length.
   * Points on one line give a zero normal.
   */
  FORCEINLINE
  Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3) noexcept;

  /**
   * Signed distance from point to the plane when the normal is unit length: positive on
   * the side the normal points to.
   */
  NODISCARD FORCEINLINE constexpr float
  planeDot(const Vector3& point) const noexcept;

 public:
  Vector3 normal;
  float w;
};

static_assert(std::is_trivially_copyable_v<Plane>);
static_assert(sizeof(Plane) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Plane::Plane(const Vector3& inNormal, float inW) noexcept
 : normal(inNormal),
   w(inW)
{}

/*
 */
FORCEINLINE constexpr
Plane::Plane(float normalX, float normalY, float normalZ, float inW) noexcept
 : normal(normalX, normalY, normalZ),
   w(inW)
{}

/*
 */
FORCEINLINE constexpr
Plane::Plane(const Vector3& point, const Vector3& inNormal) noexcept
 : normal(inNormal),
   w(point.dot(inNormal))
{}

/*
 */
FORCEINLINE
Plane::Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3) noexcept
 : normal((p2 - p1).cross(p3 - p1).getNormalized()),
   w(p1.dot(normal))
{}

/*
 */
FORCEINLINE constexpr float
Plane::planeDot(const Vector3& point) const noexcept
{
  return normal.dot(point) - w;
}
} // namespace chEngineSDK
