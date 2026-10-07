/************************************************************************/
/**
 * @file chQuaternion.cpp
 * @author AccelMR
 * @date 2022/03/29
 * @brief Quaternion conversions from and to other rotation types.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chQuaternion.h"

#include "chAngle.h"
#include "chMatrix4.h"
#include "chRotator.h"

namespace chEngineSDK {

/*
 */
Quaternion::Quaternion(const Vector3& axis, const Degree& angle) noexcept
{
  const float squareLength = axis.sqrMagnitude();
  if (squareLength <= Math::SMALL_NUMBER) {
    *this = IDENTITY;
    return;
  }

  float sinHalf;
  float cosHalf;
  Math::sinCos(0.5f * angle.valueRadian(), sinHalf, cosHalf);
  const float scale = sinHalf * Math::invSqrt(squareLength);
  x = axis.x * scale;
  y = axis.y * scale;
  z = axis.z * scale;
  w = cosHalf;
}

/*
 */
Quaternion::Quaternion(const Rotator& rotator) noexcept
{
  float sp, cp, sy, cy, sr, cr;
  Math::sinCos(rotator.pitch.valueRadian() * 0.5f, sp, cp);
  Math::sinCos(rotator.yaw.valueRadian() * 0.5f, sy, cy);
  Math::sinCos(rotator.roll.valueRadian() * 0.5f, sr, cr);

  // Same rotation as RotationMatrix(rotator), so both can be mixed freely.
  x = cr * sp * sy - sr * cp * cy;
  y = -cr * sp * cy - sr * cp * sy;
  z = cr * cp * sy - sr * sp * cy;
  w = cr * cp * cy + sr * sp * sy;
}

/*
 */
Quaternion::Quaternion(const Matrix4& m) noexcept
{
  const float trace = m[0][0] + m[1][1] + m[2][2];

  // Each branch divides by the largest of w, x, y, z, so the square root never gets a
  // value close to zero.
  if (trace > 0.0f) {
    const float s = Math::sqrt(trace + 1.0f) * 2.0f;
    const float invS = 1.0f / s;
    w = 0.25f * s;
    x = (m[1][2] - m[2][1]) * invS;
    y = (m[2][0] - m[0][2]) * invS;
    z = (m[0][1] - m[1][0]) * invS;
  }
  else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
    const float s = Math::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
    const float invS = 1.0f / s;
    w = (m[1][2] - m[2][1]) * invS;
    x = 0.25f * s;
    y = (m[0][1] + m[1][0]) * invS;
    z = (m[0][2] + m[2][0]) * invS;
  }
  else if (m[1][1] > m[2][2]) {
    const float s = Math::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
    const float invS = 1.0f / s;
    w = (m[2][0] - m[0][2]) * invS;
    x = (m[0][1] + m[1][0]) * invS;
    y = 0.25f * s;
    z = (m[1][2] + m[2][1]) * invS;
  }
  else {
    const float s = Math::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    const float invS = 1.0f / s;
    w = (m[0][1] - m[1][0]) * invS;
    x = (m[0][2] + m[2][0]) * invS;
    y = (m[1][2] + m[2][1]) * invS;
    z = 0.25f * s;
  }
}

/*
 */
Rotator
Quaternion::toRotator() const noexcept
{
  // Half the sine of the pitch. Close to +-0.5 the nose points straight up or down, where
  // yaw and roll turn around the same axis and only their difference can be recovered.
  const float pitchTest = z * x - w * y;
  const float yawY = 2.0f * (w * z + x * y);
  const float yawX = 1.0f - 2.0f * (y * y + z * z);
  const float yaw = Math::atan2(yawY, yawX).valueDegree();

  constexpr float kSingularityThreshold = 0.4999995f;
  if (pitchTest < -kSingularityThreshold) {
    const float roll = -yaw - 2.0f * Math::atan2(x, w).valueDegree();
    return Rotator(-90.0f, yaw, Math::unwindDegrees(roll));
  }
  if (pitchTest > kSingularityThreshold) {
    const float roll = yaw - 2.0f * Math::atan2(x, w).valueDegree();
    return Rotator(90.0f, yaw, Math::unwindDegrees(roll));
  }

  const float pitch = Math::asin(2.0f * pitchTest).valueDegree();
  const float roll =
      Math::atan2(-2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)).valueDegree();
  return Rotator(pitch, yaw, roll);
}
} // namespace chEngineSDK
