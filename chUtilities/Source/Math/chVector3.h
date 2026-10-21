/************************************************************************/
/**
 * @file chVector3.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/10/31
 * @brief Vector with three float components.
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
 * Holds a point or a direction in 3D space.
 *
 * No vector class includes another, so the math headers stay independent; convert
 * between them by components. The class is not exported, so the constants are inline
 * constexpr and other modules fold them at compile time.
 */
class Vector3
{
 public:
  /**
   * Leaves the values uninitialized, so a vector filled right after is written once.
   * Use Vector3::ZERO or Vector3{} when it must start at zero.
   */
  Vector3() = default;

  FORCEINLINE constexpr
  Vector3(float inX, float inY, float inZ) noexcept;

  FORCEINLINE explicit constexpr
  Vector3(const float values[3]) noexcept;

  NODISCARD FORCEINLINE constexpr float
  dot(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  cross(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  getAbs() const noexcept;

  NODISCARD FORCEINLINE float
  magnitude() const noexcept;

  NODISCARD FORCEINLINE constexpr float
  sqrMagnitude() const noexcept;

  NODISCARD FORCEINLINE float
  distance(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr float
  sqrDistance(const Vector3& other) const noexcept;

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
  NODISCARD FORCEINLINE Vector3
  getNormalized(float tolerance = Math::SMALL_NUMBER) const noexcept;

  /**
   * Projects this vector onto other, which must not be zero.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  projection(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  nearEqual(const Vector3& other, float tolerance = Math::SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator==(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  operator+(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  operator-(const Vector3& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  operator*(float scalar) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  operator/(float scalar) const noexcept;

  FORCEINLINE constexpr Vector3&
  operator+=(const Vector3& other) noexcept;

  FORCEINLINE constexpr Vector3&
  operator-=(const Vector3& other) noexcept;

  FORCEINLINE constexpr Vector3&
  operator*=(float scalar) noexcept;

 public:
  static const Vector3 ZERO;
  static const Vector3 UNIT;
  static const Vector3 FORWARD;
  static const Vector3 BACKWARD;
  static const Vector3 RIGHT;
  static const Vector3 LEFT;
  static const Vector3 UP;
  static const Vector3 DOWN;

  float x;
  float y;
  float z;
};

static_assert(std::is_trivially_copyable_v<Vector3>);
static_assert(sizeof(Vector3) == 12);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Vector3::Vector3(float inX, float inY, float inZ) noexcept
 : x(inX),
   y(inY),
   z(inZ)
{}

/*
 */
FORCEINLINE constexpr
Vector3::Vector3(const float values[3]) noexcept
 : x(values[0]),
   y(values[1]),
   z(values[2])
{}

inline constexpr Vector3 Vector3::ZERO{0.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::UNIT{1.0f, 1.0f, 1.0f};
inline constexpr Vector3 Vector3::FORWARD{1.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::BACKWARD{-1.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::RIGHT{0.0f, 1.0f, 0.0f};
inline constexpr Vector3 Vector3::LEFT{0.0f, -1.0f, 0.0f};
inline constexpr Vector3 Vector3::UP{0.0f, 0.0f, 1.0f};
inline constexpr Vector3 Vector3::DOWN{0.0f, 0.0f, -1.0f};

/*
 */
FORCEINLINE constexpr float
Vector3::dot(const Vector3& other) const noexcept
{
  return x * other.x + y * other.y + z * other.z;
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::cross(const Vector3& other) const noexcept
{
  return {y * other.z - z * other.y,
          z * other.x - x * other.z,
          x * other.y - y * other.x};
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::getAbs() const noexcept
{
  return {Math::abs(x), Math::abs(y), Math::abs(z)};
}

/*
 */
FORCEINLINE float
Vector3::magnitude() const noexcept
{
  return Math::sqrt(sqrMagnitude());
}

/*
 */
FORCEINLINE constexpr float
Vector3::sqrMagnitude() const noexcept
{
  return dot(*this);
}

/*
 */
FORCEINLINE float
Vector3::distance(const Vector3& other) const noexcept
{
  return Math::sqrt(sqrDistance(other));
}

/*
 */
FORCEINLINE constexpr float
Vector3::sqrDistance(const Vector3& other) const noexcept
{
  return (*this - other).sqrMagnitude();
}

/*
 */
FORCEINLINE bool
Vector3::normalize(float tolerance) noexcept
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
FORCEINLINE Vector3
Vector3::getNormalized(float tolerance) const noexcept
{
  const float squareLength = sqrMagnitude();
  if (squareLength <= tolerance) {
    return ZERO;
  }
  return *this * Math::invSqrt(squareLength);
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::projection(const Vector3& other) const noexcept
{
  return other * (dot(other) / other.dot(other));
}

/*
 */
FORCEINLINE constexpr bool
Vector3::nearEqual(const Vector3& other, float tolerance) const noexcept
{
  return Math::abs(other.x - x) <= tolerance && Math::abs(other.y - y) <= tolerance &&
         Math::abs(other.z - z) <= tolerance;
}

/*
 */
FORCEINLINE constexpr bool
Vector3::operator==(const Vector3& other) const noexcept
{
  return x == other.x && y == other.y && z == other.z;
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::operator+(const Vector3& other) const noexcept
{
  return {x + other.x, y + other.y, z + other.z};
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::operator-(const Vector3& other) const noexcept
{
  return {x - other.x, y - other.y, z - other.z};
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::operator-() const noexcept
{
  return {-x, -y, -z};
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::operator*(float scalar) const noexcept
{
  return {x * scalar, y * scalar, z * scalar};
}

/*
 */
FORCEINLINE constexpr Vector3
Vector3::operator/(float scalar) const noexcept
{
  // One division and three multiplications are cheaper than three divisions; the result
  // can differ from a true division in the last bit.
  const float inverse = 1.0f / scalar;
  return {x * inverse, y * inverse, z * inverse};
}

/*
 */
FORCEINLINE constexpr Vector3&
Vector3::operator+=(const Vector3& other) noexcept
{
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector3&
Vector3::operator-=(const Vector3& other) noexcept
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector3&
Vector3::operator*=(float scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Vector3
operator*(float scalar, const Vector3& vector) noexcept
{
  return vector * scalar;
}
} // namespace chEngineSDK
