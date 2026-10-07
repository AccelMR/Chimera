/************************************************************************/
/**
 * @file chRotator.h
 * @author AccelMR
 * @date 2022/03/17
 * @brief Rotation stored as pitch, yaw and roll in degrees.
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

#include "chAngle.h"
#include "chMath.h"

namespace chEngineSDK {
/**
 * Holds a rotation as Euler angles, the form people read and type in an editor. Positive
 * pitch turns forward up, positive yaw turns it right, positive roll turns right down.
 * Use Quaternion to compose rotations.
 *
 * The class is not exported, only its functions that live in the .cpp, so ZERO is inline
 * constexpr.
 */
class Rotator
{
 public:
  /**
   * Leaves the values uninitialized. Use Rotator::ZERO for no rotation.
   */
  Rotator() = default;

  FORCEINLINE constexpr
  Rotator(const Degree& inPitch, const Degree& inYaw, const Degree& inRoll) noexcept;

  FORCEINLINE constexpr
  Rotator(float inPitch, float inYaw, float inRoll) noexcept;

  NODISCARD CH_UTILITY_EXPORT Quaternion
  toQuaternion() const noexcept;

  /**
   * Wraps every angle into [-180, 180].
   */
  FORCEINLINE void
  normalize() noexcept;

  NODISCARD FORCEINLINE Rotator
  getNormalized() const noexcept;

  /**
   * Wraps every angle into [0, 360).
   */
  FORCEINLINE void
  denormalize() noexcept;

  NODISCARD FORCEINLINE Rotator
  getDenormalized() const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  containsNaN() const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  nearEqual(const Rotator& other,
            float tolerance = Math::KINDA_SMALL_NUMBER) const noexcept;

  NODISCARD constexpr bool
  operator==(const Rotator& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr Rotator
  operator+(const Rotator& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Rotator
  operator-(const Rotator& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Rotator
  operator*(float scalar) const noexcept;

  FORCEINLINE constexpr Rotator&
  operator*=(float scalar) noexcept;

  /**
   * Wraps the angle into [0, 360).
   */
  NODISCARD static FORCEINLINE Degree
  clampAxis(const Degree& angle) noexcept;

  /**
   * Wraps the angle into [-180, 180].
   */
  NODISCARD static FORCEINLINE Degree
  normalizeAxis(const Degree& angle) noexcept;

 public:
  static const Rotator ZERO;

  Degree pitch;
  Degree yaw;
  Degree roll;
};

static_assert(std::is_trivially_copyable_v<Rotator>);
static_assert(sizeof(Rotator) == 12);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Rotator::Rotator(const Degree& inPitch, const Degree& inYaw, const Degree& inRoll) noexcept
 : pitch(inPitch),
   yaw(inYaw),
   roll(inRoll)
{}

/*
 */
FORCEINLINE constexpr
Rotator::Rotator(float inPitch, float inYaw, float inRoll) noexcept
 : pitch(inPitch),
   yaw(inYaw),
   roll(inRoll)
{}

inline constexpr Rotator Rotator::ZERO{0.0f, 0.0f, 0.0f};

/*
 */
FORCEINLINE void
Rotator::normalize() noexcept
{
  pitch = normalizeAxis(pitch);
  yaw = normalizeAxis(yaw);
  roll = normalizeAxis(roll);
}

/*
 */
FORCEINLINE Rotator
Rotator::getNormalized() const noexcept
{
  return {normalizeAxis(pitch), normalizeAxis(yaw), normalizeAxis(roll)};
}

/*
 */
FORCEINLINE void
Rotator::denormalize() noexcept
{
  pitch = clampAxis(pitch);
  yaw = clampAxis(yaw);
  roll = clampAxis(roll);
}

/*
 */
FORCEINLINE Rotator
Rotator::getDenormalized() const noexcept
{
  return {clampAxis(pitch), clampAxis(yaw), clampAxis(roll)};
}

/*
 */
FORCEINLINE constexpr bool
Rotator::containsNaN() const noexcept
{
  return !Math::isFinite(pitch.valueDegree()) || !Math::isFinite(yaw.valueDegree()) ||
         !Math::isFinite(roll.valueDegree());
}

/*
 */
FORCEINLINE constexpr bool
Rotator::nearEqual(const Rotator& other, float tolerance) const noexcept
{
  return Math::abs(other.pitch.valueDegree() - pitch.valueDegree()) <= tolerance &&
         Math::abs(other.yaw.valueDegree() - yaw.valueDegree()) <= tolerance &&
         Math::abs(other.roll.valueDegree() - roll.valueDegree()) <= tolerance;
}

/*
 */
FORCEINLINE constexpr Rotator
Rotator::operator+(const Rotator& other) const noexcept
{
  return {pitch + other.pitch, yaw + other.yaw, roll + other.roll};
}

/*
 */
FORCEINLINE constexpr Rotator
Rotator::operator-(const Rotator& other) const noexcept
{
  return {pitch - other.pitch, yaw - other.yaw, roll - other.roll};
}

/*
 */
FORCEINLINE constexpr Rotator
Rotator::operator*(float scalar) const noexcept
{
  return {pitch * scalar, yaw * scalar, roll * scalar};
}

/*
 */
FORCEINLINE constexpr Rotator&
Rotator::operator*=(float scalar) noexcept
{
  pitch *= scalar;
  yaw *= scalar;
  roll *= scalar;
  return *this;
}

/*
 */
FORCEINLINE Degree
Rotator::clampAxis(const Degree& angle) noexcept
{
  float degrees = Math::fmod(angle.valueDegree(), 360.0f);
  if (degrees < 0.0f) {
    degrees += 360.0f;
    // A tiny negative angle plus 360 rounds to exactly 360.
    if (degrees >= 360.0f) {
      degrees = 0.0f;
    }
  }
  return Degree(degrees);
}

/*
 */
FORCEINLINE Degree
Rotator::normalizeAxis(const Degree& angle) noexcept
{
  return Degree(Math::unwindDegrees(angle.valueDegree()));
}
} // namespace chEngineSDK
