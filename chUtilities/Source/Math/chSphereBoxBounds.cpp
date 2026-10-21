/************************************************************************/
/**
 * @file chSphereBoxBounds.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief SphereBoxBounds functions that need Matrix4.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chSphereBoxBounds.h"

#include "chMatrix4.h"

namespace chEngineSDK {

/*
 */
SphereBoxBounds
SphereBoxBounds::getTransformed(const Matrix4& matrix) const noexcept
{
  const AABox box = getBox().getTransformed(matrix);

  float maxAxisSquareScale = 0.0f;
  for (int32 row = 0; row < 3; ++row) {
    const Vector3 axis(matrix[row][0], matrix[row][1], matrix[row][2]);
    maxAxisSquareScale = Math::max(maxAxisSquareScale, axis.sqrMagnitude());
  }

  return {box.getCenter(), box.getExtent(), sphereRadius * Math::sqrt(maxAxisSquareScale)};
}
} // namespace chEngineSDK
