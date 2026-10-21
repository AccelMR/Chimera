/************************************************************************/
/**
 * @file chSphereBoxBounds.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief Bounds made of a box and a sphere that share a center.
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

#include "chBox.h"
#include "chMath.h"
#include "chSphere.h"

namespace chEngineSDK {
class Matrix4;

/**
 * Holds a box and a sphere with the same center, so culling can run the cheap sphere
 * test first and the tighter box test only when the sphere passes.
 */
class SphereBoxBounds
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  SphereBoxBounds() = default;

  FORCEINLINE constexpr
  SphereBoxBounds(const Vector3& inCenter, const Vector3& inBoxExtent,
                  float inSphereRadius) noexcept;

  /**
   * Box around the points and the smallest sphere with the same center that holds them.
   */
  FORCEINLINE explicit
  SphereBoxBounds(const Vector<Vector3>& points) noexcept;

  /**
   * Takes the center and extent of the box; the radius is the smaller of the box's
   * half diagonal and what the sphere needs to reach from that center.
   */
  FORCEINLINE
  SphereBoxBounds(const AABox& box, const Sphere& sphere) noexcept;

  FORCEINLINE explicit
  SphereBoxBounds(const AABox& box) noexcept;

  FORCEINLINE explicit constexpr
  SphereBoxBounds(const Sphere& sphere) noexcept;

  NODISCARD FORCEINLINE constexpr AABox
  getBox() const noexcept;

  NODISCARD FORCEINLINE constexpr Sphere
  getSphere() const noexcept;

  /**
   * Bounds of the same space after matrix (scale, rotation, translation). The box is exact
   * for the old box; the sphere grows by the largest axis scale.
   */
  NODISCARD CH_UTILITY_EXPORT SphereBoxBounds
  getTransformed(const Matrix4& matrix) const noexcept;


 public:
  Vector3 center;
  Vector3 boxExtent;
  float sphereRadius;
};

static_assert(std::is_trivially_copyable_v<SphereBoxBounds>);
static_assert(sizeof(SphereBoxBounds) == 28);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
SphereBoxBounds::SphereBoxBounds(const Vector3& inCenter, const Vector3& inBoxExtent,
                                 float inSphereRadius) noexcept
 : center(inCenter),
   boxExtent(inBoxExtent),
   sphereRadius(inSphereRadius)
{}

/*
 */
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const Vector<Vector3>& points) noexcept
{
  const AABox box(points);
  center = box.getCenter();
  boxExtent = box.getExtent();

  float maxSquareDistance = 0.0f;
  for (const Vector3& point : points) {
    maxSquareDistance = Math::max(maxSquareDistance, point.sqrDistance(center));
  }
  sphereRadius = Math::sqrt(maxSquareDistance);
}

/*
 */
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const AABox& box, const Sphere& sphere) noexcept
 : center(box.getCenter()),
   boxExtent(box.getExtent())
{
  sphereRadius =
      Math::min(boxExtent.magnitude(), sphere.center.distance(center) + sphere.radius);
}

/*
 */
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const AABox& box) noexcept
 : center(box.getCenter()),
   boxExtent(box.getExtent())
{
  sphereRadius = boxExtent.magnitude();
}

/*
 */
FORCEINLINE constexpr
SphereBoxBounds::SphereBoxBounds(const Sphere& sphere) noexcept
 : center(sphere.center),
   boxExtent(sphere.radius, sphere.radius, sphere.radius),
   sphereRadius(sphere.radius)
{}

/*
 */
FORCEINLINE constexpr AABox
SphereBoxBounds::getBox() const noexcept
{
  return {center - boxExtent, center + boxExtent};
}

/*
 */
FORCEINLINE constexpr Sphere
SphereBoxBounds::getSphere() const noexcept
{
  return {center, sphereRadius};
}
} // namespace chEngineSDK
