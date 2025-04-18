/********************************************************************** /
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

 #include "chQuaternion.h"
 #include "chRadian.h"
 #include "chPlane.h"
 #include "chMath.h"

namespace chEngineSDK{

const Matrix4 Matrix4::ZERO = Matrix4(0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f);

const Matrix4 Matrix4::IDENTITY = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 1.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

const Matrix4 Matrix4::UNITY = Matrix4(1.0f, 1.0f, 1.0f, 1.0f,
                                       1.0f, 1.0f, 1.0f, 1.0f,
                                       1.0f, 1.0f, 1.0f, 1.0f,
                                       1.0f, 1.0f, 1.0f, 1.0f);

/*
*/
Rotator
Matrix4::rotator() const
{
  const Vector3 XAxis(m4x4[0][0], m4x4[0][1], m4x4[0][2]);
  const Vector3 YAxis(m4x4[1][0], m4x4[1][1], m4x4[1][2]);
  const Vector3 ZAxis(m4x4[2][0], m4x4[2][1], m4x4[2][2]);

  Rotator tmpRotator = Rotator(Math::atan2(XAxis.z, Math::sqrt(
                                                    Math::sqrt(XAxis.x) +
                                                    Math::sqrt(XAxis.y))).valueDegree(),
                              Math::atan2(XAxis.y, XAxis.x).valueDegree(),
                              0);

  RotationMatrix RM(tmpRotator);
  const Vector3 SYAxis(RM.m4x4[1][0], RM.m4x4[1][1], RM.m4x4[1][2]);
  tmpRotator.roll = Math::atan2(ZAxis.dot(SYAxis), YAxis.dot(SYAxis)).valueDegree();

  tmpRotator.diagnosticNaN();
  return tmpRotator;
}

/*
*/
Quaternion
Matrix4::toQuaternion() const
{
  Quaternion Result(*this);
  return Result;
}

/*
*/
Vector4
Matrix4::transformPosition(const Vector3& v) const
{
  return transformVector4(Vector4(v.x, v.y, v.z, 1.0f));
}

/*
*/
Vector4
Matrix4::transformVector(const Vector3& v) const
{
  return transformVector4(Vector4(v.x, v.y, v.z, 0.0f));
}


FORCEINLINE Vector4
 Matrix4::transformVector4(const Vector4& p) const {
  Vector4 Result{};
  Result.x = p.x * m00 + p.y * m01 + p.z * m02 + p.w * m03;
  Result.y = p.x * m10 + p.y * m11 + p.z * m12 + p.w * m13;
  Result.z = p.x * m20 + p.y * m21 + p.z * m22 + p.w * m23;
  Result.w = p.x * m30 + p.y * m31 + p.z * m32 + p.w * m33;
  return Result;

}

/*
*/
PerspectiveMatrix::PerspectiveMatrix(const Radian& halfFOV, float width,
                                     float height, float _near, float _far)
  : Matrix4(
      Plane(1.0f / Math::tan(halfFOV), 0.0f, 0.0f, 0.0f),
      Plane(0.0f, (width / Math::tan(halfFOV)) / height, 0.0f, 0.0f),
      Plane(0.0f, 0.0f, _far / (_far - _near), 1.0f),
      Plane(0.0f, 0.0f, -_near * _far / (_far - _near), 0.0f))
{}
}