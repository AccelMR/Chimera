/************************************************************************/
/**
 * @file chVector4.h
 * @author AccelMR
 * @date 2022/02/15
 * @brief Vector with four float components.
 *
 * Coordinate system: X = forward, Y = right, Z = up, left-handed.
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
 * Holds four floats: a point (w = 1) or a direction (w = 0) in homogeneous form, or any
 * other 4D value. Every operation uses the four components.
 *
 * No vector class includes another, so the math headers stay independent; convert
 * between them by components. The class is not exported, so the constants are inline
 * constexpr and other modules fold them at compile time.
 */
class Vector4
{
 public:
  /**
   * Leaves the values uninitialized, so a vector filled right after is written once.
   * Use Vector4::ZERO or Vector4{} when it must start at zero.
   */
  Vector4() = default;

  FORCEINLINE constexpr
  Vector4(float inX, float inY, float inZ, float inW) noexcept;

  FORCEINLINE explicit constexpr
  Vector4(const float values[4]) noexcept;

  NODISCARD FORCEINLINE constexpr float
  dot(const Vector4& other) const noexcept;

  /**
   * Cross product of xyz, with w = 0, for directions in homogeneous form.
   */
  NODISCARD FORCEINLINE constexpr Vector4
  cross(const Vector4& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector4
  getAbs() const noexcept;

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
  NODISCARD FORCEINLINE Vector4
  getNormalized(float tolerance = Math::SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  nearEqual(const Vector4& other, float tolerance = Math::SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator==(const Vector4& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector4
  operator+(const Vector4& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector4
  operator-(const Vector4& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector4
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector4
  operator*(float scalar) const noexcept;

  FORCEINLINE constexpr Vector4&
  operator+=(const Vector4& other) noexcept;

  FORCEINLINE constexpr Vector4&
  operator-=(const Vector4& other) noexcept;

  FORCEINLINE constexpr Vector4&
  operator*=(float scalar) noexcept;

 public:
  static const Vector4 ZERO;
  static const Vector4 UNIT;

  float x;
  float y;
  float z;
  float w;
};

static_assert(std::is_trivially_copyable_v<Vector4>);
static_assert(sizeof(Vector4) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Vector4::Vector4(float inX, float inY, float inZ, float inW) noexcept
 : x(inX),
   y(inY),
   z(inZ),
   w(inW)
{}

/*
 */
FORCEINLINE constexpr
Vector4::Vector4(const float values[4]) noexcept
 : x(values[0]),
   y(values[1]),
   z(values[2]),
   w(values[3])
{}

inline constexpr Vector4 Vector4::ZERO{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 Vector4::UNIT{1.0f, 1.0f, 1.0f, 1.0f};

/*
 */
FORCEINLINE constexpr float
Vector4::dot(const Vector4& other) const noexcept
{
  return x * other.x + y * other.y + z * other.z + w * other.w;
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::cross(const Vector4& other) const noexcept
{
  return {y * other.z - z * other.y,
          z * other.x - x * other.z,
          x * other.y - y * other.x,
          0.0f};
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::getAbs() const noexcept
{
  return {Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w)};
}

/*
 */
FORCEINLINE float
Vector4::magnitude() const noexcept
{
  return Math::sqrt(sqrMagnitude());
}

/*
 */
FORCEINLINE constexpr float
Vector4::sqrMagnitude() const noexcept
{
  return dot(*this);
}

/*
 */
FORCEINLINE bool
Vector4::normalize(float tolerance) noexcept
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
FORCEINLINE Vector4
Vector4::getNormalized(float tolerance) const noexcept
{
  const float squareLength = sqrMagnitude();
  if (squareLength <= tolerance) {
    return ZERO;
  }
  return *this * Math::invSqrt(squareLength);
}

/*
 */
FORCEINLINE constexpr bool
Vector4::nearEqual(const Vector4& other, float tolerance) const noexcept
{
  return Math::abs(other.x - x) <= tolerance && Math::abs(other.y - y) <= tolerance &&
         Math::abs(other.z - z) <= tolerance && Math::abs(other.w - w) <= tolerance;
}

/*
 */
FORCEINLINE constexpr bool
Vector4::operator==(const Vector4& other) const noexcept
{
  return x == other.x && y == other.y && z == other.z && w == other.w;
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::operator+(const Vector4& other) const noexcept
{
  return {x + other.x, y + other.y, z + other.z, w + other.w};
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::operator-(const Vector4& other) const noexcept
{
  return {x - other.x, y - other.y, z - other.z, w - other.w};
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::operator-() const noexcept
{
  return {-x, -y, -z, -w};
}

/*
 */
FORCEINLINE constexpr Vector4
Vector4::operator*(float scalar) const noexcept
{
  return {x * scalar, y * scalar, z * scalar, w * scalar};
}

/*
 */
FORCEINLINE constexpr Vector4&
Vector4::operator+=(const Vector4& other) noexcept
{
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector4&
Vector4::operator-=(const Vector4& other) noexcept
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector4&
Vector4::operator*=(float scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Vector4
operator*(float scalar, const Vector4& vector) noexcept
{
  return vector * scalar;
}
} // namespace chEngineSDK
