/************************************************************************/
/**
 * @file chFrustum.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief The volume a camera sees, as six planes.
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

#include "chPlane.h"

namespace chEngineSDK {

enum class FrustumSide : uint8
{
  Left,
  Right,
  Bottom,
  Top,
  Near,
  Far,
  Count
};

/**
 * Holds the six planes around what a camera sees, so culling can test bounds against
 * the view without depending on a camera. Every normal is unit length and points
 * inside, so a point is inside when planeDot is not negative for any plane.
 */
class Frustum
{
 public:
  /**
   * Leaves the values uninitialized.
   */
  Frustum() = default;

  /**
   * Planes of a view * projection matrix that projects into Direct3D's clip space
   * (depth 0 to 1), as PerspectiveMatrix and OrthographicMatrix do.
   */
  CH_UTILITY_EXPORT explicit
  Frustum(const Matrix4& viewProjection) noexcept;

  NODISCARD FORCEINLINE constexpr const Plane&
  getPlane(FrustumSide side) const noexcept;

 public:
  Plane planes[static_cast<SIZE_T>(FrustumSide::Count)];
};

static_assert(std::is_trivially_copyable_v<Frustum>);
static_assert(sizeof(Frustum) == 96);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr const Plane&
Frustum::getPlane(FrustumSide side) const noexcept
{
  return planes[static_cast<SIZE_T>(side)];
}
} // namespace chEngineSDK
