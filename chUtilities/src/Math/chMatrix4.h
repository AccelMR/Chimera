/************************************************************************/
/**
 * @file chMatrix4.h
 * @author AccelMR
 * @date 2022/02/20
 *   Matrix 4 math file. Here goes all matrix 4 related content.
 *
 * Coordinate system being X = front, Z = up, Y = right
 *
 * Left hand
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

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
class CH_UTILITY_EXPORT Matrix4
{
 public:
  /*
   * @brief Default constructor
   */
  Matrix4() = default;

  /**
   * @brief Construct a Matrix4 from 16 individual float values
   */
  FORCEINLINE
  Matrix4(float _m00, float _m01, float _m02, float _m03, float _m10, float _m11, float _m12,
          float _m13, float _m20, float _m21, float _m22, float _m23, float _m30, float _m31,
          float _m32, float _m33);

  /**
   * @brief Copy constructor
   */
  FORCEINLINE
  Matrix4(const Matrix4& copy);

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
   * @brief Constructor from four Vectors 4
   */
  Matrix4(const Vector4& firstRow, const Vector4& secondRow, const Vector4& thirdRow,
          const Vector4& fourthRow);

  /**
   * @brief Constructor from four planes
   */
  Matrix4(const Plane& InX, const Plane& InY, const Plane& InZ, const Plane& InW);

  /************************************************************************/
  /*
   * Methods
   */
  /************************************************************************/

  /**
   * @brief Sets this matrix to identity
   */
  FORCEINLINE void
  setIdentity();

  /**
   * @brief Creates a new transposed matrix with this as reference
   *
   * @return Matrix4 The new matrix created from this
   */
  NODISCARD FORCEINLINE Matrix4
  getTransposed() const;

  /**
   * @brief Transpose this Matrix
   *
   * @return Matrix4& This reference
   */
  FORCEINLINE Matrix4&
  transposed();

  /**
   * @brief Computes the determinant of this matrix
   *
   * @return float The determinant value
   */
  NODISCARD FORCEINLINE float
  getDeterminant() const;

  /**
   * @brief Computes the inverse matrix from this
   *
   * @return Matrix4 A new Matrix4 created from this
   */
   NODISCARD Matrix4
  getInverse();

  /**
   * @brief Computes the conversion between this matrix to a rotator
   *
   * @return Rotator Representation of this matrix
   */
  NODISCARD  Rotator
  rotator() const;

  /**
   * @brief Transform a rotation matrix into a quaternion
   * NOTE: rotation part will need to be unit length for this to be right!
   *
   * @return Quaternion representing this rotation matrix
   */
  NODISCARD  Quaternion
  toQuaternion() const;

  /**
   * @brief Transforms a position with this matrix
   *
   * @param v Position vector to transform
   * @return Vector4 The transformed position
   */
  NODISCARD  Vector4
  transformPosition(const Vector3& v) const;

  /**
   * @brief Transform a direction vector by this matrix
   *
   * @param v The vector to be transformed
   * @return Vector4 The new vector transformed in world space
   */
  NODISCARD  Vector4
  transformVector(const Vector3& v) const;

  /**
   * @brief Transforms a vector 4 by this matrix
   *
   * @param p Vector to be transformed
   * @return Vector4 The new Vector created by the transformation
   */
  NODISCARD  Vector4
  transformVector4(const Vector4& p) const;

  /**
   * @brief Get the element at the specified row and column
   *
   * @param row Row index (0-3)
   * @param col Column index (0-3)
   * @return float& Reference to the element
   */
  NODISCARD FORCEINLINE float&
  at(int32 row, int32 col);

  /**
   * @brief Get the element at the specified row and column (const version)
   *
   * @param row Row index (0-3)
   * @param col Column index (0-3)
   * @return const float& Const reference to the element
   */
  NODISCARD FORCEINLINE const float&
  at(int32 row, int32 col) const;

  /**
   * @brief Get a specific row as an array of 4 floats
   *
   * @param row Row index (0-3)
   * @return const float* Pointer to the row
   */
  NODISCARD FORCEINLINE const float*
  getRow(int32 row) const;

  /**
   * @brief Get all elements as a contiguous array of 16 floats
   *
   * @return const float* Pointer to the elements
   */
  NODISCARD FORCEINLINE const float*
  data() const;

  /************************************************************************/
  /*
   * Operator overload
   */
  /************************************************************************/

  /**
   * @brief Computes a multiplication between two matrices
   *
   * @param other The other matrix to be multiplied
   * @return Matrix4 A new matrix result
   */
  NODISCARD FORCEINLINE Matrix4
  operator*(const Matrix4& other) const;

  /**
   * @brief Computes a multiplication between two matrices, modifying this matrix
   *
   * @param other The other matrix to be multiplied
   */
  FORCEINLINE void
  operator*=(const Matrix4& other);

  /**
   * @brief Computes a sum between two matrices
   *
   * @param other The other matrix to be summed
   * @return Matrix4 A new matrix result
   */
  NODISCARD FORCEINLINE Matrix4
  operator+(const Matrix4& other) const;

  /**
   * @brief Computes a subtraction between two matrices
   *
   * @param other The other matrix to be subtracted
   * @return Matrix4 A new matrix result
   */
  NODISCARD FORCEINLINE Matrix4
  operator-(const Matrix4& other) const;

  /**
   * @brief Computes a multiplication between a matrix and a float
   * NOTE: This does not change the scale of this Matrix. It'll multiply
   * all the elements by the value.
   *
   * @param value The float value to be multiplied
   * @return Matrix4 A new matrix result
   */
  NODISCARD FORCEINLINE Matrix4
  operator*(float value) const;

  /**
   * @brief Computes a multiplication between a matrix and a float, modifying this matrix
   *
   * @param value The float value to be multiplied
   * @return Matrix4& Reference to this modified matrix
   */
  FORCEINLINE Matrix4&
  operator*=(float value);

  /**
   * @brief Assigns to this all the given values from another matrix4
   *
   * @param assignable Matrix4 to be taken as source
   * @return Matrix4& Reference to this changed Matrix4
   */
  FORCEINLINE Matrix4&
  operator=(const Matrix4& assignable);

  /**
   * @brief Assigns to this all the given values from another rValue matrix4
   *
   * @param move RValue matrix4 to be taken as source
   * @return Matrix4& Reference to this changed Matrix4
   */
  FORCEINLINE Matrix4&
  operator=(Matrix4&& move) noexcept;

  /**
   * @brief Checks if this matrix is exactly equal to another Matrix4
   *
   * @param other The other matrix to check against this one
   * @return bool True if both matrices are exactly equals
   */
  NODISCARD FORCEINLINE bool
  operator==(const Matrix4& other) const;

  /**
   * @brief Subscript operator for accessing matrix elements
   *
   * @param row Row index (0-3)
   * @return float* Pointer to the row
   */
  NODISCARD FORCEINLINE float*
  operator[](int32 row);

  /**
   * @brief Subscript operator for accessing matrix elements (const version)
   *
   * @param row Row index (0-3)
   * @return const float* Pointer to the row
   */
  NODISCARD FORCEINLINE const float*
  operator[](int32 row) const;

 private:
  /**
   * Logical string representations of this Matrix.
   *
   * @param output The output stream where to write
   * @param matrix4 The reference to this Matrix to be out
   * @return Ostream of this class to string
   */
  friend ostream&
  operator<<(ostream& output, const Matrix4& matrix4);

 public:
  /**
   * All zero matrix
   */
  static  const Matrix4 ZERO;

  /**
   * Identity Matrix
   */
  static  const Matrix4 IDENTITY;

  /**
   * All one matrix
   */
  static  const Matrix4 UNITY;

 protected:
  // Matrix data stored in row-major order (m[row][column])
  float m_data[4][4];
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE
Matrix4::Matrix4(float _m00, float _m01, float _m02, float _m03, float _m10, float _m11,
                 float _m12, float _m13, float _m20, float _m21, float _m22, float _m23,
                 float _m30, float _m31, float _m32, float _m33) {
  m_data[0][0] = _m00;
  m_data[0][1] = _m01;
  m_data[0][2] = _m02;
  m_data[0][3] = _m03;
  m_data[1][0] = _m10;
  m_data[1][1] = _m11;
  m_data[1][2] = _m12;
  m_data[1][3] = _m13;
  m_data[2][0] = _m20;
  m_data[2][1] = _m21;
  m_data[2][2] = _m22;
  m_data[2][3] = _m23;
  m_data[3][0] = _m30;
  m_data[3][1] = _m31;
  m_data[3][2] = _m32;
  m_data[3][3] = _m33;
}

/*
 */
FORCEINLINE
Matrix4::Matrix4(const Matrix4& copy) {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      m_data[i][j] = copy.m_data[i][j];
    }
  }
}

/*
 */
FORCEINLINE float&
Matrix4::at(int32 row, int32 col) {
  CH_ASSERT(row >= 0 && row < 4 && col >= 0 && col < 4);
  return m_data[row][col];
}

/*
 */
FORCEINLINE const float&
Matrix4::at(int32 row, int32 col) const {
  CH_ASSERT(row >= 0 && row < 4 && col >= 0 && col < 4);
  return m_data[row][col];
}

/*
 */
FORCEINLINE const float*
Matrix4::getRow(int32 row) const {
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

/*
 */
FORCEINLINE const float*
Matrix4::data() const {
  return &m_data[0][0];
}

/*
 */
FORCEINLINE float*
Matrix4::operator[](int32 row) {
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

/*
 */
FORCEINLINE const float*
Matrix4::operator[](int32 row) const {
  CH_ASSERT(row >= 0 && row < 4);
  return m_data[row];
}

/*
 */
FORCEINLINE void
Matrix4::setIdentity() {
  *this = Matrix4::IDENTITY;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::getTransposed() const {
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m_data[i][j] = m_data[j][i];
    }
  }

  return Result;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::transposed() {
  *this = getTransposed();
  return *this;
}

/*
 */
FORCEINLINE float
Matrix4::getDeterminant() const {
  return m_data[0][0] *
             (m_data[1][1] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
              m_data[2][1] * (m_data[1][2] * m_data[3][3] - m_data[1][3] * m_data[3][2]) +
              m_data[3][1] * (m_data[1][2] * m_data[2][3] - m_data[1][3] * m_data[2][2])) -
         m_data[1][0] *
             (m_data[0][1] * (m_data[2][2] * m_data[3][3] - m_data[2][3] * m_data[3][2]) -
              m_data[2][1] * (m_data[0][2] * m_data[3][3] - m_data[0][3] * m_data[3][2]) +
              m_data[3][1] * (m_data[0][2] * m_data[2][3] - m_data[0][3] * m_data[2][2])) +
         m_data[2][0] *
             (m_data[0][1] * (m_data[1][2] * m_data[3][3] - m_data[1][3] * m_data[3][2]) -
              m_data[1][1] * (m_data[0][2] * m_data[3][3] - m_data[0][3] * m_data[3][2]) +
              m_data[3][1] * (m_data[0][2] * m_data[1][3] - m_data[0][3] * m_data[1][2])) -
         m_data[3][0] *
             (m_data[0][1] * (m_data[1][2] * m_data[2][3] - m_data[1][3] * m_data[2][2]) -
              m_data[1][1] * (m_data[0][2] * m_data[2][3] - m_data[0][3] * m_data[2][2]) +
              m_data[2][1] * (m_data[0][2] * m_data[1][3] - m_data[0][3] * m_data[1][2]));
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator*(const Matrix4& other) const {
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m_data[i][j] =
          m_data[i][0] * other.m_data[0][j] + m_data[i][1] * other.m_data[1][j] +
          m_data[i][2] * other.m_data[2][j] + m_data[i][3] * other.m_data[3][j];
    }
  }

  return Result;
}

/*
 */
FORCEINLINE void
Matrix4::operator*=(const Matrix4& other) {
  *this = *this * other;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator+(const Matrix4& other) const {
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m_data[i][j] = m_data[i][j] + other.m_data[i][j];
    }
  }

  return Result;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator-(const Matrix4& other) const {
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m_data[i][j] = m_data[i][j] - other.m_data[i][j];
    }
  }

  return Result;
}

/*
 */
FORCEINLINE Matrix4
Matrix4::operator*(float value) const {
  Matrix4 Result;

  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      Result.m_data[i][j] = m_data[i][j] * value;
    }
  }

  return Result;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::operator*=(float value) {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      m_data[i][j] *= value;
    }
  }

  return *this;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::operator=(const Matrix4& assignable) {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      m_data[i][j] = assignable.m_data[i][j];
    }
  }
  return *this;
}

/*
 */
FORCEINLINE Matrix4&
Matrix4::operator=(Matrix4&& move) noexcept {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      m_data[i][j] = move.m_data[i][j];
    }
  }
  return *this;
}

/*
 */
FORCEINLINE bool
Matrix4::operator==(const Matrix4& other) const {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      if (m_data[i][j] != other.m_data[i][j]) {
        return false;
      }
    }
  }
  return true;
}

// Implementation of friend output operator
inline ostream&
operator<<(ostream& output, const Matrix4& matrix4) {
  for (int32 i = 0; i < 4; ++i) {
    for (int32 j = 0; j < 4; ++j) {
      output << matrix4.m_data[i][j] << ", ";
    }
    output << std::endl;
  }
  return output;
}
} // namespace chEngineSDK
