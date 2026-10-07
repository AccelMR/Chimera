/************************************************************************/
/**
 * @file chCapsule.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Capsule in 3D space.
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
 * Holds a capsule: every point within radius of the segment from start to end. Its
 * round ends slide along walls and corners, so it is the usual shape for characters.
 */
class Capsule
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Capsule() = default;

  FORCEINLINE constexpr
  Capsule(const Vector3& inStart, const Vector3& inEnd, float inRadius) noexcept;

  /**
   * The point of the segment from start to end closest to point.
   */
  NODISCARD FORCEINLINE constexpr Vector3
  getClosestAxisPoint(const Vector3& point) const noexcept;

 public:
  Vector3 start;
  Vector3 end;
  float radius;
};

static_assert(std::is_trivially_copyable_v<Capsule>);
static_assert(sizeof(Capsule) == 28);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Capsule::Capsule(const Vector3& inStart, const Vector3& inEnd, float inRadius) noexcept
 : start(inStart),
   end(inEnd),
   radius(inRadius)
{}

/*
 */
FORCEINLINE constexpr Vector3
Capsule::getClosestAxisPoint(const Vector3& point) const noexcept
{
  const Vector3 axis = end - start;
  const float squareLength = axis.sqrMagnitude();
  if (squareLength <= Math::SMALL_NUMBER) {
    return start;
  }
  const float alpha = Math::clamp((point - start).dot(axis) / squareLength, 0.0f, 1.0f);
  return start + axis * alpha;
}
} // namespace chEngineSDK
