/************************************************************************/
/**
 * @file chMatrix4.h
 * @author AccelMR
 * @date 2022/02/20
 *   Matrix 4 math file. Here goes all matrix 4 related content. 
 * 
 *
 * Coordinate system being X = front, Z = up, Y = right
 * 
 * Left hand
 */
 /************************************************************************/

#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chVector4.h"
#include "chVector3.h"
#include "chRotator.h"
#include "chPlane.h"

namespace chEngineSDK {
using std::ostream;
/*
 * Description: 
 *     Class that holds a 4x4 matrix, represented as Row-Major. 
 *     That means a matrix-vector multiplication will be Result = Vector * Matrix.
 *
 * Sample usage:
 *  Matrix4 m4(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
 *  m4.transpose();
 *
 */
class Matrix4
{
 public:
 /*
  * @brief Default constructor
  */
  Matrix4() = default;

  FORCEINLINE Matrix4(float _m00, float _m01, float _m02, float _m03,
                      float _m10, float _m11, float _m12, float _m13,
                      float _m20, float _m21, float _m22, float _m23,
                      float _m30, float _m31, float _m32, float _m33);

  /** 
   *   Copy constructor.
   **/
  FORCEINLINE Matrix4(const Matrix4& copy);

 /*
  * @brief Default destructor
  */
  ~Matrix4() = default;

  /************************************************************************/
  /*
   * Vector4 interaction if its included before this file.
   */                                                                     
  /************************************************************************/

/** 
 *   Constructor from four Vectors 4;
 **/
  FORCEINLINE Matrix4(const Vector4& firscRow,
                      const Vector4& secondRow, 
                      const Vector4& thirdRow, 
                      const Vector4& fourthRow);

  FORCEINLINE Matrix4(const Plane &InX,
                      const Plane &InY,
                      const Plane &InZ,
                      const Plane &InW);

  /************************************************************************/
  /*
   * Methods
   */                                                                     
  /************************************************************************/

  /** 
   *   Sets this matrix to identity.
   **/
  FORCEINLINE void
  setIdentity();

  /** 
   *   Creates a new transposed matrix with this as reference.
   * 
   * @return Matrix4
   *    The new matrix created from this.
   **/
  FORCEINLINE Matrix4
  getTransposed() const;

  /** 
   *   Transpose this Matrix.
   * 
   * @return Matrix4&
   *   This reference.
   **/
  FORCEINLINE Matrix4&
  transposed();

  /** 
   *   Computes the determinant of this matrix.
   *
   * @return float
   *  New float created.
   **/
  FORCEINLINE float
  getDeterminant() const;

  /** 
   *   Computes the inverse matrix from this.
   * 
   * @return Matrix4
   *    A new Matrix4 created from this.
   **/
  FORCEINLINE Matrix4
  getInverse();

  /**
   *  Computes the conversion between this matrix to a rotator.
   *
   * @return Rotator 
   *  Representation of this matrix
   */
  CH_UTILITY_EXPORT Rotator
  rotator() const;

  /**
   *  Transform a rotation matrix into a quaternion.
   * NOTE: rotation part will need to be unit length for this to be right!
   */
  CH_UTILITY_EXPORT Quaternion
  toQuaternion() const;

  /** 
   *   Transforms a position with this matrix.
   * 
   * @return
   *   The new position by the transform of this matrix.
   **/
  CH_UTILITY_EXPORT Vector4
  transformPosition(const Vector3& v) const;

  /** 
   *   Transform a direction vector by this matrix.
   * 
   * @param v
   *   The vector to be transformed
   * 
   * @return
   *    The new vector transformed in world space.
   **/
  CH_UTILITY_EXPORT Vector4
  transformVector(const Vector3& v) const;

  /** 
   *   TRanforms a vector 4 by this matrix.
   * 
   * @param p
   *    Vector to be transformed.
   * 
   * @return
   *  The new Vector created by the transformation.
   **/
  CH_UTILITY_EXPORT Vector4
  transformVector4(const Vector4& p) const;

  /************************************************************************/
  /*
   * Operator overload
   */                                                                     
  /************************************************************************/

  /** 
   *   Computes a multiplication between two matrices.
   * @param other
   *  The other matrix to be multiplied.
   * 
   * @return Matrix4
   *  A new matrix result. 
   **/
  FORCEINLINE Matrix4
  operator*(const Matrix4& other) const;

  /** 
   *   Computes a multiplication between two matrices, modifying this matrix.
   * @param other
   *  The other matrix to be multiplied.
   * 
   * @return Matrix4
   *  A new matrix result.
   **/
  FORCEINLINE void
  operator*=(const Matrix4& Other);

  /**
   *   Computes a sum between two matrices.
   * @param other
   *  The other matrix to be sum.
   *
   * @return Matrix4
   *  A new matrix result.
   **/
  FORCEINLINE Matrix4
  operator+(const Matrix4& other) const;

  /**
   *   Computes a subtracts between two matrices.
   * @param other
   *  The other matrix to be subtracted.
   *
   * @return Matrix4
   *  A new matrix result.
   **/
  FORCEINLINE Matrix4
  operator-(const Matrix4& other) const;

  /**
   *   Computes a multiplication between a matrix and a float.
   *    NOTE: This does not change the scale of this Matrix. It'll multiply
   *    all the elements by the value.
   * @param other
   *  The float value to be multiplied.
   *
   * @return Matrix4
   *  A new matrix result.
   **/
  FORCEINLINE Matrix4
  operator*(const float& value) const;

  /** 
   *   Computes a multiplication between a matrix and a float. Modyfing this matrix.
   * @param other
   *  The float value to be multiplied.
   **/
  FORCEINLINE Matrix4&
  operator*=(float Other);

  /** 
   *   Assigns to this all the given values from another matrix4.
   * 
   * @param assignable
   *  Matrix4 to be taken as source.
   * 
   * @return Matrix4&
   *  Reference to this changed Matrix4.
   **/
  FORCEINLINE Matrix4&
  operator=(const Matrix4& assignable);

  /**
   *   Assigns to this all the given values from another rValue matrix4.
   *
   * @param assignable
   *  RValue matrix4 to be taken as source.
   *
   * @return Matrix4&
   *  Reference to this changed Matrix4.
   **/
  FORCEINLINE Matrix4&
  operator=(const Matrix4&& move) noexcept;

  /** 
   *   Checks if this matrix is exactly equal to another Matrix4.
   * 
   * @param other
   *  The other matrix to check against this one.
   * 
   * @return bool
   *  True if both matrices are exactly equals.
   **/
  FORCEINLINE bool
  operator==(const Matrix4& other) const;

 private:
  /**
  * Logical string representations of this Matrix.
  *
  * @param _output
  *   The output stream where to write.
  *
  * @param
  *   Matrix4 The reference to this Matrix to be out.
  *
  * @return
  *   Ostream of this class to string.
  */
  friend ostream& operator<<(ostream& output, const Matrix4& matrix4)
  {
    for (auto& Row : matrix4.m4x4) {
      for (const float& Value : Row){
        output << Value << ", ";
       }
        output << std::endl;
     }
     return output;
   }

 public:
  union
  {
    struct
    {
      float m00;
      float m01;
      float m02;
      float m03;

      float m10;
      float m11;
      float m12;
      float m13;

      float m20;
      float m21;
      float m22;
      float m23;

      float m30;
      float m31;
      float m32;
      float m33;
    };
    float m4x4[4][4];
    float m16[16];

    struct 
    {
      Array<float, 4> FirstRow;
      Array<float, 4> SecondRow;
      Array<float, 4> ThirdRow;
      Array<float, 4> FourthRow;
    };
  };

  /** 
   *   All zero matrix.
   **/
  static CH_UTILITY_EXPORT const Matrix4 ZERO;

  /**
   *   Identity Matrix.
   **/
  static CH_UTILITY_EXPORT const Matrix4 IDENTITY;

  /**
   *   All one matrix.
   **/
  static CH_UTILITY_EXPORT const Matrix4 UNITY;
 
};

/************************************************************************/
/*
 * Implementation
 */                                                                     
/************************************************************************/

/*
*/
FORCEINLINE
Matrix4::Matrix4(float _m00, float _m01, float _m02, float _m03, 
                 float _m10, float _m11, float _m12, float _m13, 
                 float _m20, float _m21, float _m22, float _m23, 
                 float _m30, float _m31, float _m32, float _m33)
  : m00(_m00), m01(_m01), m02(_m02), m03(_m03),
    m10(_m10), m11(_m11), m12(_m12), m13(_m13),
    m20(_m20), m21(_m21), m22(_m22), m23(_m23),
    m30(_m30), m31(_m31), m32(_m32), m33(_m33)
{}

/*
*/
FORCEINLINE 
Matrix4::Matrix4(const Matrix4& copy)
  : m00(copy.m00), m01(copy.m01), m02(copy.m02), m03(copy.m03),
    m10(copy.m10), m11(copy.m11), m12(copy.m12), m13(copy.m13),
    m20(copy.m20), m21(copy.m21), m22(copy.m22), m23(copy.m23),
    m30(copy.m30), m31(copy.m31), m32(copy.m32), m33(copy.m33)
{}

/*
*/
FORCEINLINE
Matrix4::Matrix4(const Vector4& firscRow, 
                 const Vector4& secondRow, 
                 const Vector4& thirdRow, 
                 const Vector4& fourthRow)
  : m00(firscRow.x), m01(firscRow.y), m02(firscRow.z), m03(firscRow.w),
    m10(secondRow.x), m11(secondRow.y), m12(secondRow.z), m13(secondRow.w),
    m20(thirdRow.x), m21(thirdRow.y), m22(thirdRow.z), m23(thirdRow.w),
    m30(fourthRow.x), m31(fourthRow.y), m32(fourthRow.z), m33(fourthRow.w)
{}

/*
*/
FORCEINLINE
Matrix4::Matrix4(const Plane& InX,
                 const Plane& InY, 
                 const Plane& InZ, 
                 const Plane& InW ) {
    m00 = InX.x; m01 = InX.y;  m02 = InX.z;  m03 = InX.w;
    m10 = InY.x; m11 = InY.y;  m12 = InY.z;  m13 = InY.w;
    m20 = InZ.x; m21 = InZ.y;  m22 = InZ.z;  m23 = InZ.w;
    m30 = InW.x; m31 = InW.y;  m32 = InW.z;  m33 = InW.w;
}



/*
*/
FORCEINLINE void
Matrix4::setIdentity()
{
  *this = Matrix4::IDENTITY;
}

/*
*/
FORCEINLINE Matrix4
Matrix4::getTransposed() const
{
  Matrix4 Result;

  Result.m4x4[0][0] = m4x4[0][0];
  Result.m4x4[0][1] = m4x4[1][0];
  Result.m4x4[0][2] = m4x4[2][0];
  Result.m4x4[0][3] = m4x4[3][0];

  Result.m4x4[1][0] = m4x4[0][1];
  Result.m4x4[1][1] = m4x4[1][1];
  Result.m4x4[1][2] = m4x4[2][1];
  Result.m4x4[1][3] = m4x4[3][1];

  Result.m4x4[2][0] = m4x4[0][2];
  Result.m4x4[2][1] = m4x4[1][2];
  Result.m4x4[2][2] = m4x4[2][2];
  Result.m4x4[2][3] = m4x4[3][2];

  Result.m4x4[3][0] = m4x4[0][3];
  Result.m4x4[3][1] = m4x4[1][3];
  Result.m4x4[3][2] = m4x4[2][3];
  Result.m4x4[3][3] = m4x4[3][3];

  return Result;
}

/*
*/
FORCEINLINE Matrix4&
Matrix4::transposed()
{
   *this = getTransposed();
   return *this;
}

/*
*/
FORCEINLINE float
Matrix4::getDeterminant() const
{
  return
    m4x4[0][0] * (
      m4x4[1][1] * (m4x4[2][2] * m4x4[3][3] - m4x4[2][3] * m4x4[3][2]) -
      m4x4[2][1] * (m4x4[1][2] * m4x4[3][3] - m4x4[1][3] * m4x4[3][2]) +
      m4x4[3][1] * (m4x4[1][2] * m4x4[2][3] - m4x4[1][3] * m4x4[2][2])) -
    m4x4[1][0] * (
      m4x4[0][1] * (m4x4[2][2] * m4x4[3][3] - m4x4[2][3] * m4x4[3][2]) -
      m4x4[2][1] * (m4x4[0][2] * m4x4[3][3] - m4x4[0][3] * m4x4[3][2]) +
      m4x4[3][1] * (m4x4[0][2] * m4x4[2][3] - m4x4[0][3] * m4x4[2][2])) +
    m4x4[2][0] * (
      m4x4[0][1] * (m4x4[1][2] * m4x4[3][3] - m4x4[1][3] * m4x4[3][2]) -
      m4x4[1][1] * (m4x4[0][2] * m4x4[3][3] - m4x4[0][3] * m4x4[3][2]) +
      m4x4[3][1] * (m4x4[0][2] * m4x4[1][3] - m4x4[0][3] * m4x4[1][2])) -
    m4x4[3][0] * (
      m4x4[0][1] * (m4x4[1][2] * m4x4[2][3] - m4x4[1][3] * m4x4[2][2]) -
      m4x4[1][1] * (m4x4[0][2] * m4x4[2][3] - m4x4[0][3] * m4x4[2][2]) +
      m4x4[2][1] * (m4x4[0][2] * m4x4[1][3] - m4x4[0][3] * m4x4[1][2]));
}

/*
*/
FORCEINLINE Matrix4
Matrix4::getInverse()
{
  Matrix4 Res;
  const float	Det = getDeterminant();
  if (0.0f == Det) {
    Res = Matrix4::IDENTITY;
    //TODO: Throw a warning
  }
  else {
    Matrix4 Temp;

    Temp.m4x4[0][0] = m4x4[2][2] * m4x4[3][3] - m4x4[2][3] * m4x4[3][2];
    Temp.m4x4[0][1] = m4x4[1][2] * m4x4[3][3] - m4x4[1][3] * m4x4[3][2];
    Temp.m4x4[0][2] = m4x4[1][2] * m4x4[2][3] - m4x4[1][3] * m4x4[2][2];

    Temp.m4x4[1][0] = m4x4[2][2] * m4x4[3][3] - m4x4[2][3] * m4x4[3][2];
    Temp.m4x4[1][1] = m4x4[0][2] * m4x4[3][3] - m4x4[0][3] * m4x4[3][2];
    Temp.m4x4[1][2] = m4x4[0][2] * m4x4[2][3] - m4x4[0][3] * m4x4[2][2];

    Temp.m4x4[2][0] = m4x4[1][2] * m4x4[3][3] - m4x4[1][3] * m4x4[3][2];
    Temp.m4x4[2][1] = m4x4[0][2] * m4x4[3][3] - m4x4[0][3] * m4x4[3][2];
    Temp.m4x4[2][2] = m4x4[0][2] * m4x4[1][3] - m4x4[0][3] * m4x4[1][2];

    Temp.m4x4[3][0] = m4x4[1][2] * m4x4[2][3] - m4x4[1][3] * m4x4[2][2];
    Temp.m4x4[3][1] = m4x4[0][2] * m4x4[2][3] - m4x4[0][3] * m4x4[2][2];
    Temp.m4x4[3][2] = m4x4[0][2] * m4x4[1][3] - m4x4[0][3] * m4x4[1][2];

    const float Det0 = m4x4[1][1] * Temp.m4x4[0][0] - m4x4[2][1] * Temp.m4x4[0][1] + m4x4[3][1] * Temp.m4x4[0][2];
    const float Det1 = m4x4[0][1] * Temp.m4x4[1][0] - m4x4[2][1] * Temp.m4x4[1][1] + m4x4[3][1] * Temp.m4x4[1][2];
    const float Det2 = m4x4[0][1] * Temp.m4x4[2][0] - m4x4[1][1] * Temp.m4x4[2][1] + m4x4[3][1] * Temp.m4x4[2][2];
    const float Det3 = m4x4[0][1] * Temp.m4x4[3][0] - m4x4[1][1] * Temp.m4x4[3][1] + m4x4[2][1] * Temp.m4x4[3][2];

    const float Determinant = m4x4[0][0] * Det0 - m4x4[1][0] * Det1 + m4x4[2][0] * Det2 - m4x4[3][0] * Det3;
    const float	RDet = 1.0f / Determinant;

    Res.m4x4[0][0] =  RDet * Det0;
    Res.m4x4[0][1] = -RDet * Det1;
    Res.m4x4[0][2] =  RDet * Det2;
    Res.m4x4[0][3] = -RDet * Det3;
    Res.m4x4[1][0] = -RDet * (m4x4[1][0] * Temp.m4x4[0][0] -
                              m4x4[2][0] * Temp.m4x4[0][1] +
                              m4x4[3][0] * Temp.m4x4[0][2]);
    Res.m4x4[1][1] =  RDet * (m4x4[0][0] * Temp.m4x4[1][0] -
                              m4x4[2][0] * Temp.m4x4[1][1] +
                              m4x4[3][0] * Temp.m4x4[1][2]);
    Res.m4x4[1][2] = -RDet * (m4x4[0][0] * Temp.m4x4[2][0] -
                              m4x4[1][0] * Temp.m4x4[2][1] +
                              m4x4[3][0] * Temp.m4x4[2][2]);
    Res.m4x4[1][3] =  RDet * (m4x4[0][0] * Temp.m4x4[3][0] -
                              m4x4[1][0] * Temp.m4x4[3][1] +
                              m4x4[2][0] * Temp.m4x4[3][2]);
    Res.m4x4[2][0] =  RDet * (m4x4[1][0] * (m4x4[2][1] * m4x4[3][3] - m4x4[2][3] * m4x4[3][1]) -
                              m4x4[2][0] * (m4x4[1][1] * m4x4[3][3] - m4x4[1][3] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[1][1] * m4x4[2][3] - m4x4[1][3] * m4x4[2][1]));
    Res.m4x4[2][1] = -RDet * (m4x4[0][0] * (m4x4[2][1] * m4x4[3][3] - m4x4[2][3] * m4x4[3][1]) -
                              m4x4[2][0] * (m4x4[0][1] * m4x4[3][3] - m4x4[0][3] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[0][1] * m4x4[2][3] - m4x4[0][3] * m4x4[2][1]));
    Res.m4x4[2][2] =  RDet * (m4x4[0][0] * (m4x4[1][1] * m4x4[3][3] - m4x4[1][3] * m4x4[3][1]) -
                              m4x4[1][0] * (m4x4[0][1] * m4x4[3][3] - m4x4[0][3] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[0][1] * m4x4[1][3] - m4x4[0][3] * m4x4[1][1]));
    Res.m4x4[2][3] = -RDet * (m4x4[0][0] * (m4x4[1][1] * m4x4[2][3] - m4x4[1][3] * m4x4[2][1]) -
                              m4x4[1][0] * (m4x4[0][1] * m4x4[2][3] - m4x4[0][3] * m4x4[2][1]) +
                              m4x4[2][0] * (m4x4[0][1] * m4x4[1][3] - m4x4[0][3] * m4x4[1][1]));
    Res.m4x4[3][0] = -RDet * (m4x4[1][0] * (m4x4[2][1] * m4x4[3][2] - m4x4[2][2] * m4x4[3][1]) -
                              m4x4[2][0] * (m4x4[1][1] * m4x4[3][2] - m4x4[1][2] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[1][1] * m4x4[2][2] - m4x4[1][2] * m4x4[2][1]));
    Res.m4x4[3][1] =  RDet * (m4x4[0][0] * (m4x4[2][1] * m4x4[3][2] - m4x4[2][2] * m4x4[3][1]) -
                              m4x4[2][0] * (m4x4[0][1] * m4x4[3][2] - m4x4[0][2] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[0][1] * m4x4[2][2] - m4x4[0][2] * m4x4[2][1]));
    Res.m4x4[3][2] = -RDet * (m4x4[0][0] * (m4x4[1][1] * m4x4[3][2] - m4x4[1][2] * m4x4[3][1]) -
                              m4x4[1][0] * (m4x4[0][1] * m4x4[3][2] - m4x4[0][2] * m4x4[3][1]) +
                              m4x4[3][0] * (m4x4[0][1] * m4x4[1][2] - m4x4[0][2] * m4x4[1][1]));
    Res.m4x4[3][3] =  RDet * (m4x4[0][0] * (m4x4[1][1] * m4x4[2][2] - m4x4[1][2] * m4x4[2][1]) -
                              m4x4[1][0] * (m4x4[0][1] * m4x4[2][2] - m4x4[0][2] * m4x4[2][1]) +
                              m4x4[2][0] * (m4x4[0][1] * m4x4[1][2] - m4x4[0][2] * m4x4[1][1]));
  }
  return Res;
}


/*
*/
FORCEINLINE Matrix4
Matrix4::operator*(const Matrix4& other) const
{
  Matrix4 Res;

  Res.m4x4[0][0] = m4x4[0][0] * other.m4x4[0][0] + m4x4[0][1] * other.m4x4[1][0] +
                   m4x4[0][2] * other.m4x4[2][0] + m4x4[0][3] * other.m4x4[3][0];
  Res.m4x4[0][1] = m4x4[0][0] * other.m4x4[0][1] + m4x4[0][1] * other.m4x4[1][1] +
                   m4x4[0][2] * other.m4x4[2][1] + m4x4[0][3] * other.m4x4[3][1];
  Res.m4x4[0][2] = m4x4[0][0] * other.m4x4[0][2] + m4x4[0][1] * other.m4x4[1][2] +
                   m4x4[0][2] * other.m4x4[2][2] + m4x4[0][3] * other.m4x4[3][2];
  Res.m4x4[0][3] = m4x4[0][0] * other.m4x4[0][3] + m4x4[0][1] * other.m4x4[1][3] +
                   m4x4[0][2] * other.m4x4[2][3] + m4x4[0][3] * other.m4x4[3][3];
                                   
  Res.m4x4[1][0] = m4x4[1][0] * other.m4x4[0][0] + m4x4[1][1] * other.m4x4[1][0] +
                   m4x4[1][2] * other.m4x4[2][0] + m4x4[1][3] * other.m4x4[3][0];
  Res.m4x4[1][1] = m4x4[1][0] * other.m4x4[0][1] + m4x4[1][1] * other.m4x4[1][1] +
                   m4x4[1][2] * other.m4x4[2][1] + m4x4[1][3] * other.m4x4[3][1];
  Res.m4x4[1][2] = m4x4[1][0] * other.m4x4[0][2] + m4x4[1][1] * other.m4x4[1][2] +
                   m4x4[1][2] * other.m4x4[2][2] + m4x4[1][3] * other.m4x4[3][2];
  Res.m4x4[1][3] = m4x4[1][0] * other.m4x4[0][3] + m4x4[1][1] * other.m4x4[1][3] +
                   m4x4[1][2] * other.m4x4[2][3] + m4x4[1][3] * other.m4x4[3][3];
                                   
  Res.m4x4[2][0] = m4x4[2][0] * other.m4x4[0][0] + m4x4[2][1] * other.m4x4[1][0] +
                   m4x4[2][2] * other.m4x4[2][0] + m4x4[2][3] * other.m4x4[3][0];
  Res.m4x4[2][1] = m4x4[2][0] * other.m4x4[0][1] + m4x4[2][1] * other.m4x4[1][1] +
                   m4x4[2][2] * other.m4x4[2][1] + m4x4[2][3] * other.m4x4[3][1];
  Res.m4x4[2][2] = m4x4[2][0] * other.m4x4[0][2] + m4x4[2][1] * other.m4x4[1][2] +
                   m4x4[2][2] * other.m4x4[2][2] + m4x4[2][3] * other.m4x4[3][2];
  Res.m4x4[2][3] = m4x4[2][0] * other.m4x4[0][3] + m4x4[2][1] * other.m4x4[1][3] +
                   m4x4[2][2] * other.m4x4[2][3] + m4x4[2][3] * other.m4x4[3][3];

  Res.m4x4[3][0] = m4x4[3][0] * other.m4x4[0][0] + m4x4[3][1] * other.m4x4[1][0] +
                   m4x4[3][2] * other.m4x4[2][0] + m4x4[3][3] * other.m4x4[3][0];
  Res.m4x4[3][1] = m4x4[3][0] * other.m4x4[0][1] + m4x4[3][1] * other.m4x4[1][1] +
                   m4x4[3][2] * other.m4x4[2][1] + m4x4[3][3] * other.m4x4[3][1];
  Res.m4x4[3][2] = m4x4[3][0] * other.m4x4[0][2] + m4x4[3][1] * other.m4x4[1][2] +
                   m4x4[3][2] * other.m4x4[2][2] + m4x4[3][3] * other.m4x4[3][2];
  Res.m4x4[3][3] = m4x4[3][0] * other.m4x4[0][3] + m4x4[3][1] * other.m4x4[1][3] +
                   m4x4[3][2] * other.m4x4[2][3] + m4x4[3][3] * other.m4x4[3][3];
  return Res;
}

/*
*/
FORCEINLINE void
Matrix4::operator*=(const Matrix4& other)
{
  *this  = *this * other;
}

/*
*/
FORCEINLINE Matrix4
Matrix4::operator+(const Matrix4& other) const
{
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m4x4[i][j] = m4x4[i][j] + other.m4x4[i][j];
    }
  }

  return Result;
}

/*
*/
FORCEINLINE Matrix4
Matrix4::operator-(const Matrix4& other) const
{
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m4x4[i][j] = m4x4[i][j] - other.m4x4[i][j];
    }
  }

  return Result;
}

/*
*/
FORCEINLINE Matrix4
Matrix4::operator*(const float& value) const
{
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m4x4[i][j] = m4x4[i][j] * value;
    }
  }

  return Result;
}


/*
*/
FORCEINLINE Matrix4&
Matrix4::operator*=(float other)
{
  for (auto& Row : m4x4) {
    for (float& Value : Row) {
      Value *= other;
    }
  }

  return *this;
}

/*
*/
FORCEINLINE Matrix4&
Matrix4::operator=(const Matrix4& assignable)
{
  m00 = assignable.m00;
  m01 = assignable.m01;
  m02 = assignable.m02;
  m03 = assignable.m03;
  m10 = assignable.m10;
  m11 = assignable.m11;
  m12 = assignable.m12;
  m13 = assignable.m13;
  m20 = assignable.m20;
  m21 = assignable.m21;
  m22 = assignable.m22;
  m23 = assignable.m23;
  m30 = assignable.m30;
  m31 = assignable.m31;
  m32 = assignable.m32;
  m33 = assignable.m33;
  return *this;
}

/*
*/
FORCEINLINE Matrix4&
Matrix4::operator=(const Matrix4&& _move) noexcept
{
  m00 = _move.m00;
  m01 = _move.m01;
  m02 = _move.m02;
  m03 = _move.m03;
  m10 = _move.m10;
  m11 = _move.m11;
  m12 = _move.m12;
  m13 = _move.m13;
  m20 = _move.m20;
  m21 = _move.m21;
  m22 = _move.m22;
  m23 = _move.m23;
  m30 = _move.m30;
  m31 = _move.m31;
  m32 = _move.m32;
  m33 = _move.m33;
  return *this;
}

/*
*/
FORCEINLINE bool
Matrix4::operator==(const Matrix4& other) const
{
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      if (m4x4[i][j] != other.m4x4[i][j]) { return false; }
    }
  }
  return true;
}
}

/************************************************************************/
/*
 * Helper Matrices.
 */                                                                     
/************************************************************************/
namespace chEngineSDK {
 /*
  * Description:
  *     Translation Matrix.
  *
  * Sample usage:
  *  const TranslationMatrix posMatrix(Vector3(2.0f, 3.0f, 150.0f))
  *  posMatrix.FourthRow.x = 1.0f;
  *  posMatrix.setIdentity();
  *
  */
class TranslationMatrix : public Matrix4
{
public:
  /**
   *   Constructor translation matrix given a vector.
   *
   * @param translation
   *    Initial position for this Matrix to be.
   **/
  TranslationMatrix(const Vector3& translation)
    : Matrix4(Vector4(1.0f, 0.0f, 0.0f, 0.0f),
              Vector4(0.0f, 1.0f, 0.0f, 0.0f),
              Vector4(0.0f, 0.0f, 1.0f, 0.0f),
              Vector4(translation, 1.0f))
  {}
};

/************************************************************************/
/*
 * Rotation - Translation Matrix
 */
 /************************************************************************/
 /*
  * Description: 
  *     Rotation Translation matrix class.
  *
  * Sample usage:
  *   RotationTranslationMatrix RTM(Rotator(90.f,90.f,90.f), Vector::UNITY);
  *
  */

  //Refer:
  //http://theamazingking.com/ogl-matrix.php
  //https://robotics.stackexchange.com/questions/18898/rotation-matrices-using-dh-parameters
  //http://www.c-jump.com/bcc/common/Talk3/Math/Matrices/W01_0120_3d_rotation.htm
  //https://paroj.github.io/gltut/Positioning/Tut06%20Rotation.html#idp5620
  //https://github.com/YclepticStudios/gmath/blob/master/src/Quaternion.hpp
class RotationTranslationMatrix : public Matrix4
{
public:

 /** 
  *   Constructor base for Rotation-Translation matrix.
  **/
  FORCEINLINE RotationTranslationMatrix(const Rotator& rotator,
                                        const Vector3& origin)
  {
    const float SP = Math::sin(rotator.pitch);
    const float CP = Math::cos(rotator.pitch);

    const float SY = Math::sin(rotator.yaw);
    const float CY = Math::cos(rotator.yaw);

    const float SR = Math::sin(rotator.roll);
    const float CR = Math::cos(rotator.roll);

    //Short for mult Rx, Ry, Rz 
    m4x4[0][0] = CP * CY;
    m4x4[0][1] = CP * SY;
    m4x4[0][2] = SP;
    m4x4[0][3] = 0.f;

    m4x4[1][0] = SR * SP * CY - CR * SY;
    m4x4[1][1] = SR * SP * SY + CR * CY;
    m4x4[1][2] = -SR * CP;
    m4x4[1][3] = 0.f;

    m4x4[2][0] = -(CR * SP * CY + SR * SY);
    m4x4[2][1] = CY * SR - CR * SP * SY;
    m4x4[2][2] = CR * CP;
    m4x4[2][3] = 0.f;

    m4x4[3][0] = origin.x;
    m4x4[3][1] = origin.y;
    m4x4[3][2] = origin.z;
    m4x4[3][3] = 1.f;
  }
};

/************************************************************************/
/*
 * Rotation
 */
 /************************************************************************/
 /*
  * Description: 
  *     Rotation Matrix.
  *
  * Sample usage:
  *   RotationMatrix RM(90.f, 90.f, 90.f);
  *
  */
class RotationMatrix : public RotationTranslationMatrix
{
 public:
  RotationMatrix(const Rotator& rotator) 
    : RotationTranslationMatrix(rotator, Vector3::ZERO)
  {}
};

/************************************************************************/
/*
 * ScaleRotationTranslation Matrix.
 */                                                                     
/************************************************************************/

/*
 * Description: 
 *     Scale Rotation Translation Matrix.
 *
 * Sample usage:
 *  ScaleRotationTranslationMatrix SRTM(Vector3(1.f,1.f,1.f), 
                                        Rotator(90.f,90.f,90.f),
                                        Vector::ZERO);
 *
 */
class ScaleRotationTranslationMatrix : public Matrix4
{
public:
  ScaleRotationTranslationMatrix(const Vector3& scale,
                                 const Rotator& rotator,
                                 const Vector3& origin) {
    const float SP = Math::sin(rotator.pitch);
    const float CP = Math::cos(rotator.pitch);

    const float SY = Math::sin(rotator.yaw);
    const float CY = Math::cos(rotator.yaw);

    const float SR = Math::sin(rotator.roll);
    const float CR = Math::cos(rotator.roll);

    m4x4[0][0] = (CP * CY) * scale.x;
    m4x4[0][1] = (CP * SY) * scale.x;
    m4x4[0][2] = (SP)*scale.x;
    m4x4[0][3] = 0.f;
    
    m4x4[1][0] = (SR * SP * CY - CR * SY) * scale.y;
    m4x4[1][1] = (SR * SP * SY + CR * CY) * scale.y;
    m4x4[1][2] = (-SR * CP) * scale.y;
    m4x4[1][3] = 0.f;
    
    m4x4[2][0] = (-(CR * SP * CY + SR * SY)) * scale.z;
    m4x4[2][1] = (CY * SR - CR * SP * SY) * scale.z;
    m4x4[2][2] = (CR * CP) * scale.z;
    m4x4[2][3] = 0.f;
    
    m4x4[3][0] = origin.x;
    m4x4[3][1] = origin.y;
    m4x4[3][2] = origin.z;
    m4x4[3][3] = 1.f;
  }

};

/************************************************************************/
/*
 * View Matrix
 */                                                                     
/************************************************************************/
/*
 * Description: 
 *     Perspective Matrix class
 *
 * Sample usage: TODO
 *
 */
class PerspectiveMatrix : public Matrix4 {
 public:

  /**
   * @param HalfFOV 
   *    half Field of View in the Y direction
   * @param Width 
   *    view space width
   * @param Height v
   *    view space height
   * @param MinZ 
   *    distance to the near Z plane
   * @param MaxZ 
   *    distance to the far Z plane
   * @note that the FOV you pass in is actually half the FOV, unlike most
   *       perspective matrix functions (D3DXMatrixPerspectiveFovLH).
   */
  CH_UTILITY_EXPORT PerspectiveMatrix(const Radian& halfFOV, float width, 
                                      float height, float _near, float _far);
  
};

/*
 * Description: 
 *     Look at matrix class
 *
 * Sample usage:
 *
 */
class LookAtMatrix : public Matrix4
{
public:
  /**
   * @brief Creates a view matrix using an eye position, a look at position, and up vector.
   *        This does the same thing as D3DXMatrixLookAtLH.
   */
  FORCEINLINE LookAtMatrix(const Vector3& EyePosition,
                           const Vector3& LookAtPosition,
                           const Vector3& UpVector);
};

FORCEINLINE LookAtMatrix::LookAtMatrix(const Vector3& EyePosition,
                                       const Vector3& LookAtPosition,
                                       const Vector3& UpVector) {
  const Vector3 ZAxis = (LookAtPosition - EyePosition).getNormalized();
  const Vector3 XAxis = ZAxis.cross(UpVector).getNormalized();
  const Vector3 YAxis = XAxis.cross(ZAxis);

  m00 = XAxis.x;  m01 = YAxis.x;  m02 = ZAxis.x;  m03 = 0.0f;
  m10 = XAxis.y;  m11 = YAxis.y;  m12 = ZAxis.y;  m13 = 0.0f;
  m20 = XAxis.z;  m21 = YAxis.z;  m22 = ZAxis.z;  m23 = 0.0f;

  m30 = -EyePosition.dot(XAxis);
  m31 = -EyePosition.dot(YAxis);
  m32 = -EyePosition.dot(ZAxis);
  m33 = 1.0f;
}
}