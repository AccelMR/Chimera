/************************************************************************/
/**
 * @file chVector2I.h
 * @author AccelMR
 * @date 2022/09/19
 * @brief Vector with two int32 components.
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

namespace chEngineSDK {
/**
 * Holds a 2D position or size in whole units, such as pixels or grid cells.
 *
 * No vector class includes another; convert between them by components. The class is
 * not exported, so the constants are inline constexpr.
 */
class Vector2I
{
 public:
  /**
   * Leaves the values uninitialized. Use Vector2I::ZERO or Vector2I{} for zero.
   */
  Vector2I() = default;

  FORCEINLINE constexpr
  Vector2I(int32 inX, int32 inY) noexcept;

  NODISCARD constexpr bool
  operator==(const Vector2I& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr Vector2I
  operator+(const Vector2I& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2I
  operator-(const Vector2I& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2I
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2I
  operator*(int32 scalar) const noexcept;

  FORCEINLINE constexpr Vector2I&
  operator+=(const Vector2I& other) noexcept;

  FORCEINLINE constexpr Vector2I&
  operator-=(const Vector2I& other) noexcept;

  FORCEINLINE constexpr Vector2I&
  operator*=(int32 scalar) noexcept;

 public:
  static const Vector2I ZERO;
  static const Vector2I UNIT;
  static const Vector2I UNIT_X;
  static const Vector2I UNIT_Y;

  int32 x;
  int32 y;
};

static_assert(std::is_trivially_copyable_v<Vector2I>);
static_assert(sizeof(Vector2I) == 8);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Vector2I::Vector2I(int32 inX, int32 inY) noexcept
 : x(inX),
   y(inY)
{}

inline constexpr Vector2I Vector2I::ZERO{0, 0};
inline constexpr Vector2I Vector2I::UNIT{1, 1};
inline constexpr Vector2I Vector2I::UNIT_X{1, 0};
inline constexpr Vector2I Vector2I::UNIT_Y{0, 1};

/*
 */
FORCEINLINE constexpr Vector2I
Vector2I::operator+(const Vector2I& other) const noexcept
{
  return {x + other.x, y + other.y};
}

/*
 */
FORCEINLINE constexpr Vector2I
Vector2I::operator-(const Vector2I& other) const noexcept
{
  return {x - other.x, y - other.y};
}

/*
 */
FORCEINLINE constexpr Vector2I
Vector2I::operator-() const noexcept
{
  return {-x, -y};
}

/*
 */
FORCEINLINE constexpr Vector2I
Vector2I::operator*(int32 scalar) const noexcept
{
  return {x * scalar, y * scalar};
}

/*
 */
FORCEINLINE constexpr Vector2I&
Vector2I::operator+=(const Vector2I& other) noexcept
{
  x += other.x;
  y += other.y;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector2I&
Vector2I::operator-=(const Vector2I& other) noexcept
{
  x -= other.x;
  y -= other.y;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector2I&
Vector2I::operator*=(int32 scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Vector2I
operator*(int32 scalar, const Vector2I& vector) noexcept
{
  return vector * scalar;
}
} // namespace chEngineSDK
