/************************************************************************/
/**
 * @file chBox.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief AABox functions that need Matrix4.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chBox.h"

#include "chMatrix4.h"

namespace chEngineSDK {

/*
 */
AABox
AABox::getTransformed(const Matrix4& matrix) const noexcept
{
  // Transforms the center and grows the extent by the absolute value of each axis
  // (Arvo's method), which gives the same box as transforming the 8 corners for less.
  const Vector3 center = getCenter();
  const Vector3 extent = getExtent();

  Vector3 newCenter(matrix[3][0], matrix[3][1], matrix[3][2]);
  Vector3 newExtent = Vector3::ZERO;
  const float centerValues[3] = {center.x, center.y, center.z};
  const float extentValues[3] = {extent.x, extent.y, extent.z};
  for (int32 row = 0; row < 3; ++row) {
    const Vector3 axis(matrix[row][0], matrix[row][1], matrix[row][2]);
    newCenter += axis * centerValues[row];
    newExtent += axis.getAbs() * extentValues[row];
  }

  return {newCenter - newExtent, newCenter + newExtent};
}
} // namespace chEngineSDK
