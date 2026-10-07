/************************************************************************/
/**
 * @file chRay.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Half line in 3D space.
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
 * Holds a ray: it starts at origin and goes on forever along direction. Picking and line
 * of sight use it with RayCast.
 *
 * The direction must be unit length, so a hit distance is a distance in world units.
 */
class Ray
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Ray() = default;

  FORCEINLINE constexpr
  Ray(const Vector3& inOrigin, const Vector3& inDirection) noexcept;

  /**
   * The point at distance along the ray.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  getPoint(float distance) const noexcept;

 public:
  Vector3 origin;
  Vector3 direction;
};

static_assert(std::is_trivially_copyable_v<Ray>);
static_assert(sizeof(Ray) == 24);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Ray::Ray(const Vector3& inOrigin, const Vector3& inDirection) noexcept
 : origin(inOrigin),
   direction(inDirection)
{}

/*
 */
FORCEINLINE constexpr Vector3
Ray::getPoint(float distance) const noexcept
{
  return origin + direction * distance;
}
} // namespace chEngineSDK
