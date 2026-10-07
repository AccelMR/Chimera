/************************************************************************/
/**
 * @file chQuaternion.h
 * @author AccelMR
 * @date 2022/03/29
 * @brief Rotation stored as a unit quaternion.
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
#include "chVector3.h"

namespace chEngineSDK {
/**
 * Holds a rotation as (x, y, z, w), with (x, y, z) the vector part and w the scalar part.
 * Rotations compose without the gimbal lock of Euler angles.
 *
 * Composition goes right to left, unlike Matrix4: C = A * B first applies B, then A.
 *
 * The class is not exported, only its functions that live in the .cpp, so IDENTITY is
 * inline constexpr.
 */
class Quaternion
{
 public:
  /**
   * Leaves the values uninitialized. Use Quaternion::IDENTITY for no rotation.
   */
  Quaternion() = default;

  FORCEINLINE constexpr
  Quaternion(float inX, float inY, float inZ, float inW) noexcept;

  /**
   * Same rotation as RotationMatrix(rotator).
   */
  CH_UTILITY_EXPORT explicit
  Quaternion(const Rotator& rotator) noexcept;

  /**
   * Takes the rotation of a matrix with no scale.
   */
  CH_UTILITY_EXPORT explicit
  Quaternion(const Matrix4& matrix) noexcept;

  /**
   * Rotation of angle around axis, which does not need to be unit length. An axis of
   * length close to zero gives IDENTITY.
   */
  CH_UTILITY_EXPORT
  Quaternion(const Vector3& axis, const Degree& angle) noexcept;

  NODISCARD CH_UTILITY_EXPORT Rotator
  toRotator() const noexcept;

  NODISCARD FORCEINLINE constexpr float
  squaredLength() const noexcept;

  NODISCARD FORCEINLINE float
  length() const noexcept;

  /**
   * Makes the length 1. Returns false and leaves the quaternion as it was when its
   * squared length is not above tolerance.
   */
  FORCEINLINE bool
  normalize(float tolerance = Math::SMALL_NUMBER) noexcept;

  /**
   * Returns the quaternion with length 1, or IDENTITY when its squared length is not
   * above tolerance.
   */
  NODISCARD FORCEINLINE Quaternion
  getNormalized(float tolerance = Math::SMALL_NUMBER) const noexcept;

  FORCEINLINE constexpr void
  conjugate() noexcept;

  /**
   * The inverse rotation of a unit quaternion, cheaper than getInverse.
   */
  NODISCARD FORCEINLINE constexpr Quaternion
  getConjugated() const noexcept;

  /**
   * Inverse of any quaternion. Returns IDENTITY when it has no inverse.
   */
  NODISCARD FORCEINLINE Quaternion
  getInverse() const noexcept;

  /**
   * Rotates v. The quaternion must be unit length.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  rotateVector(const Vector3& v) const noexcept;

  /**
   * Applies the inverse rotation to v. The quaternion must be unit length.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  unrotateVector(const Vector3& v) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  containsNaN() const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  nearEqual(const Quaternion& other, float tolerance = Math::SMALL_NUMBER) const noexcept;

  /**
   * True when both give the same rotation, also when one is the negation of the other.
   */
  NODISCARD FORCEINLINE constexpr bool
  isRotationEqual(const Quaternion& other,
                  float tolerance = Math::SMALL_NUMBER) const noexcept;

  NODISCARD constexpr bool
  operator==(const Quaternion& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr Quaternion
  operator+(const Quaternion& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Quaternion
  operator*(float scalar) const noexcept;

  /**
   * Composes the rotations: the result first applies other, then this one.
   */
  NODISCARD FORCEINLINE constexpr Quaternion
  operator*(const Quaternion& other) const noexcept;

  FORCEINLINE constexpr Quaternion&
  operator*=(float scalar) noexcept;

  FORCEINLINE constexpr Quaternion&
  operator*=(const Quaternion& other) noexcept;

 public:
  static const Quaternion IDENTITY;

  float x;
  float y;
  float z;
  float w;
};

static_assert(std::is_trivially_copyable_v<Quaternion>);
static_assert(sizeof(Quaternion) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Quaternion::Quaternion(float inX, float inY, float inZ, float inW) noexcept
 : x(inX),
   y(inY),
   z(inZ),
   w(inW)
{}

inline constexpr Quaternion Quaternion::IDENTITY{0.0f, 0.0f, 0.0f, 1.0f};

/*
 */
FORCEINLINE constexpr float
Quaternion::squaredLength() const noexcept
{
  return x * x + y * y + z * z + w * w;
}

/*
 */
FORCEINLINE float
Quaternion::length() const noexcept
{
  return Math::sqrt(squaredLength());
}

/*
 */
FORCEINLINE bool
Quaternion::normalize(float tolerance) noexcept
{
  const float squareLength = squaredLength();
  if (squareLength <= tolerance) {
    return false;
  }
  *this *= Math::invSqrt(squareLength);
  return true;
}

/*
 */
FORCEINLINE Quaternion
Quaternion::getNormalized(float tolerance) const noexcept
{
  const float squareLength = squaredLength();
  if (squareLength <= tolerance) {
    return IDENTITY;
  }
  return *this * Math::invSqrt(squareLength);
}

/*
 */
FORCEINLINE constexpr void
Quaternion::conjugate() noexcept
{
  x = -x;
  y = -y;
  z = -z;
}

/*
 */
FORCEINLINE constexpr Quaternion
Quaternion::getConjugated() const noexcept
{
  return {-x, -y, -z, w};
}

/*
 */
FORCEINLINE Quaternion
Quaternion::getInverse() const noexcept
{
  const float squareLength = squaredLength();
  if (squareLength < Math::SMALL_NUMBER) {
    return IDENTITY;
  }
  const float scale = 1.0f / squareLength;
  return {-x * scale, -y * scale, -z * scale, w * scale};
}

/*
 */
FORCEINLINE constexpr Vector3
Quaternion::rotateVector(const Vector3& v) const noexcept
{
  // v + 2w(q x v) + 2q x (q x v), written with t = 2(q x v) to save a cross product.
  const Vector3 q(x, y, z);
  const Vector3 t = q.cross(v) * 2.0f;
  return v + t * w + q.cross(t);
}

/*
 */
FORCEINLINE constexpr Vector3
Quaternion::unrotateVector(const Vector3& v) const noexcept
{
  return getConjugated().rotateVector(v);
}

/*
 */
FORCEINLINE constexpr bool
Quaternion::containsNaN() const noexcept
{
  return !Math::isFinite(x) || !Math::isFinite(y) || !Math::isFinite(z) ||
         !Math::isFinite(w);
}

/*
 */
FORCEINLINE constexpr bool
Quaternion::nearEqual(const Quaternion& other, float tolerance) const noexcept
{
  return Math::abs(other.x - x) <= tolerance && Math::abs(other.y - y) <= tolerance &&
         Math::abs(other.z - z) <= tolerance && Math::abs(other.w - w) <= tolerance;
}

/*
 */
FORCEINLINE constexpr bool
Quaternion::isRotationEqual(const Quaternion& other, float tolerance) const noexcept
{
  const float dot = x * other.x + y * other.y + z * other.z + w * other.w;
  return Math::abs(dot) >= 1.0f - tolerance;
}

/*
 */
FORCEINLINE constexpr Quaternion
Quaternion::operator+(const Quaternion& other) const noexcept
{
  return {x + other.x, y + other.y, z + other.z, w + other.w};
}

/*
 */
FORCEINLINE constexpr Quaternion
Quaternion::operator*(float scalar) const noexcept
{
  return {x * scalar, y * scalar, z * scalar, w * scalar};
}

/*
 */
FORCEINLINE constexpr Quaternion
Quaternion::operator*(const Quaternion& other) const noexcept
{
  return {w * other.x + x * other.w + y * other.z - z * other.y,
          w * other.y - x * other.z + y * other.w + z * other.x,
          w * other.z + x * other.y - y * other.x + z * other.w,
          w * other.w - x * other.x - y * other.y - z * other.z};
}

/*
 */
FORCEINLINE constexpr Quaternion&
Quaternion::operator*=(float scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}

/*
 */
FORCEINLINE constexpr Quaternion&
Quaternion::operator*=(const Quaternion& other) noexcept
{
  *this = *this * other;
  return *this;
}
} // namespace chEngineSDK
