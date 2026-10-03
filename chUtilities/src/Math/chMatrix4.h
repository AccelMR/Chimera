/************************************************************************/
/**
 * @file chMatrix4.h
 * @author AccelMR
 * @date 2022/02/20
 * @brief 4x4 matrix for transforms and projections.
 *
 * Coordinate system: X = forward, Y = right, Z = up, left-handed.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include "chSIMD.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

/**
 * Holds a 4x4 matrix for transforms and projections. It uses row vectors: a point is
 * transformed as point * matrix, so matrices are combined in the order they apply
 * (local * parent, world * view * projection) and the translation lives in row 3.
 * It is stored row by row, which is byte for byte what GLSL reads as a column-major
 * matrix for column vectors, so it is uploaded as is and the shader writes
 * projection * view * model * position.
 *
 * The class itself is not exported, only its functions that live in the .cpp, so the
 * constants can be inline constexpr and other modules fold them at compile time.
 */
class Matrix4
{
 public:
  /**
   * Leaves the values uninitialized, so arrays of matrices cost nothing to create.
   */
  Matrix4() = default;

  FORCEINLINE constexpr
  Matrix4(float m00, float m01, float m02, float m03,
          float m10, float m11, float m12, float m13,
          float m20, float m21, float m22, float m23,
          float m30, float m31, float m32, float m33) noexcept;

  FORCEINLINE
  Matrix4(const Vector4& row0, const Vector4& row1, const Vector4& row2,
          const Vector4& row3) noexcept;

  CH_UTILITY_EXPORT
  Matrix4(const Plane& row0, const Plane& row1, const Plane& row2, const Plane& row3) noexcept;

  FORCEINLINE void
  setIdentity() noexcept;

  NODISCARD FORCEINLINE Matrix4
  getTransposed() const noexcept;

  FORCEINLINE void
  transpose() noexcept;

  NODISCARD CH_UTILITY_EXPORT float
  getDeterminant() const noexcept;

  /**
   * Inverse of any matrix. Returns IDENTITY when the matrix has no inverse.
   */
  NODISCARD CH_UTILITY_EXPORT Matrix4
  getInverse() const noexcept;

  /**
   * Faster inverse for matrices whose last column is (0, 0, 0, 1): any mix of scale,
   * rotation and translation, but not a projection. Returns IDENTITY when the matrix
   * has no inverse.
   */
  NODISCARD CH_UTILITY_EXPORT Matrix4
  getInverseAffine() const noexcept;

  /**
   * Rotation of the matrix as Euler angles. Scale is ignored.
   */
  NODISCARD CH_UTILITY_EXPORT Rotator
  rotator() const noexcept;

  /**
   * Rotation of the matrix as a quaternion. The rotation rows must have unit length.
   */
  NODISCARD CH_UTILITY_EXPORT Quaternion
  toQuaternion() const noexcept;

  /**
   * Transforms a point (w = 1), so the translation is applied.
   */
  NODISCARD FORCEINLINE Vector4
  transformPosition(const Vector3& position) const noexcept;

  /**
   * Transforms a direction (w = 0), so the translation is ignored.
   */
  NODISCARD FORCEINLINE Vector4
  transformVector(const Vector3& direction) const noexcept;

  NODISCARD FORCEINLINE Vector4
  transformVector4(const Vector4& vector) const noexcept;

  NODISCARD FORCEINLINE float&
  at(int32 row, int32 column) noexcept;

  NODISCARD FORCEINLINE const float&
  at(int32 row, int32 column) const noexcept;

  NODISCARD FORCEINLINE const float*
  getRow(int32 row) const noexcept;

  NODISCARD FORCEINLINE constexpr const float*
  data() const noexcept;

  NODISCARD FORCEINLINE bool
  nearEqual(const Matrix4& other, float tolerance = Math::KINDA_SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE Matrix4
  operator*(const Matrix4& other) const noexcept;

  FORCEINLINE Matrix4&
  operator*=(const Matrix4& other) noexcept;

  NODISCARD FORCEINLINE Matrix4
  operator+(const Matrix4& other) const noexcept;

  NODISCARD FORCEINLINE Matrix4
  operator-(const Matrix4& other) const noexcept;

  /**
   * Multiplies every element, so it also scales the translation and the last column.
   */
  NODISCARD FORCEINLINE Matrix4
  operator*(float value) const noexcept;

  FORCEINLINE Matrix4&
  operator*=(float value) noexcept;

  NODISCARD FORCEINLINE bool
  operator==(const Matrix4& other) const noexcept;

  NODISCARD FORCEINLINE float*
  operator[](int32 row) noexcept;

  NODISCARD FORCEINLINE const float*
  operator[](int32 row) const noexcept;

 public:
  static const Matrix4 ZERO;
  static const Matrix4 IDENTITY;
  static const Matrix4 UNITY;

 protected:
  // Aligned so every row loads into one SIMD register.
  alignas(16) float m_data[4][4];
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Matrix4::Matrix4(float m00, float m01, float m02, float m03,
                 float m10, float m11, float m12, float m13,
                 float m20, float m21, float m22, float m23,
                 float m30, float m31, float m32, float m33) noexcept
 : m_data{{m00, m01, m02, m03},
          {m10, m11, m12, m13},
          {m20, m21, m22, m23},
          {m30, m31, m32, m33}}
{}

inline constexpr Matrix4 Matrix4::ZERO{0.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 0.0f};

inline constexpr Matrix4 Matrix4::IDENTITY{1.0f, 0.0f, 0.0f, 0.0f,
                                           0.0f, 1.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 1.0f, 0.0f,
                                           0.0f, 0.0f, 0.0f, 1.0f};

inline constexpr Matrix4 Matrix4::UNITY{1.0f, 1.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 1.0f, 1.0f};

/*
 */
FORCEINLINE
Matrix4::Matrix4(const Vector4& row0, const Vector4& row1, const Vector4& row2,
                 const Vector4& row3) noexcept
 : m_data{{row0.x, row0.y, row0.z, row0.w},
          {row1.x, row1.y, row1.z, row1.w},
          {row2.x, row2.y, row2.z, row2.w},
          {row3.x, row3.y, row3.z, row3.w}}
{}

/*
 */
FORCEINLINE void
Matrix4::setIdentity() noexcept
{
  *this = IDENTITY;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::getTransposed() const noexcept
{
  return Matrix4(m_data[0][0], m_data[1][0], m_data[2][0], m_data[3][0],
                 m_data[0][1], m_data[1][1], m_data[2][1], m_data[3][1],
                 m_data[0][2], m_data[1][2], m_data[2][2], m_data[3][2],
                 m_data[0][3], m_data[1][3], m_data[2][3], m_data[3][3]);
}

/*
 */
FORCEINLINE void
Matrix4::transpose() noexcept
{
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = row + 1; column < 4; ++column) {
      const float value = m_data[row][column];
      m_data[row][column] = m_data[column][row];
      m_data[column][row] = value;
    }
  }
}

/*
 */
FORCEINLINE Vector4
Matrix4::transformPosition(const Vector3& position) const noexcept
{
  SIMD::Float4 result = SIMD::loadAligned(m_data[3]);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[0]), position.x);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[1]), position.y);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[2]), position.z);

  alignas(16) float values[4];
  SIMD::storeAligned(values, result);
  return Vector4(values[0], values[1], values[2], values[3]);
}

/*
 */
FORCEINLINE Vector4
Matrix4::transformVector(const Vector3& direction) const noexcept
{
  SIMD::Float4 result = SIMD::multiply(SIMD::loadAligned(m_data[0]), direction.x);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[1]), direction.y);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[2]), direction.z);

  alignas(16) float values[4];
  SIMD::storeAligned(values, result);
  return Vector4(values[0], values[1], values[2], values[3]);
}

/*
 */
FORCEINLINE Vector4
Matrix4::transformVector4(const Vector4& vector) const noexcept
{
  SIMD::Float4 result = SIMD::multiply(SIMD::loadAligned(m_data[0]), vector.x);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[1]), vector.y);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[2]), vector.z);
  result = SIMD::multiplyAdd(result, SIMD::loadAligned(m_data[3]), vector.w);

  alignas(16) float values[4];
  SIMD::storeAligned(values, result);
  return Vector4(values[0], values[1], values[2], values[3]);
}

/*
 */
FORCEINLINE float&
Matrix4::at(int32 row, int32 column) noexcept
{
  CH_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
  return m_data[row][column];
}

/*
 */
FORCEINLINE const float&
Matrix4::at(int32 row, int32 column) const noexcept
{
  CH_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
  return m_data[row][column];
}

/*
 */
FORCEINLINE const float*
Matrix4::getRow(int32 row) const noexcept
{
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

/*
 */
FORCEINLINE constexpr const float*
Matrix4::data() const noexcept
{
  return &m_data[0][0];
}

/*
 */
FORCEINLINE bool
Matrix4::nearEqual(const Matrix4& other, float tolerance) const noexcept
{
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      if (Math::abs(m_data[row][column] - other.m_data[row][column]) > tolerance) {
        return false;
      }
    }
  }
  return true;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator*(const Matrix4& other) const noexcept
{
  const SIMD::Float4 otherRow0 = SIMD::loadAligned(other.m_data[0]);
  const SIMD::Float4 otherRow1 = SIMD::loadAligned(other.m_data[1]);
  const SIMD::Float4 otherRow2 = SIMD::loadAligned(other.m_data[2]);
  const SIMD::Float4 otherRow3 = SIMD::loadAligned(other.m_data[3]);

  // With row vectors, each result row is this row's values weighting the other rows.
  Matrix4 result;
  for (int32 row = 0; row < 4; ++row) {
    SIMD::Float4 combined = SIMD::multiply(otherRow0, m_data[row][0]);
    combined = SIMD::multiplyAdd(combined, otherRow1, m_data[row][1]);
    combined = SIMD::multiplyAdd(combined, otherRow2, m_data[row][2]);
    combined = SIMD::multiplyAdd(combined, otherRow3, m_data[row][3]);
    SIMD::storeAligned(result.m_data[row], combined);
  }
  return result;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::operator*=(const Matrix4& other) noexcept
{
  *this = *this * other;
  return *this;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator+(const Matrix4& other) const noexcept
{
  Matrix4 result;
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      result.m_data[row][column] = m_data[row][column] + other.m_data[row][column];
    }
  }
  return result;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator-(const Matrix4& other) const noexcept
{
  Matrix4 result;
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      result.m_data[row][column] = m_data[row][column] - other.m_data[row][column];
    }
  }
  return result;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator*(float value) const noexcept
{
  Matrix4 result;
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      result.m_data[row][column] = m_data[row][column] * value;
    }
  }
  return result;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::operator*=(float value) noexcept
{
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      m_data[row][column] *= value;
    }
  }
  return *this;
}

/*
 */
FORCEINLINE bool
Matrix4::operator==(const Matrix4& other) const noexcept
{
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      if (m_data[row][column] != other.m_data[row][column]) {
        return false;
      }
    }
  }
  return true;
}

/*
 */
FORCEINLINE float*
Matrix4::operator[](int32 row) noexcept
{
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

/*
 */
FORCEINLINE const float*
Matrix4::operator[](int32 row) const noexcept
{
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

} // namespace chEngineSDK
