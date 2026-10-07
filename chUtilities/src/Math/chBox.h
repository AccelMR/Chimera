/************************************************************************/
/**
 * @file chBox.h
 * @author AccelMR
 * @date 2022/06/03
 * @brief Axis-aligned box in 3D space.
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
 * Holds a box aligned with the world axes, the cheapest bounds to build and to test.
 */
class AABox
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  AABox() = default;

  FORCEINLINE constexpr
  AABox(const Vector3& inMin, const Vector3& inMax) noexcept;

  /**
   * Smallest box that holds every point. No points gives a box of size zero at the
   * origin.
   */
  FORCEINLINE explicit
  AABox(const Vector<Vector3>& points) noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  getCenter() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  getSize() const noexcept;

  /**
   * Half the size: the distance from the center to each face.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  getExtent() const noexcept;

  /**
   * Smallest box aligned with the world axes that holds this box after the matrix
   * (scale, rotation and translation, no projection) moves it. Used to take local bounds
   * to world space.
   */
  NODISCARD CH_UTILITY_EXPORT AABox
  getTransformed(const Matrix4& matrix) const noexcept;

  FORCEINLINE constexpr void
  moveTo(const Vector3& center) noexcept;

  FORCEINLINE constexpr void
  shiftBy(const Vector3& offset) noexcept;

  /**
   * Grows the box to hold point.
   */
  FORCEINLINE constexpr AABox&
  operator+=(const Vector3& point) noexcept;

  /**
   * Grows the box to hold other.
   */
  FORCEINLINE constexpr AABox&
  operator+=(const AABox& other) noexcept;

 public:
  Vector3 minPoint;
  Vector3 maxPoint;
};

static_assert(std::is_trivially_copyable_v<AABox>);
static_assert(sizeof(AABox) == 24);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
AABox::AABox(const Vector3& inMin, const Vector3& inMax) noexcept
 : minPoint(inMin),
   maxPoint(inMax)
{}

/*
 */
FORCEINLINE
AABox::AABox(const Vector<Vector3>& points) noexcept
 : minPoint(Vector3::ZERO),
   maxPoint(Vector3::ZERO)
{
  if (points.empty()) {
    return;
  }

  // Starting at the first point, not at zero, so the box does not reach the origin.
  minPoint = points[0];
  maxPoint = points[0];
  for (const Vector3& point : points) {
    *this += point;
  }
}

/*
 */
FORCEINLINE constexpr Vector3
AABox::getCenter() const noexcept
{
  return (minPoint + maxPoint) * 0.5f;
}

/*
 */
FORCEINLINE constexpr Vector3
AABox::getSize() const noexcept
{
  return maxPoint - minPoint;
}

/*
 */
FORCEINLINE constexpr Vector3
AABox::getExtent() const noexcept
{
  return getSize() * 0.5f;
}

/*
 */
FORCEINLINE constexpr void
AABox::moveTo(const Vector3& center) noexcept
{
  shiftBy(center - getCenter());
}

/*
 */
FORCEINLINE constexpr void
AABox::shiftBy(const Vector3& offset) noexcept
{
  minPoint += offset;
  maxPoint += offset;
}

/*
 */
FORCEINLINE constexpr AABox&
AABox::operator+=(const Vector3& point) noexcept
{
  minPoint.x = Math::min(minPoint.x, point.x);
  minPoint.y = Math::min(minPoint.y, point.y);
  minPoint.z = Math::min(minPoint.z, point.z);
  maxPoint.x = Math::max(maxPoint.x, point.x);
  maxPoint.y = Math::max(maxPoint.y, point.y);
  maxPoint.z = Math::max(maxPoint.z, point.z);
  return *this;
}

/*
 */
FORCEINLINE constexpr AABox&
AABox::operator+=(const AABox& other) noexcept
{
  *this += other.minPoint;
  return *this += other.maxPoint;
}
} // namespace chEngineSDK
