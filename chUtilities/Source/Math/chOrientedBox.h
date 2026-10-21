/************************************************************************/
/**
 * @file chOrientedBox.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Box with a rotation in 3D space.
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
#include "chQuaternion.h"
#include "chVector3.h"

namespace chEngineSDK {
/**
 * Holds a box that turns with its object, so it stays tight around rotated objects where
 * an AABox grows. Used for triggers, volumes and tighter bounds.
 */
class OrientedBox
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  OrientedBox() = default;

  /**
   * extent is half the size along each local axis; rotation must be unit length.
   */
  FORCEINLINE constexpr
  OrientedBox(const Vector3& inCenter, const Vector3& inExtent,
              const Quaternion& inRotation) noexcept;

  /**
   * The local X, Y or Z axis in world space (index 0, 1 or 2).
   */
  NODISCARD FORCEINLINE constexpr Vector3
  getAxis(int32 index) const noexcept;

  /**
   * A world point in the box's space, where the box spans -extent to extent.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  toLocalPoint(const Vector3& worldPoint) const noexcept;

  NODISCARD FORCEINLINE constexpr Vector3
  toLocalDirection(const Vector3& worldDirection) const noexcept;

  /**
   * The point of the box closest to worldPoint; worldPoint itself when it is inside.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  getClosestPoint(const Vector3& worldPoint) const noexcept;

 public:
  Vector3 center;
  Vector3 extent;
  Quaternion rotation;
};

static_assert(std::is_trivially_copyable_v<OrientedBox>);
static_assert(sizeof(OrientedBox) == 40);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
OrientedBox::OrientedBox(const Vector3& inCenter, const Vector3& inExtent,
                         const Quaternion& inRotation) noexcept
 : center(inCenter),
   extent(inExtent),
   rotation(inRotation)
{}

/*
 */
FORCEINLINE constexpr Vector3
OrientedBox::getAxis(int32 index) const noexcept
{
  return rotation.rotateVector(index == 0   ? Vector3::FORWARD
                               : index == 1 ? Vector3::RIGHT
                                            : Vector3::UP);
}

/*
 */
FORCEINLINE constexpr Vector3
OrientedBox::toLocalPoint(const Vector3& worldPoint) const noexcept
{
  return rotation.unrotateVector(worldPoint - center);
}

/*
 */
FORCEINLINE constexpr Vector3
OrientedBox::toLocalDirection(const Vector3& worldDirection) const noexcept
{
  return rotation.unrotateVector(worldDirection);
}

/*
 */
FORCEINLINE constexpr Vector3
OrientedBox::getClosestPoint(const Vector3& worldPoint) const noexcept
{
  const Vector3 local = toLocalPoint(worldPoint);
  const Vector3 clamped(Math::clamp(local.x, -extent.x, extent.x),
                        Math::clamp(local.y, -extent.y, extent.y),
                        Math::clamp(local.z, -extent.z, extent.z));
  return center + rotation.rotateVector(clamped);
}
} // namespace chEngineSDK
