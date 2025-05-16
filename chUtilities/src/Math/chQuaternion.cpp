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
  Math::sin_cos(&sinVal, &cosVal, halfRad);

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
Quaternion::toRotator() const {
  diagnosticCheckNaN();

  // Instead of testing x*y + z*w, let's set up specific tests for each rotation component

  // For roll detection (around X axis)
  float rollTest = w*x - y*z;
  // For pitch detection (around Y axis)
  float pitchTest = x*z + w*y;

  const float SINGULARITY_THRESHOLD = 0.4999995f;
  Rotator result;

  // Check for roll singularity
  if (abs(rollTest) > SINGULARITY_THRESHOLD) {
    // Roll singularity detected
    result.roll = 90.0f * (rollTest > 0 ? 1.0f : -1.0f);
    result.pitch = 0.0f;
    result.yaw = 0.0f;
    return result;
  }

  // Check for pitch singularity
  if (abs(pitchTest) > SINGULARITY_THRESHOLD) {
    // Pitch singularity detected
    result.pitch = 90.0f * (pitchTest > 0 ? 1.0f : -1.0f);
    result.roll = 0.0f;
    result.yaw = 0.0f;
    return result;
  }

  // Check for yaw singularity
  float yawTest = x*y + w*z;
  if (abs(yawTest) > SINGULARITY_THRESHOLD) {
    // Yaw singularity detected
    result.yaw = 90.0f * (yawTest > 0 ? 1.0f : -1.0f);
    result.roll = 0.0f;
    result.pitch = 0.0f;
    return result;
  }

  // No singularity detected, use standard conversion
  const float sqx = x * x;
  const float sqy = y * y;
  const float sqz = z * z;

  // Regular Euler angle calculation adjusted for your coordinate system
  result.roll = Math::atan2(2.0f * (w*x - y*z), 1.0f - 2.0f * (sqx + sqy)).valueDegree();
  result.pitch = Math::asin(2.0f * (x*z + w*y)).valueDegree();
  result.yaw = Math::atan2(2.0f * (w*z - x*y), 1.0f - 2.0f * (sqy + sqz)).valueDegree();

  return result;
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
Quaternion::Quaternion(const Rotator& rotator) {
  // Convert Euler angles to quaternion following standard algorithm

  // Convert angles to radians and halve them
  const Radian pitchHalves(rotator.pitch.valueRadian() * 0.5f);
  const Radian yawHalves(rotator.yaw.valueRadian() * 0.5f);
  const Radian rollHalves(rotator.roll.valueRadian() * 0.5f);

  // Precompute sin/cos values
  const float SP = Math::sin(pitchHalves);
  const float CP = Math::cos(pitchHalves);

  const float SY = Math::sin(yawHalves);
  const float CY = Math::cos(yawHalves);

  const float SR = Math::sin(rollHalves);
  const float CR = Math::cos(rollHalves);

  // Calculate quaternion components
  // Using the Euler angles to quaternion formula from:
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/
  w = CY * CP * CR + SY * SP * SR;
  x = CY * CP * SR - SY * SP * CR;
  y = SY * CP * SR + CY * SP * CR;
  z = SY * CP * CR - CY * SP * SR;

  // Verify that the result is valid
  diagnosticCheckNaN();
}

/*
 * Construct a quaternion from a rotation matrix
 */
Quaternion::Quaternion(const Matrix4& m) {
  const float trace = m[0][0] + m[1][1] + m[2][2];

  if (trace > 0.0f) {
    const float s = Math::sqrt(trace + 1.0f) * 2.0f;
    w = 0.25f * s;
    const float invS = 1.0f / s;
    x = (m[1][2] - m[2][1]) * invS;
    y = (m[2][0] - m[0][2]) * invS;
    z = (m[0][1] - m[1][0]) * invS;  // (m[1][0] - m[0][1])
  }
  else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
    const float s = Math::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
    x = 0.25f * s;
    const float invS = 1.0f / s;
    w = (m[2][1] - m[1][2]) * invS;
    y = (m[0][1] + m[1][0]) * invS;
    z = (m[0][2] + m[2][0]) * invS;
  }
  else if (m[1][1] > m[2][2]) {
    const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
    y = 0.25f * s;
    const float invS = 1.0f / s;
    w = (m[0][2] - m[2][0]) * invS;
    x = (m[0][1] + m[1][0]) * invS;
    z = (m[1][2] + m[2][1]) * invS;
  }
  else {
    const float s = Math::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    const float invS = 1.0f / s;
    w = (m[1][0] - m[0][1]) * invS;
    x = (m[0][2] + m[2][0]) * invS;
    y = (m[1][2] + m[2][1]) * invS;
    z = 0.25f * s;
  }
}
} // namespace chEngineSDK
