/************************************************************************/
/**
 * @file chQuaternion.cpp
 * @author AccelMR
 * @date 2022/03/29
 *   Quaternion file hold implementation of Quaternion externals.
 */
/************************************************************************/

#include "chQuaternion.h"

#include "chMatrix4.h"
#include "chRadian.h"
#include "chRotator.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

// Static identity quaternion definition
const Quaternion Quaternion::IDENTITY = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

/*
 * Construct a quaternion from an axis and angle
 */
Quaternion::Quaternion(const Vector3& axis, const Degree& angle) {
  const float halfRad = 0.5f * angle.valueRadian();
  float sinVal, cosVal;
  Math::sinCos(halfRad, sinVal, cosVal);

  // Use normalized axis to ensure proper quaternion creation
  Vector3 normAxis = axis;
  if (axis.sqrMagnitude() > Math::SMALL_NUMBER) {
    normAxis = axis.getNormalized();
  }

  x = sinVal * normAxis.x;
  y = sinVal * normAxis.y;
  z = sinVal * normAxis.z;
  w = cosVal;

  diagnosticCheckNaN();
}

/*
 * Construct quaternion from Vector4
 */
Quaternion::Quaternion(const Vector4& v4) : x(v4.x), y(v4.y), z(v4.z), w(v4.w) {
  diagnosticCheckNaN();
}

/*
 * Convert quaternion to Rotator (Euler angles)
 */
Rotator
Quaternion::toRotator() const
{
  diagnosticCheckNaN();

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

/*
 * Rotate a vector by this quaternion
 */
Vector3
Quaternion::rotateVector(const Vector3& v) const {
  const Vector3 q(x, y, z);
  const Vector3 qCrossV(q.y * v.z - q.z * v.y,
                        q.z * v.x - q.x * v.z,
                        q.x * v.y - q.y * v.x);

  const Vector3 qCrossQCrossV(q.y * qCrossV.z - q.z * qCrossV.y,
                              q.z * qCrossV.x - q.x * qCrossV.z,
                              q.x * qCrossV.y - q.y * qCrossV.x);

  return Vector3(v.x + 2.0f * (w * qCrossV.x + qCrossQCrossV.x),
                 v.y + 2.0f * (w * qCrossV.y + qCrossQCrossV.y),
                 v.z + 2.0f * (w * qCrossV.z + qCrossQCrossV.z)
  );
}

/*
 * Rotate a vector by the inverse of this quaternion
 */
Vector3
Quaternion::unrotateVector(const Vector3& v) const {
  // Apply rotation with conjugate quaternion (inverse for unit quaternions)
  const Vector3 q(-x, -y, -z);

  // Same algorithm as rotateVector but with negated x,y,z
  const Vector3 qCrossV =
      Vector3(q.y * v.z - q.z * v.y, q.z * v.x - q.x * v.z, q.x * v.y - q.y * v.x);

  const Vector3 qCrossQCrossV =
      Vector3(q.y * qCrossV.z - q.z * qCrossV.y, q.z * qCrossV.x - q.x * qCrossV.z,
              q.x * qCrossV.y - q.y * qCrossV.x);

  return Vector3(v.x + 2.0f * (w * qCrossV.x + qCrossQCrossV.x),
                 v.y + 2.0f * (w * qCrossV.y + qCrossQCrossV.y),
                 v.z + 2.0f * (w * qCrossV.z + qCrossQCrossV.z));
}

/*
 * Construct a quaternion from a rotator (Euler angles)
 */
Quaternion::Quaternion(const Rotator& rotator)
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

  diagnosticCheckNaN();
}

/*
 * Construct a quaternion from a rotation matrix
 */
Quaternion::Quaternion(const Matrix4& m)
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
} // namespace chEngineSDK
