/************************************************************************/
/**
 * @file chQuaternion.h
 * @author AccelMR
 * @date 2022/03/29
 *   Quaternion file that holds all the API for this math component.
 *
 *
 *   Quaternion multiplication done right - left.
 *
 * Order matters when composing quaternions: C = A * B will yield a quaternion
 * C that logically first applies B then A to any subsequent transformation
 * (right first, then left).
 *
 *
 * Coordinate system being X = front, Z = up, Y = right
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK {
/*
 * Description:
 *     Quaternion math component to handle rotations.
 *     Represented as (x, y, z, w) where (x, y, z) is the vector part
 *     and w is the scalar part.
 *
 * Sample usage:
 *    Quaternion q(Math::PI, Math::PI, Math::PI);
 *    Vector3 rotated = q.rotateVector(originalVector);
 */
class CH_UTILITY_EXPORT Quaternion
{
 public:
  /*
   * @brief Default constructor
   */
  Quaternion();

  /**
   * @brief Copy constructor
   */
  FORCEINLINE
  Quaternion(const Quaternion& q);

  /**
   * @brief Constructor with 4 components (x, y, z, w)
   */
  FORCEINLINE
  Quaternion(float _x, float _y, float _z, float _w);

  /**
   * @brief Constructor from a Rotator
   */
  explicit Quaternion(const Rotator& rotator);

  /**
   * @brief Constructor from a rotation Matrix4
   */
  explicit Quaternion(const Matrix4& m4);

  /**
   * @brief Constructor from Vector4 (x, y, z, w)
   */
  explicit Quaternion(const Vector4& v4);

  /**
   * @brief Constructor from an axis and angle
   *
   * @param axis The axis of rotation (should be normalized)
   * @param angle The angle to rotate in degrees
   */
  Quaternion(const Vector3& axis, const Degree& angle);

  /*
   * @brief Default destructor
   */
  ~Quaternion() = default;

  /************************************************************************/
  /*
   * Methods
   */
  /************************************************************************/

  /**
   * @brief Normalizes this quaternion
   */
  FORCEINLINE void
  normalize();

  /**
   * @brief Creates a new normalized quaternion from this one
   *
   * @return Quaternion The new quaternion created
   */
  NODISCARD FORCEINLINE Quaternion
  getNormalized() const;

  /**
   * @brief Conjugates this quaternion (negates the vector part)
   */
  FORCEINLINE void
  conjugate();

  /**
   * @brief Creates a rotator from this quaternion
   *
   * @return Rotator New rotator created from this
   */
  NODISCARD Rotator
  toRotator() const;

  /**
   * @brief Creates a new conjugated quaternion from this one
   *
   * @return Quaternion New quaternion created from this
   */
  NODISCARD FORCEINLINE Quaternion
  getConjugated() const;

  /**
   * @brief Checks if this quaternion is valid and has no NaNs
   * NOTE: If this quaternion has any NaN value, it'll be converted to IDENTITY
   */
  FORCEINLINE void
  diagnosticCheckNaN() const;

  /**
   * @brief Checks if this quaternion contains NaN
   *
   * @return bool True if this quaternion has an invalid component
   */
  NODISCARD FORCEINLINE bool
  containsNaN() const;

  /**
   * @brief Compares two quaternions with an acceptable threshold
   *
   * @param q Quaternion to check against
   * @param threshold The range of acceptance to compare
   * @return bool True if quaternions are nearly equal
   */
  NODISCARD FORCEINLINE bool
  nearEqual(const Quaternion& q, float threshold = Math::SMALL_NUMBER) const;

  /**
   * @brief Rotate a vector by this quaternion
   *
   * @param v The vector to be rotated
   * @return Vector3 Rotated vector
   */
  NODISCARD Vector3
  rotateVector(const Vector3& v) const;

  /**
   * @brief Rotate a vector by the inverse of this quaternion
   *
   * @param v The vector to be rotated
   * @return Vector3 Vector rotated by the inverse quaternion
   */
  NODISCARD Vector3
  unrotateVector(const Vector3& v) const;

  /**
   * @brief Get the length squared of this quaternion
   *
   * @return float Squared length of the quaternion
   */
  NODISCARD FORCEINLINE float
  squaredLength() const;

  /**
   * @brief Get the length of this quaternion
   *
   * @return float Length of the quaternion
   */
  NODISCARD FORCEINLINE float
  length() const;

  /**
   * @brief Calculates the inverse of this quaternion
   *
   * @return Quaternion The inverse quaternion
   */
  NODISCARD FORCEINLINE Quaternion
  getInverse() const;

  /************************************************************************/
  /*
   * Operators
   */
  /************************************************************************/

  /**
   * @brief Copy operator
   *
   * @param q The quaternion to copy
   * @return Quaternion& Reference to this
   */
  FORCEINLINE Quaternion&
  operator=(const Quaternion& q);

  /**
   * @brief Compares two quaternions
   *
   * @param q The quaternion to check against
   * @return bool True if both quaternions are the same
   */
  NODISCARD FORCEINLINE bool
  operator==(const Quaternion& q) const;

  /**
   * @brief Multiplies this quaternion against a scalar
   *
   * @param scalar The scalar to be multiplied
   * @return Quaternion New quaternion multiplied by the scalar
   */
  NODISCARD FORCEINLINE Quaternion
  operator*(float scalar) const;

  /**
   * @brief Multiplies this quaternion against a scalar and modifies it
   *
   * @param scalar The scalar to be multiplied
   * @return Quaternion& Reference to this
   */
  FORCEINLINE Quaternion&
  operator*=(float scalar);

  /**
   * @brief Multiplies this quaternion against another quaternion
   *
   * @param q The quaternion to be multiplied
   * @return Quaternion New quaternion from multiplication
   */
  NODISCARD FORCEINLINE Quaternion
  operator*(const Quaternion& q) const;

  /**
   * @brief Multiplies this quaternion against another and modifies it
   *
   * @param q The quaternion to be multiplied
   * @return Quaternion& Reference to this
   */
  FORCEINLINE Quaternion&
  operator*=(const Quaternion& q);

  /**
   * @brief Adds two quaternions
   *
   * @param q The quaternion to add
   * @return Quaternion Result of addition
   */
  NODISCARD FORCEINLINE Quaternion
  operator+(const Quaternion& q) const;

  /**
   * @brief Component-wise access
   *
   * @param index Index (0-3 for x,y,z,w)
   * @return float& Reference to the component
   */
  NODISCARD FORCEINLINE float&
  operator[](int32 index);

  /**
   * @brief Component-wise access (const version)
   *
   * @param index Index (0-3 for x,y,z,w)
   * @return const float& Reference to the component
   */
  NODISCARD FORCEINLINE const float&
  operator[](int32 index) const;

 public:
  /**
   * @brief Identity quaternion (no rotation)
   */
  static const Quaternion IDENTITY;

  /**
   * @brief Vector part
   */
  float x;
  float y;
  float z;

  /**
   * @brief Scalar part
   */
  float w;
};

/************************************************************************/
/*
 * Implementations
 */
/************************************************************************/

/*
 */
FORCEINLINE
Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

/*
 */
FORCEINLINE
Quaternion::Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {
  diagnosticCheckNaN();
}

/*
 */
FORCEINLINE
Quaternion::Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w) {
  diagnosticCheckNaN();
}

/*
 */
FORCEINLINE Quaternion&
Quaternion::operator=(const Quaternion& q) {
  x = q.x;
  y = q.y;
  z = q.z;
  w = q.w;
  return *this;
}

/*
 */
FORCEINLINE bool
Quaternion::nearEqual(const Quaternion& q, const float threshold) const {
  return (Math::abs(q.x - x) <= threshold) && (Math::abs(q.y - y) <= threshold) &&
         (Math::abs(q.z - z) <= threshold) && (Math::abs(q.w - w) <= threshold);
}

/*
 */
FORCEINLINE bool
Quaternion::operator==(const Quaternion& q) const {
  return nearEqual(q, 0.0f);
}

/*
 */
FORCEINLINE Quaternion
Quaternion::operator*(float scalar) const {
  return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
}

/*
 */
FORCEINLINE Quaternion&
Quaternion::operator*=(float scalar) {
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}

/*
 */
FORCEINLINE Quaternion
Quaternion::operator*(const Quaternion& q) const {
  return Quaternion(
      w * q.x + x * q.w + y * q.z - z * q.y,
      w * q.y - x * q.z + y * q.w + z * q.x,
      w * q.z + x * q.y - y * q.x + z * q.w,
      w * q.w - x * q.x - y * q.y - z * q.z);
}

/*
 */
FORCEINLINE Quaternion&
Quaternion::operator*=(const Quaternion& q) {
  const float newX = w * q.x + x * q.w + y * q.z - z * q.y;
  const float newY = w * q.y - x * q.z + y * q.w + z * q.x;
  const float newZ = w * q.z + x * q.y - y * q.x + z * q.w;
  const float newW = w * q.w - x * q.x - y * q.y - z * q.z;

  x = newX;
  y = newY;
  z = newZ;
  w = newW;

  return *this;
}

/*
 */
FORCEINLINE Quaternion
Quaternion::operator+(const Quaternion& q) const {
  return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
}

/*
 */
FORCEINLINE float&
Quaternion::operator[](int32 index) {
  CH_ASSERT(index >= 0 && index < 4);
  return (&x)[index];
}

/*
 */
FORCEINLINE const float&
Quaternion::operator[](int32 index) const {
  CH_ASSERT(index >= 0 && index < 4);
  return (&x)[index];
}

/*
 */
FORCEINLINE float
Quaternion::squaredLength() const {
  return x * x + y * y + z * z + w * w;
}

/*
 */
FORCEINLINE float
Quaternion::length() const {
  return Math::sqrt(squaredLength());
}

/*
 */
FORCEINLINE void
Quaternion::normalize() {
  const float len = length();
  if (len > Math::SMALL_NUMBER) {
    const float invLen = 1.0f / len;
    x *= invLen;
    y *= invLen;
    z *= invLen;
    w *= invLen;
  } else {
    // Set to identity if too small
    *this = IDENTITY;
  }
}

/*
 */
FORCEINLINE Quaternion
Quaternion::getNormalized() const {
  Quaternion result = *this;
  result.normalize();
  return result;
}

/*
 */
FORCEINLINE void
Quaternion::conjugate() {
  x = -x;
  y = -y;
  z = -z;
}

/*
 */
FORCEINLINE Quaternion
Quaternion::getConjugated() const {
  return Quaternion(-x, -y, -z, w);
}

/*
 */
FORCEINLINE void
Quaternion::diagnosticCheckNaN() const {
#if USING(CH_DEBUG_MODE)
  if (containsNaN()) {
    *const_cast<Quaternion*>(this) = Quaternion::IDENTITY;
  }
#endif // CH_DEBUG_MODE
}

/*
 */
FORCEINLINE bool
Quaternion::containsNaN() const {
  return (!Math::isFinite(x) || !Math::isFinite(y) || !Math::isFinite(z) ||
          !Math::isFinite(w));
}

/*
 */
FORCEINLINE Quaternion
Quaternion::getInverse() const {
  const float squareSum = squaredLength();
  if (squareSum >= Math::SMALL_NUMBER) {
    const float scale = 1.0f / squareSum;
    return Quaternion(-x * scale, -y * scale, -z * scale, w * scale);
  }

  // Return identity for degenerate quaternions
  return IDENTITY;
}

} // namespace chEngineSDK
