/************************************************************************/
/**
 * @file chMatrix4.cpp
 * @author AccelMR
 * @date 2022/02/20
 *   Matrix 4 math file. Here goes all matrix 4 related content.
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
#include "chRadian.h"
#include "chRotator.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

// Initialize static constants
const Matrix4 Matrix4::ZERO = Matrix4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

const Matrix4 Matrix4::IDENTITY = Matrix4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

const Matrix4 Matrix4::UNITY = Matrix4(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                       1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

/*
*/
Matrix4::Matrix4(const Vector4& firstRow,
                const Vector4& secondRow,
                const Vector4& thirdRow,
                const Vector4& fourthRow) {
  m_data[0][0] = firstRow.x;  m_data[0][1] = firstRow.y;  m_data[0][2] = firstRow.z;  m_data[0][3] = firstRow.w;
  m_data[1][0] = secondRow.x; m_data[1][1] = secondRow.y; m_data[1][2] = secondRow.z; m_data[1][3] = secondRow.w;
  m_data[2][0] = thirdRow.x;  m_data[2][1] = thirdRow.y;  m_data[2][2] = thirdRow.z;  m_data[2][3] = thirdRow.w;
  m_data[3][0] = fourthRow.x; m_data[3][1] = fourthRow.y; m_data[3][2] = fourthRow.z; m_data[3][3] = fourthRow.w;
}


/*
*/
FORCEINLINE
Matrix4::Matrix4(const Plane& InX,
                const Plane& InY,
                const Plane& InZ,
                const Plane& InW) {
  m_data[0][0] = InX.x; m_data[0][1] = InX.y; m_data[0][2] = InX.z; m_data[0][3] = InX.w;
  m_data[1][0] = InY.x; m_data[1][1] = InY.y; m_data[1][2] = InY.z; m_data[1][3] = InY.w;
  m_data[2][0] = InZ.x; m_data[2][1] = InZ.y; m_data[2][2] = InZ.z; m_data[2][3] = InZ.w;
  m_data[3][0] = InW.x; m_data[3][1] = InW.y; m_data[3][2] = InW.z; m_data[3][3] = InW.w;
}


/*
 * Extracts a rotation matrix as a Rotator (Euler angles)
 */
Rotator
Matrix4::rotator() const {
  // Extract the rotation axes
  const Vector3 XAxis(m_data[0][0], m_data[0][1], m_data[0][2]);
  const Vector3 YAxis(m_data[1][0], m_data[1][1], m_data[1][2]);
  const Vector3 ZAxis(m_data[2][0], m_data[2][1], m_data[2][2]);

  // Calculate pitch from the x-axis (elevation angle)
  Rotator tmpRotator =
      Rotator(Math::atan2(XAxis.z, Math::sqrt(Math::sqrt(XAxis.x) + Math::sqrt(XAxis.y)))
                  .valueDegree(),
              Math::atan2(XAxis.y, XAxis.x).valueDegree(), 0);

  // Calculate roll using the temporary rotator to get a reference frame
  RotationMatrix RM(tmpRotator);
  const Vector3 SYAxis(RM.m_data[1][0], RM.m_data[1][1], RM.m_data[1][2]);
  tmpRotator.roll = Math::atan2(ZAxis.dot(SYAxis), YAxis.dot(SYAxis)).valueDegree();

  // Check for NaN values
  tmpRotator.diagnosticNaN();
  return tmpRotator;
}

/*
 * Converts this matrix to a quaternion
 */
Quaternion
Matrix4::toQuaternion() const {
  // Create quaternion from this matrix
  // This assumes the matrix contains a pure rotation part
  Quaternion Result(*this);
  return Result;
}

/*
 * Transforms a position vector by this matrix
 */
Vector4
Matrix4::transformPosition(const Vector3& v) const {
  // Transform position vectors with homogeneous coordinate w=1.0
  return transformVector4(Vector4(v.x, v.y, v.z, 1.0f));
}

/*
 * Transforms a direction vector by this matrix
 */
Vector4
Matrix4::transformVector(const Vector3& v) const {
  // Transform direction vectors with homogeneous coordinate w=0.0
  return transformVector4(Vector4(v.x, v.y, v.z, 0.0f));
}

/*
*/
Matrix4
Matrix4::getInverse()
{
  Matrix4 Result;
  const float Det = getDeterminant();

  if (Math::abs(Det) < Math::SMALL_NUMBER) {
    Result = Matrix4::IDENTITY;
    // TODO: Log a warning about matrix being non-invertible
  }
  else {
    const float InvDet = 1.0f / Det;

    // Calculate cofactors and adjugate matrix
    // This approach is clearer and less prone to errors than the previous implementation

    // First row of cofactors
    Result.m_data[0][0] = InvDet * (
      m_data[1][1] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
      m_data[1][2] * (m_data[2][1] * m_data[3][3] - m_data[2][3] * m_data[3][1]) +
      m_data[1][3] * (m_data[2][1] * m_data[3][2] - m_data[2][2] * m_data[3][1])
    );

    Result.m_data[0][1] = -InvDet * (
      m_data[0][1] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
      m_data[0][2] * (m_data[2][1] * m_data[3][3] - m_data[2][3] * m_data[3][1]) +
      m_data[0][3] * (m_data[2][1] * m_data[3][2] - m_data[2][2] * m_data[3][1])
    );

    Result.m_data[0][2] = InvDet * (
      m_data[0][1] * (m_data[1][2] * m_data[3][3] - m_data[1][3] * m_data[3][2]) -
      m_data[0][2] * (m_data[1][1] * m_data[3][3] - m_data[1][3] * m_data[3][1]) +
      m_data[0][3] * (m_data[1][1] * m_data[3][2] - m_data[1][2] * m_data[3][1])
    );

    Result.m_data[0][3] = -InvDet * (
      m_data[0][1] * (m_data[1][2] * m_data[2][3] - m_data[1][3] * m_data[2][2]) -
      m_data[0][2] * (m_data[1][1] * m_data[2][3] - m_data[1][3] * m_data[2][1]) +
      m_data[0][3] * (m_data[1][1] * m_data[2][2] - m_data[1][2] * m_data[2][1])
    );

    // Second row of cofactors
    Result.m_data[1][0] = -InvDet * (
      m_data[1][0] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
      m_data[1][2] * (m_data[2][0] * m_data[3][3] - m_data[2][3] * m_data[3][0]) +
      m_data[1][3] * (m_data[2][0] * m_data[3][2] - m_data[2][2] * m_data[3][0])
    );

    Result.m_data[1][1] = InvDet * (
      m_data[0][0] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
      m_data[0][2] * (m_data[2][0] * m_data[3][3] - m_data[2][3] * m_data[3][0]) +
      m_data[0][3] * (m_data[2][0] * m_data[3][2] - m_data[2][2] * m_data[3][0])
    );

    Result.m_data[1][2] = -InvDet * (
      m_data[0][0] * (m_data[1][2] * m_data[3][3] - m_data[1][3] * m_data[3][2]) -
      m_data[0][2] * (m_data[1][0] * m_data[3][3] - m_data[1][3] * m_data[3][0]) +
      m_data[0][3] * (m_data[1][0] * m_data[3][2] - m_data[1][2] * m_data[3][0])
    );

    Result.m_data[1][3] = InvDet * (
      m_data[0][0] * (m_data[1][2] * m_data[2][3] - m_data[1][3] * m_data[2][2]) -
      m_data[0][2] * (m_data[1][0] * m_data[2][3] - m_data[1][3] * m_data[2][0]) +
      m_data[0][3] * (m_data[1][0] * m_data[2][2] - m_data[1][2] * m_data[2][0])
    );

    // Third row of cofactors
    Result.m_data[2][0] = InvDet * (
      m_data[1][0] * (m_data[2][1] * m_data[3][3] - m_data[2][3] * m_data[3][1]) -
      m_data[1][1] * (m_data[2][0] * m_data[3][3] - m_data[2][3] * m_data[3][0]) +
      m_data[1][3] * (m_data[2][0] * m_data[3][1] - m_data[2][1] * m_data[3][0])
    );

    Result.m_data[2][1] = -InvDet * (
      m_data[0][0] * (m_data[2][1] * m_data[3][3] - m_data[2][3] * m_data[3][1]) -
      m_data[0][1] * (m_data[2][0] * m_data[3][3] - m_data[2][3] * m_data[3][0]) +
      m_data[0][3] * (m_data[2][0] * m_data[3][1] - m_data[2][1] * m_data[3][0])
    );

    Result.m_data[2][2] = InvDet * (
      m_data[0][0] * (m_data[1][1] * m_data[3][3] - m_data[1][3] * m_data[3][1]) -
      m_data[0][1] * (m_data[1][0] * m_data[3][3] - m_data[1][3] * m_data[3][0]) +
      m_data[0][3] * (m_data[1][0] * m_data[3][1] - m_data[1][1] * m_data[3][0])
    );

    Result.m_data[2][3] = -InvDet * (
      m_data[0][0] * (m_data[1][1] * m_data[2][3] - m_data[1][3] * m_data[2][1]) -
      m_data[0][1] * (m_data[1][0] * m_data[2][3] - m_data[1][3] * m_data[2][0]) +
      m_data[0][3] * (m_data[1][0] * m_data[2][1] - m_data[1][1] * m_data[2][0])
    );

    // Fourth row of cofactors
    Result.m_data[3][0] = -InvDet * (
      m_data[1][0] * (m_data[2][1] * m_data[3][2] - m_data[2][2] * m_data[3][1]) -
      m_data[1][1] * (m_data[2][0] * m_data[3][2] - m_data[2][2] * m_data[3][0]) +
      m_data[1][2] * (m_data[2][0] * m_data[3][1] - m_data[2][1] * m_data[3][0])
    );

    Result.m_data[3][1] = InvDet * (
      m_data[0][0] * (m_data[2][1] * m_data[3][2] - m_data[2][2] * m_data[3][1]) -
      m_data[0][1] * (m_data[2][0] * m_data[3][2] - m_data[2][2] * m_data[3][0]) +
      m_data[0][2] * (m_data[2][0] * m_data[3][1] - m_data[2][1] * m_data[3][0])
    );

    Result.m_data[3][2] = -InvDet * (
      m_data[0][0] * (m_data[1][1] * m_data[3][2] - m_data[1][2] * m_data[3][1]) -
      m_data[0][1] * (m_data[1][0] * m_data[3][2] - m_data[1][2] * m_data[3][0]) +
      m_data[0][2] * (m_data[1][0] * m_data[3][1] - m_data[1][1] * m_data[3][0])
    );

    Result.m_data[3][3] = InvDet * (
      m_data[0][0] * (m_data[1][1] * m_data[2][2] - m_data[1][2] * m_data[2][1]) -
      m_data[0][1] * (m_data[1][0] * m_data[2][2] - m_data[1][2] * m_data[2][0]) +
      m_data[0][2] * (m_data[1][0] * m_data[2][1] - m_data[1][1] * m_data[2][0])
    );
  }

  return Result;
}

/*
*/
Vector4
Matrix4::transformVector4(const Vector4& p) const {
  Vector4 Result;
  Result.x = m_data[0][0] * p.x + m_data[1][0] * p.y + m_data[2][0] * p.z + m_data[3][0] * p.w;
  Result.y = m_data[0][1] * p.x + m_data[1][1] * p.y + m_data[2][1] * p.z + m_data[3][1] * p.w;
  Result.z = m_data[0][2] * p.x + m_data[1][2] * p.y + m_data[2][2] * p.z + m_data[3][2] * p.w;
  Result.w = m_data[0][3] * p.x + m_data[1][3] * p.y + m_data[2][3] * p.z + m_data[3][3] * p.w;
  return Result;
}

} // namespace chEngineSDK



/*Helper matrixes*/

namespace chEngineSDK {

/*
 * Implementation of RotationTranslationMatrix constructor
 */
RotationTranslationMatrix::RotationTranslationMatrix(const Rotator& rotator,
                                                     const Vector3& origin)
 : Matrix4() {
  const float SP = Math::sin(rotator.pitch);
  const float CP = Math::cos(rotator.pitch);

  const float SY = Math::sin(rotator.yaw);
  const float CY = Math::cos(rotator.yaw);

  const float SR = Math::sin(rotator.roll);
  const float CR = Math::cos(rotator.roll);


  m_data[0][0] = CP * CY;    // cos(0) * cos(90) = 1 * 0 = 0 ✔
  m_data[0][1] = CP * SY;    // cos(0) * sin(90) = 1 * 1 = 1 ✔
  m_data[0][2] = SP;         // sin(0) = 0 ✔
  m_data[0][3] = 0.0f;

  m_data[1][0] = SR * SP * CY - CR * SY;  // = 0 - (1 * 1) = -1 ✔
  m_data[1][1] = SR * SP * SY + CR * CY;  // = 0 + (1 * 0) = 0 ✔
  m_data[1][2] = -SR * CP;                // = 0 ✔
  m_data[1][3] = 0.0f;

  m_data[2][0] = -(CR * SP * CY + SR * SY);  // = -(0 + 0) = 0 ✔
  m_data[2][1] = CY * SR - CR * SP * SY;     // = 0 - 0 = 0 ✔
  m_data[2][2] = CR * CP;
  m_data[2][3] = 0.0f;

  m_data[3][0] = origin.x;
  m_data[3][1] = origin.y;
  m_data[3][2] = origin.z;
  m_data[3][3] = 1.0f;
}

/*
 * Implementation of ScaleRotationTranslationMatrix constructor
 */
ScaleRotationTranslationMatrix::ScaleRotationTranslationMatrix(const Vector3& scale,
                                                               const Rotator& rotator,
                                                               const Vector3& origin)
 : Matrix4() {
  const float SP = Math::sin(rotator.pitch);
  const float CP = Math::cos(rotator.pitch);

  const float SY = Math::sin(rotator.yaw);
  const float CY = Math::cos(rotator.yaw);

  const float SR = Math::sin(rotator.roll);
  const float CR = Math::cos(rotator.roll);

  // Compute rotation matrix elements with scale (Rx * Ry * Rz * S)
  at(0, 0) = (CP * CY) * scale.x;
  at(0, 1) = (CP * SY) * scale.x;
  at(0, 2) = (SP)*scale.x;
  at(0, 3) = 0.0f;

  at(1, 0) = (SR * SP * CY - CR * SY) * scale.y;
  at(1, 1) = (SR * SP * SY + CR * CY) * scale.y;
  at(1, 2) = (-SR * CP) * scale.y;
  at(1, 3) = 0.0f;

  at(2, 0) = (-(CR * SP * CY + SR * SY)) * scale.z;
  at(2, 1) = (CY * SR - CR * SP * SY) * scale.z;
  at(2, 2) = (CR * CP) * scale.z;
  at(2, 3) = 0.0f;

  // Set translation components
  at(3, 0) = origin.x;
  at(3, 1) = origin.y;
  at(3, 2) = origin.z;
  at(3, 3) = 1.0f;
}

/*
 * Implementation of PerspectiveMatrix constructor
 */
PerspectiveMatrix::PerspectiveMatrix(const Radian& halfFOV, float width, float height,
                                     float near, float far)
 : Matrix4(Plane(1.0f / Math::tan(halfFOV), 0.0f, 0.0f, 0.0f),
           Plane(0.0f, (width / Math::tan(halfFOV)) / height, 0.0f, 0.0f),
           Plane(0.0f, 0.0f, far / (far - near), 1.0f),
           Plane(0.0f, 0.0f, -near * far / (far - near), 0.0f)) {}

/*
 * Implementation of LookAtMatrix constructor
 */
LookAtMatrix::LookAtMatrix(const Vector3& eyePosition, const Vector3& lookAtPosition,
                           const Vector3& upVector)
 : Matrix4() {
  const Vector3 ZAxis = (lookAtPosition - eyePosition).getNormalized();

  const float UpDot = Math::abs(upVector.dot(ZAxis));
  Vector3 effectiveUp = upVector;

  if (UpDot > (1.0f - Math::SMALL_NUMBER)) {
    const float UpForwardDot = Math::abs(upVector.dot(Vector3::FORWARD));
    const float UpRightDot = Math::abs(upVector.dot(Vector3::RIGHT));
    if (UpForwardDot < UpRightDot) {
      effectiveUp = Vector3::FORWARD;
    }
    else {
      effectiveUp = Vector3::RIGHT;
    }
  }

  const Vector3 XAxis = ZAxis.cross(effectiveUp).getNormalized();
  const Vector3 YAxis = ZAxis.cross(XAxis);

  m_data[0][0] = XAxis.x;
  m_data[0][1] = YAxis.x;
  m_data[0][2] = ZAxis.x;
  m_data[0][3] = 0.0f;
  m_data[1][0] = XAxis.y;
  m_data[1][1] = YAxis.y;
  m_data[1][2] = ZAxis.y;
  m_data[1][3] = 0.0f;
  m_data[2][0] = XAxis.z;
  m_data[2][1] = YAxis.z;
  m_data[2][2] = ZAxis.z;
  m_data[2][3] = 0.0f;
  m_data[3][0] = -eyePosition.dot(XAxis);
  m_data[3][1] = -eyePosition.dot(YAxis);
  m_data[3][2] = -eyePosition.dot(ZAxis);
  m_data[3][3] = 1.0f;
}

} // namespace chEngineSDK
