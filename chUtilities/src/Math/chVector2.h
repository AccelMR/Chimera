/************************************************************************/
/**
 * @file chVector2.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/16
 * @brief Vector with two float components.
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

namespace chEngineSDK {
/**
 * Holds a point or a direction in 2D space.
 *
 * No vector class includes another, so the math headers stay independent; convert
 * between them by components. The class is not exported, so the constants are inline
 * constexpr and other modules fold them at compile time.
 */
class Vector2
{
 public:
  /**
   * Leaves the values uninitialized, so a vector filled right after is written once.
   * Use Vector2::ZERO or Vector2{} when it must start at zero.
   */
  Vector2() = default;

  FORCEINLINE constexpr
  Vector2(float inX, float inY) noexcept;

  FORCEINLINE explicit constexpr
  Vector2(const float values[2]) noexcept;

  NODISCARD FORCEINLINE constexpr float
  dot(const Vector2& other) const noexcept;

  /**
   * Z of the 3D cross product: positive when other is counter-clockwise from this vector
   * (with X right and Y up).
   */
  NODISCARD FORCEINLINE constexpr float
  cross(const Vector2& other) const noexcept;

  NODISCARD FORCEINLINE float
  magnitude() const noexcept;

  NODISCARD FORCEINLINE constexpr float
  sqrMagnitude() const noexcept;

  /**
   * Makes the length 1. Returns false and leaves the vector as it was when its squared
   * length is not above tolerance.
   */
  FORCEINLINE bool
  normalize(float tolerance = Math::SMALL_NUMBER) noexcept;

  /**
   * Returns the vector with length 1, or ZERO when its squared length is not above
   * tolerance.
   */
  NODISCARD FORCEINLINE Vector2
  getNormalized(float tolerance = Math::SMALL_NUMBER) const noexcept;

  /**
   * Projects this vector onto other, which must not be zero.
   */
  NODISCARD FORCEINLINE constexpr Vector2
  projection(const Vector2& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  nearEqual(const Vector2& other, float tolerance = Math::SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator==(const Vector2& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  operator+(const Vector2& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  operator-(const Vector2& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector2
  operator*(float scalar) const noexcept;

  FORCEINLINE constexpr Vector2&
  operator+=(const Vector2& other) noexcept;

  FORCEINLINE constexpr Vector2&
  operator-=(const Vector2& other) noexcept;

  FORCEINLINE constexpr Vector2&
  operator*=(float scalar) noexcept;

 public:
  static const Vector2 ZERO;
  static const Vector2 UNIT;
  static const Vector2 UNIT_X;
  static const Vector2 UNIT_Y;

  float x;
  float y;
};

static_assert(std::is_trivially_copyable_v<Vector2>);
static_assert(sizeof(Vector2) == 8);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Vector2::Vector2(float inX, float inY) noexcept
 : x(inX),
   y(inY)
{}

/*
 */
FORCEINLINE constexpr
Vector2::Vector2(const float values[2]) noexcept
 : x(values[0]),
   y(values[1])
{}

inline constexpr Vector2 Vector2::ZERO{0.0f, 0.0f};
inline constexpr Vector2 Vector2::UNIT{1.0f, 1.0f};
inline constexpr Vector2 Vector2::UNIT_X{1.0f, 0.0f};
inline constexpr Vector2 Vector2::UNIT_Y{0.0f, 1.0f};

/*
 */
FORCEINLINE constexpr float
Vector2::dot(const Vector2& other) const noexcept
{
  return x * other.x + y * other.y;
}

/*
 */
FORCEINLINE constexpr float
Vector2::cross(const Vector2& other) const noexcept
{
  return x * other.y - y * other.x;
}

/*
 */
FORCEINLINE float
Vector2::magnitude() const noexcept
{
  return Math::sqrt(sqrMagnitude());
}

/*
 */
FORCEINLINE constexpr float
Vector2::sqrMagnitude() const noexcept
{
  return dot(*this);
}

/*
 */
FORCEINLINE bool
Vector2::normalize(float tolerance) noexcept
{
  const float squareLength = sqrMagnitude();
  if (squareLength <= tolerance) {
    return false;
  }
  *this *= Math::invSqrt(squareLength);
  return true;
}

/*
 */
FORCEINLINE Vector2
Vector2::getNormalized(float tolerance) const noexcept
{
  const float squareLength = sqrMagnitude();
  if (squareLength <= tolerance) {
    return ZERO;
  }
  return *this * Math::invSqrt(squareLength);
}

/*
 */
FORCEINLINE constexpr Vector2
Vector2::projection(const Vector2& other) const noexcept
{
  return other * (dot(other) / other.dot(other));
}

/*
 */
FORCEINLINE constexpr bool
Vector2::nearEqual(const Vector2& other, float tolerance) const noexcept
{
  return Math::abs(other.x - x) <= tolerance && Math::abs(other.y - y) <= tolerance;
}

/*
 */
FORCEINLINE constexpr bool
Vector2::operator==(const Vector2& other) const noexcept
{
  return x == other.x && y == other.y;
}

/*
 */
FORCEINLINE constexpr Vector2
Vector2::operator+(const Vector2& other) const noexcept
{
  return {x + other.x, y + other.y};
}

/*
 */
FORCEINLINE constexpr Vector2
Vector2::operator-(const Vector2& other) const noexcept
{
  return {x - other.x, y - other.y};
}

/*
 */
FORCEINLINE constexpr Vector2
Vector2::operator-() const noexcept
{
  return {-x, -y};
}

/*
 */
FORCEINLINE constexpr Vector2
Vector2::operator*(float scalar) const noexcept
{
  return {x * scalar, y * scalar};
}

/*
 */
FORCEINLINE constexpr Vector2&
Vector2::operator+=(const Vector2& other) noexcept
{
  x += other.x;
  y += other.y;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector2&
Vector2::operator-=(const Vector2& other) noexcept
{
  x -= other.x;
  y -= other.y;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector2&
Vector2::operator*=(float scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Vector2
operator*(float scalar, const Vector2& vector) noexcept
{
  return vector * scalar;
}
} // namespace chEngineSDK
