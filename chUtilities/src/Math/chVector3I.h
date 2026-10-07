/************************************************************************/
/**
 * @file chVector3I.h
 * @author AccelMR
 * @date 2022/09/19
 * @brief Vector with three int32 components.
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
 * Holds a 3D position or size in whole units, such as voxels or grid cells.
 *
 * No vector class includes another; convert between them by components. The class is
 * not exported, so the constants are inline constexpr.
 */
class Vector3I
{
 public:
  /**
   * Leaves the values uninitialized. Use Vector3I::ZERO or Vector3I{} for zero.
   */
  Vector3I() = default;

  FORCEINLINE constexpr
  Vector3I(int32 inX, int32 inY, int32 inZ) noexcept;

  NODISCARD constexpr bool
  operator==(const Vector3I& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr Vector3I
  operator+(const Vector3I& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3I
  operator-(const Vector3I& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3I
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3I
  operator*(int32 scalar) const noexcept;

  FORCEINLINE constexpr Vector3I&
  operator+=(const Vector3I& other) noexcept;

  FORCEINLINE constexpr Vector3I&
  operator-=(const Vector3I& other) noexcept;

  FORCEINLINE constexpr Vector3I&
  operator*=(int32 scalar) noexcept;

 public:
  static const Vector3I ZERO;
  static const Vector3I UNIT;
  static const Vector3I UNIT_X;
  static const Vector3I UNIT_Y;
  static const Vector3I UNIT_Z;

  int32 x;
  int32 y;
  int32 z;
};

static_assert(std::is_trivially_copyable_v<Vector3I>);
static_assert(sizeof(Vector3I) == 12);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Vector3I::Vector3I(int32 inX, int32 inY, int32 inZ) noexcept
 : x(inX),
   y(inY),
   z(inZ)
{}

inline constexpr Vector3I Vector3I::ZERO{0, 0, 0};
inline constexpr Vector3I Vector3I::UNIT{1, 1, 1};
inline constexpr Vector3I Vector3I::UNIT_X{1, 0, 0};
inline constexpr Vector3I Vector3I::UNIT_Y{0, 1, 0};
inline constexpr Vector3I Vector3I::UNIT_Z{0, 0, 1};

/*
 */
FORCEINLINE constexpr Vector3I
Vector3I::operator+(const Vector3I& other) const noexcept
{
  return {x + other.x, y + other.y, z + other.z};
}

/*
 */
FORCEINLINE constexpr Vector3I
Vector3I::operator-(const Vector3I& other) const noexcept
{
  return {x - other.x, y - other.y, z - other.z};
}

/*
 */
FORCEINLINE constexpr Vector3I
Vector3I::operator-() const noexcept
{
  return {-x, -y, -z};
}

/*
 */
FORCEINLINE constexpr Vector3I
Vector3I::operator*(int32 scalar) const noexcept
{
  return {x * scalar, y * scalar, z * scalar};
}

/*
 */
FORCEINLINE constexpr Vector3I&
Vector3I::operator+=(const Vector3I& other) noexcept
{
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector3I&
Vector3I::operator-=(const Vector3I& other) noexcept
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

/*
 */
FORCEINLINE constexpr Vector3I&
Vector3I::operator*=(int32 scalar) noexcept
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Vector3I
operator*(int32 scalar, const Vector3I& vector) noexcept
{
  return vector * scalar;
}
} // namespace chEngineSDK
