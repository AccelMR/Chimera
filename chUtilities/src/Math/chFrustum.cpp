/************************************************************************/
/**
 * @file chFrustum.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Frustum planes from a view-projection matrix.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chFrustum.h"

#include "chMath.h"
#include "chMatrix4.h"

namespace chEngineSDK {
namespace {
// Builds a plane from a * x + b * y + c * z + d >= 0 with a unit normal. Plane stores
// n . p = w, so d goes in negated.
Plane
makeUnitPlane(float a, float b, float c, float d) noexcept
{
  const float invLength = Math::invSqrt(a * a + b * b + c * c);
  return Plane(a * invLength, b * invLength, c * invLength, -d * invLength);
}

// With row vectors a point is inside when clip.w + sign * clip[column] >= 0, which reads
// the columns of the view-projection matrix.
Plane
clipPlane(const Matrix4& viewProjection, int32 column, float sign) noexcept
{
  return makeUnitPlane(viewProjection[0][3] + sign * viewProjection[0][column],
                       viewProjection[1][3] + sign * viewProjection[1][column],
                       viewProjection[2][3] + sign * viewProjection[2][column],
                       viewProjection[3][3] + sign * viewProjection[3][column]);
}
} // namespace

/*
 */
Frustum::Frustum(const Matrix4& viewProjection) noexcept
{
  planes[static_cast<SIZE_T>(FrustumSide::Left)] = clipPlane(viewProjection, 0, 1.0f);
  planes[static_cast<SIZE_T>(FrustumSide::Right)] = clipPlane(viewProjection, 0, -1.0f);
  planes[static_cast<SIZE_T>(FrustumSide::Bottom)] = clipPlane(viewProjection, 1, 1.0f);
  planes[static_cast<SIZE_T>(FrustumSide::Top)] = clipPlane(viewProjection, 1, -1.0f);
  planes[static_cast<SIZE_T>(FrustumSide::Far)] = clipPlane(viewProjection, 2, -1.0f);

  // Depth starts at 0, not -w, so the near plane is clip.z >= 0 alone.
  planes[static_cast<SIZE_T>(FrustumSide::Near)] =
      makeUnitPlane(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2],
                    viewProjection[3][2]);
}
} // namespace chEngineSDK
