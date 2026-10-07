/************************************************************************/
/**
 * @file chMatrix4.cpp
 * @author AccelMR
 * @date 2022/02/20
 * @brief 4x4 matrix for transforms and projections.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chMatrix4.h"
#include "chMatrixHelpers.h"

#include "chMath.h"
#include "chPlane.h"
#include "chQuaternion.h"
#include "chAngle.h"
#include "chRotator.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

namespace {

Matrix4
buildPerspective(const Radian& halfFOV, float width, float height, float near,
                 float far) noexcept
{
  const float xScale = 1.0f / Math::tan(halfFOV);
  const float yScale = xScale * width / height;
  const float depthScale = far / (far - near);

  return Matrix4(xScale, 0.0f, 0.0f, 0.0f,
                 0.0f, yScale, 0.0f, 0.0f,
                 0.0f, 0.0f, depthScale, 1.0f,
                 0.0f, 0.0f, -near * depthScale, 0.0f);
}

Matrix4
buildLookAt(const Vector3& eyePosition, const Vector3& lookAtPosition,
            const Vector3& upVector) noexcept
{
  const Vector3 zAxis = (lookAtPosition - eyePosition).getNormalized();

  const float upDot = Math::abs(upVector.dot(zAxis));
  Vector3 effectiveUp = upVector;

  if (upDot > (1.0f - Math::SMALL_NUMBER)) {
    const float upForwardDot = Math::abs(upVector.dot(Vector3::FORWARD));
    const float upRightDot = Math::abs(upVector.dot(Vector3::RIGHT));
    if (upForwardDot < upRightDot) {
      effectiveUp = Vector3::FORWARD;
    }
    else {
      effectiveUp = Vector3::RIGHT;
    }
  }

  const Vector3 xAxis = effectiveUp.cross(zAxis).getNormalized();
  const Vector3 yAxis = zAxis.cross(xAxis);

  return Matrix4(xAxis.x, yAxis.x, zAxis.x, 0.0f,
                 xAxis.y, yAxis.y, zAxis.y, 0.0f,
                 xAxis.z, yAxis.z, zAxis.z, 0.0f,
                 -eyePosition.dot(xAxis), -eyePosition.dot(yAxis), -eyePosition.dot(zAxis),
                 1.0f);
}

} // namespace

/*
 */
Matrix4::Matrix4(const Plane& row0, const Plane& row1, const Plane& row2,
                 const Plane& row3) noexcept
 : m_data{{row0.x, row0.y, row0.z, row0.w},
          {row1.x, row1.y, row1.z, row1.w},
          {row2.x, row2.y, row2.z, row2.w},
          {row3.x, row3.y, row3.z, row3.w}}
{}

/*
 */
float
Matrix4::getDeterminant() const noexcept
{
  const auto& m = m_data;

  const float top01 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
  const float top02 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
  const float top03 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
  const float top12 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
  const float top13 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
  const float top23 = m[0][2] * m[1][3] - m[1][2] * m[0][3];

  const float bottom01 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
  const float bottom02 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
  const float bottom03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
  const float bottom12 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
  const float bottom13 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
  const float bottom23 = m[2][2] * m[3][3] - m[3][2] * m[2][3];

  return top01 * bottom23 - top02 * bottom13 + top03 * bottom12 + top12 * bottom03 -
         top13 * bottom02 + top23 * bottom01;
}

/*
 */
Matrix4
Matrix4::getInverse() const noexcept
{
  const auto& m = m_data;

  // The 2x2 determinants of the top and bottom row pairs are shared by every cofactor,
  // so they are computed once.
  const float top01 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
  const float top02 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
  const float top03 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
  const float top12 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
  const float top13 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
  const float top23 = m[0][2] * m[1][3] - m[1][2] * m[0][3];

  const float bottom01 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
  const float bottom02 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
  const float bottom03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
  const float bottom12 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
  const float bottom13 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
  const float bottom23 = m[2][2] * m[3][3] - m[3][2] * m[2][3];

  const float determinant = top01 * bottom23 - top02 * bottom13 + top03 * bottom12 +
                            top12 * bottom03 - top13 * bottom02 + top23 * bottom01;

  // A zero or denormal determinant gives an infinite scale, which would fill the result
  // with infinities and NaN.
  const float invDet = 1.0f / determinant;
  if (!Math::isFinite(invDet)) {
    return IDENTITY;
  }

  return Matrix4(
      ( m[1][1] * bottom23 - m[1][2] * bottom13 + m[1][3] * bottom12) * invDet,
      (-m[0][1] * bottom23 + m[0][2] * bottom13 - m[0][3] * bottom12) * invDet,
      ( m[3][1] * top23 - m[3][2] * top13 + m[3][3] * top12) * invDet,
      (-m[2][1] * top23 + m[2][2] * top13 - m[2][3] * top12) * invDet,

      (-m[1][0] * bottom23 + m[1][2] * bottom03 - m[1][3] * bottom02) * invDet,
      ( m[0][0] * bottom23 - m[0][2] * bottom03 + m[0][3] * bottom02) * invDet,
      (-m[3][0] * top23 + m[3][2] * top03 - m[3][3] * top02) * invDet,
      ( m[2][0] * top23 - m[2][2] * top03 + m[2][3] * top02) * invDet,

      ( m[1][0] * bottom13 - m[1][1] * bottom03 + m[1][3] * bottom01) * invDet,
      (-m[0][0] * bottom13 + m[0][1] * bottom03 - m[0][3] * bottom01) * invDet,
      ( m[3][0] * top13 - m[3][1] * top03 + m[3][3] * top01) * invDet,
      (-m[2][0] * top13 + m[2][1] * top03 - m[2][3] * top01) * invDet,

      (-m[1][0] * bottom12 + m[1][1] * bottom02 - m[1][2] * bottom01) * invDet,
      ( m[0][0] * bottom12 - m[0][1] * bottom02 + m[0][2] * bottom01) * invDet,
      (-m[3][0] * top12 + m[3][1] * top02 - m[3][2] * top01) * invDet,
      ( m[2][0] * top12 - m[2][1] * top02 + m[2][2] * top01) * invDet);
}

/*
 */
Matrix4
Matrix4::getInverseAffine() const noexcept
{
  const auto& m = m_data;
  CH_ASSERT(m[0][3] == 0.0f && m[1][3] == 0.0f && m[2][3] == 0.0f && m[3][3] == 1.0f);

  const float cofactor00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
  const float cofactor01 = m[0][2] * m[2][1] - m[0][1] * m[2][2];
  const float cofactor02 = m[0][1] * m[1][2] - m[0][2] * m[1][1];

  const float determinant = m[0][0] * cofactor00 + m[1][0] * cofactor01 +
                            m[2][0] * cofactor02;
  const float invDet = 1.0f / determinant;
  if (!Math::isFinite(invDet)) {
    return IDENTITY;
  }

  const float r00 = cofactor00 * invDet;
  const float r01 = cofactor01 * invDet;
  const float r02 = cofactor02 * invDet;
  const float r10 = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
  const float r11 = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
  const float r12 = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * invDet;
  const float r20 = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
  const float r21 = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * invDet;
  const float r22 = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;

  const float tx = m[3][0];
  const float ty = m[3][1];
  const float tz = m[3][2];

  return Matrix4(r00, r01, r02, 0.0f,
                 r10, r11, r12, 0.0f,
                 r20, r21, r22, 0.0f,
                 -(tx * r00 + ty * r10 + tz * r20),
                 -(tx * r01 + ty * r11 + tz * r21),
                 -(tx * r02 + ty * r12 + tz * r22),
                 1.0f);
}

/*
 */
Rotator
Matrix4::rotator() const noexcept
{
  const Vector3 forward(m_data[0][0], m_data[0][1], m_data[0][2]);
  const Vector3 right(m_data[1][0], m_data[1][1], m_data[1][2]);
  const Vector3 up(m_data[2][0], m_data[2][1], m_data[2][2]);

  Rotator result(
      Math::atan2(forward.z, Math::sqrt(forward.x * forward.x + forward.y * forward.y))
          .valueDegree(),
      Math::atan2(forward.y, forward.x).valueDegree(),
      0.0f);

  // Pitch and yaw already place the forward axis; roll is how far this matrix's right
  // axis is turned from the right axis of that rotation without roll.
  const RotationMatrix noRoll(result);
  const Vector3 noRollRight(noRoll[1][0], noRoll[1][1], noRoll[1][2]);
  result.roll = Degree(Math::atan2(up.dot(noRollRight), right.dot(noRollRight)));

  return result;
}

/*
 */
Quaternion
Matrix4::toQuaternion() const noexcept
{
  return Quaternion(*this);
}

/*
 */
RotationTranslationMatrix::RotationTranslationMatrix(const Rotator& rotator,
                                                     const Vector3& origin) noexcept
{
  float sp, cp, sy, cy, sr, cr;
  Math::sinCos(rotator.pitch.valueRadian(), sp, cp);
  Math::sinCos(rotator.yaw.valueRadian(), sy, cy);
  Math::sinCos(rotator.roll.valueRadian(), sr, cr);

  m_data[0][0] = cp * cy;
  m_data[0][1] = cp * sy;
  m_data[0][2] = sp;
  m_data[0][3] = 0.0f;

  m_data[1][0] = sr * sp * cy - cr * sy;
  m_data[1][1] = sr * sp * sy + cr * cy;
  m_data[1][2] = -sr * cp;
  m_data[1][3] = 0.0f;

  m_data[2][0] = -(cr * sp * cy + sr * sy);
  m_data[2][1] = cy * sr - cr * sp * sy;
  m_data[2][2] = cr * cp;
  m_data[2][3] = 0.0f;

  m_data[3][0] = origin.x;
  m_data[3][1] = origin.y;
  m_data[3][2] = origin.z;
  m_data[3][3] = 1.0f;
}

/*
 */
ScaleRotationTranslationMatrix::ScaleRotationTranslationMatrix(const Vector3& scale,
                                                               const Rotator& rotator,
                                                               const Vector3& origin) noexcept
 : Matrix4(RotationTranslationMatrix(rotator, origin))
{
  // With row vectors, scaling first is the same as scaling each rotation row.
  for (int32 column = 0; column < 3; ++column) {
    m_data[0][column] *= scale.x;
    m_data[1][column] *= scale.y;
    m_data[2][column] *= scale.z;
  }
}

/*
 */
PerspectiveMatrix::PerspectiveMatrix(const Radian& halfFOV, float width, float height,
                                     float near, float far) noexcept
 : Matrix4(buildPerspective(halfFOV, width, height, near, far))
{}

/*
 */
LookAtMatrix::LookAtMatrix(const Vector3& eyePosition, const Vector3& lookAtPosition,
                           const Vector3& upVector) noexcept
 : Matrix4(buildLookAt(eyePosition, lookAtPosition, upVector))
{}

} // namespace chEngineSDK
