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

/************************************************************************/
/*
 * Includes.
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Quaternion math component to handle right rotations.
 *
 * Sample usage:
 *    Quaternion q(MAth::PI, MAth::PI, MAth::PI);
 *
 */
class CH_UTILITY_EXPORT Quaternion
{
 public:
 /*
  * @brief Default constructor
  */
  Quaternion() = default;

  /** 
   *   Copy constructor.
   **/
  Quaternion(const Quaternion& _q);

  /** 
   *   Constructor 4 components.
   **/
  FORCEINLINE Quaternion(float _x, float _y, float _z, float _w);

  /** 
   *   Constructor from a Rotator.
   **/
  explicit  Quaternion(const Rotator& rotator);

  /** 
   *   Matrix Constructor.
   **/
  explicit Quaternion(const Matrix4& m4);

  explicit Quaternion(const Vector4& v4);

  /** 
   *   Constructor from a Axis and an angle
   * 
   * @param axis
   *  The axis where is going to be rotated 
   * 
   * @param angle
   *  The angle to rotate this quaternion in degrees.
   **/
  Quaternion(const Vector3& axis, const Degree& angle);

 /*
  * @brief Default destructor
  */
  ~Quaternion() = default;

  /************************************************************************/
  /*
   * Methods.
   */
   /************************************************************************/

   /**
    *   Normalizes this quaternion.
    **/
  FORCEINLINE void
  normalize();

  /**
   *   Creates a new Quaternion normalized from this.
   *
   * @return Quaternion
   *  The new quaternion created.
   **/
  FORCEINLINE Quaternion
  getNormalized() const;

  /**
   *   Conjugates this quaternion.
   **/
  FORCEINLINE void
  conjugate();

  /**
   *   Creates a Rotator from this quaternion.
   *
   * @return Rotator
   *  New rotator created from this.
   **/
  Rotator
  toRotator() const;

  /**
   *   Creates a new Quaternion conjugated from this.
   *
   * @return Quaternion
   *   New quaternion created from this.
   **/
  FORCEINLINE Quaternion
  getConjugated() const;

  /**
   *   Checks this quaternion to see if is valid and has no NaB inner.
   *  NOTE: If this Quaternion has any NaN value it'll be converted to a IDENTITY.
   **/
  FORCEINLINE void
  diagnosticCheckNaN() const;

  /**
   *   Computes if  this quaternion contains NaN.
   *
   * @return bool
   *  True if this quaternion has an invalid component.
   **/
  FORCEINLINE bool
  containsNaN() const;

  /** 
   *   Compares two quaternions with a acceptable threshold.
   * 
   * @param q
   *  Quaternion that is going to be checked against.
   * 
   * @param threshold
   *   The range of acceptance to compare.
   * 
   * @return
   **/
  FORCEINLINE bool
  nearEqual(const Quaternion& q, const float threshold = Math::SMALL_NUMBER) const;
  
  /**
   *    Rotate a vector by this quaternion.
   * 
   * @param V 
   *   The vector to be rotated
   * 
   * @return 
   *  vector after rotation
   */
  Vector3
  rotateVector(const Vector3& v) const;

  /**
   *   Rotate a vector by the inverse of this quaternion.
   * 
   * @param V 
   *   The vector to be rotated
   * 
   * @return 
   *   vector after rotation by the inverse of this quaternion.
   */
  Vector3
  unrotateVector(const Vector3& v) const;

  /************************************************************************/
  /*
   * Operators.
   */                                                                     
  /************************************************************************/

  /** 
   *   Copy operator.
   * @param _q
   *    The copy quaternion.
   * 
   * @return Quaternion
   *  The new quaternion.
   **/
  FORCEINLINE Quaternion&
  operator=(const Quaternion& _q);

  /** 
   *   Compares two Quaternions. 
   * 
   * @param _q
   *  The quaternion to check against.
   * 
   * @return bool
   *  True if both Quaternions are the same.
   **/
  FORCEINLINE bool
  operator==(const Quaternion& _q) const;

  /** 
   *   Multiplies this quaternion against a scalar.
   * 
   * @param scalar
   *    The scalar to be multiplied.
   * 
   * @return Quaternion
   *    The new quaternion multiplied by the scalar
   **/
  FORCEINLINE Quaternion
  operator*(const float& scalar) const;

  /**
   *   Multiplies this quaternion against a scalar and keeps this quaternion.
   *
   * @param scalar
   *    The scalar to be multiplied.
   **/
  FORCEINLINE Quaternion&
  operator*=(const float& scalar);

  /**
   *   Multiplies this quaternion against another quaternion.
   *
   * @param q
   *    The quaternion to be multiplied.
   *
   * @return Quaternion
   *    The new quaternion multiplied by the scalar
   **/
  FORCEINLINE Quaternion
  operator*(const Quaternion& q) const;

  /**
   *   Multiplies this quaternion against another quaternion and keeps this quaternion.
   *
   * @param q
   *    The quaternion to be multiplied.
   **/
  FORCEINLINE Quaternion&
  operator*=(const Quaternion & q);


 public:
  static const Quaternion IDENTITY;

  union
  {
    struct
    {
      float x;
      float y;
      float z;
      float w;
    };
  };
};

/************************************************************************/
/*
 * Implementations.
 */                                                                     
/************************************************************************/

/*
*/
FORCEINLINE
Quaternion::Quaternion(float _x, float _y, float _z, float _w)
  : x(_x), y(_y), z(_z), w(_w) 
{
  diagnosticCheckNaN();
}

/*
*/
FORCEINLINE
Quaternion::Quaternion(const Quaternion& _q)
  : x(_q.x), y(_q.y), z(_q.z), w(_q.w)
{
  diagnosticCheckNaN();
}

/*
*/
FORCEINLINE Quaternion&
Quaternion::operator=(const Quaternion& _q)
{
  x = _q.x;
  y = _q.y;
  z = _q.z;
  w = _q.w;
  return *this;
}

/*
*/
FORCEINLINE bool
Quaternion::nearEqual(const Quaternion& q, const float threshold) const
{
  return (Math::abs(q.x - x) <= threshold) && 
         (Math::abs(q.y - y) <= threshold) &&
         (Math::abs(q.z - z) <= threshold);
}

/*
*/
FORCEINLINE bool
Quaternion::operator==(const Quaternion& _q) const
{
  return nearEqual(_q, 0.0f);
}

/*
*/
FORCEINLINE Quaternion
Quaternion::operator*(const float& scalar) const
{
  Quaternion Q = *this;
  Q *= scalar;
  return Q;
}

/*
*/
FORCEINLINE Quaternion&
Quaternion::operator*=(const float& scalar)
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}

/*
*/
FORCEINLINE Quaternion
Quaternion::operator*(const Quaternion& q) const
{
  Quaternion Q = *this;
  Q *= q;
  return Q;
}

/*
*/
FORCEINLINE Quaternion&
Quaternion::operator*=(const Quaternion& q)
{
  x = w * q.x + x * q.w + y * q.z - z * q.y;
  y = w * q.y - x * q.z + y * q.w + z * q.x;
  z = w * q.z + x * q.y - y * q.x + z * q.w;
  w = w * q.w - x * q.x - y * q.y - z * q.z;

  return *this;
}

/*
*/
FORCEINLINE void
Quaternion::normalize()
{
  const float n = 1.f / Math::sqrt(x * x + y * y + z * z + w * w);
  x *= n;
  y *= n;
  z *= n;
  w *= n;
}

/*
*/
FORCEINLINE Quaternion
Quaternion::getNormalized() const
{
  Quaternion Q = *this;
  Q.normalize();
  return Q;
}

/*
*/
FORCEINLINE void
Quaternion::conjugate()
{
  x *= -1;
  y *= -1;
  z *= -1;
}

/*
*/
FORCEINLINE Quaternion
Quaternion::getConjugated() const
{
  Quaternion Q = *this;
  Q.conjugate();
  return Q;
}

/*
*/
FORCEINLINE void
Quaternion::diagnosticCheckNaN() const
{
#if USING(CH_DEBUG_MODE)
  if (containsNaN()) {
    *const_cast<Quaternion*>(this) = Quaternion::IDENTITY;
  }
#endif // CH_DEBUG_MODE
}

/*
*/
FORCEINLINE bool
Quaternion::containsNaN() const
{
  return (!Math::isFinite(x) ||
          !Math::isFinite(y) ||
          !Math::isFinite(z) ||
          !Math::isFinite(w));
}

}