/************************************************************************/
/**
 * @file chBox2D.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief Axis-aligned rectangle in 2D space.
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
#include "chVector2.h"

namespace chEngineSDK {
/**
 * Holds a rectangle aligned with the axes, for screen areas and 2D bounds.
 */
class Box2D
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Box2D() = default;

  FORCEINLINE constexpr
  Box2D(const Vector2& inMin, const Vector2& inMax) noexcept;

  /**
   * Smallest rectangle that holds every point. No points gives a rectangle of size zero
   * at the origin.
   */
  FORCEINLINE explicit
  Box2D(const Vector<Vector2>& points) noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  getCenter() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  getSize() const noexcept;

  /**
   * Half the size: the distance from the center to each edge.
   */
  NODISCARD FORCEINLINE constexpr Vector2
  getExtent() const noexcept;

  /**
   * Moves both corners inside other, so this rectangle ends up within it.
   */
  FORCEINLINE constexpr void
  clamp(const Box2D& other) noexcept;

  /**
   * Grows the rectangle to hold point.
   */
  FORCEINLINE constexpr Box2D&
  operator+=(const Vector2& point) noexcept;

 public:
  Vector2 minPoint;
  Vector2 maxPoint;
};

static_assert(std::is_trivially_copyable_v<Box2D>);
static_assert(sizeof(Box2D) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Box2D::Box2D(const Vector2& inMin, const Vector2& inMax) noexcept
 : minPoint(inMin),
   maxPoint(inMax)
{}

/*
 */
FORCEINLINE
Box2D::Box2D(const Vector<Vector2>& points) noexcept
 : minPoint(Vector2::ZERO),
   maxPoint(Vector2::ZERO)
{
  if (points.empty()) {
    return;
  }

  // Starting at the first point, not at zero, so the rectangle does not reach the origin.
  minPoint = points[0];
  maxPoint = points[0];
  for (const Vector2& point : points) {
    *this += point;
  }
}

/*
 */
FORCEINLINE constexpr Vector2
Box2D::getCenter() const noexcept
{
  return (minPoint + maxPoint) * 0.5f;
}

/*
 */
FORCEINLINE constexpr Vector2
Box2D::getSize() const noexcept
{
  return maxPoint - minPoint;
}

/*
 */
FORCEINLINE constexpr Vector2
Box2D::getExtent() const noexcept
{
  return getSize() * 0.5f;
}

/*
 */
FORCEINLINE constexpr void
Box2D::clamp(const Box2D& other) noexcept
{
  minPoint.x = Math::clamp(minPoint.x, other.minPoint.x, other.maxPoint.x);
  minPoint.y = Math::clamp(minPoint.y, other.minPoint.y, other.maxPoint.y);
  maxPoint.x = Math::clamp(maxPoint.x, other.minPoint.x, other.maxPoint.x);
  maxPoint.y = Math::clamp(maxPoint.y, other.minPoint.y, other.maxPoint.y);
}

/*
 */
FORCEINLINE constexpr Box2D&
Box2D::operator+=(const Vector2& point) noexcept
{
  minPoint.x = Math::min(minPoint.x, point.x);
  minPoint.y = Math::min(minPoint.y, point.y);
  maxPoint.x = Math::max(maxPoint.x, point.x);
  maxPoint.y = Math::max(maxPoint.y, point.y);
  return *this;
}
} // namespace chEngineSDK
