/************************************************************************/
/**
 * @file chVector2I.h
 * @author AccelMR
 * @date 2022/09/19
 *   Math Vector2 that works only with integers.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK{
class Vector3I {
 public:
 /*
  *   Default constructor
  */
  Vector3I() = default;
  FORCEINLINE Vector3I(int32 _x, int32 _y, int32 _z);

 /*
  *   Default destructor
  */
  ~Vector3I() = default;

  /************************************************************************/
  /*
   * Operator overloads.
   */
   /************************************************************************/

   /**
    * Compares this vector against a given one.
    *
    * @param vector3
    *  Vector to compare against this.
    *
    * @return
    *  True if both vectors are equal.
    */
  FORCEINLINE bool
  operator==(const Vector3I& vector3) const;

  /**
   * Compares this vector against a given one.
   *
   * @param vector3
   *   Vector to compare against this.
   *
   * @return
   *   true if both vectors are equal.
   */
  FORCEINLINE bool
  operator!=(const Vector3I& vector3) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *   Scalar to be multiplied.
   *
   * @return
   *    Vector3I new Vector3I created from this operation.
   */
  FORCEINLINE Vector3I
  operator*(const int32& scalar) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *   Scalar to be multiplied.
   *
   * @return
   *   THIS reference. Note that this function modifies this Vector.
   */
  FORCEINLINE Vector3I&
  operator*=(const int32& scalar);

  /**
   *   Adds two vectors and returns a new one created.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  The new vector created from adding other two vector.
   **/
  FORCEINLINE Vector3I
  operator+(const Vector3I& vector3) const;

  /**
   *   Adds two vectors modifying this vector.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  This reference modified.
   **/
  FORCEINLINE Vector3I&
  operator+=(const Vector3I& vector3);

  /**
   *   Subtracts this vector against other and returns a new one.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  The new created vector.
   **/
  FORCEINLINE Vector3I
  operator-(const Vector3I& vector3) const;

  /**
   *   Subtracts this vector against other modifying this.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return vector3
   *  This vector reference modified.
   **/
  FORCEINLINE Vector3I&   
  operator-=(const Vector3I& vector3);

  /**
   *   Modifies sign of this vector.
   *
   * @return
   *  A new vector3 created from this vector.
   **/
  FORCEINLINE Vector3I
  operator-() const;


 public:
  int32 x;
  int32 y;
  int32 z;

 public:
  CH_UTILITY_EXPORT static const Vector3I ZERO;
  CH_UTILITY_EXPORT static const Vector3I UNIT;
  CH_UTILITY_EXPORT static const Vector3I UNIT_X;
  CH_UTILITY_EXPORT static const Vector3I UNIT_Y;
  CH_UTILITY_EXPORT static const Vector3I UNIT_Z;
};

/*
*/
FORCEINLINE
Vector3I::Vector3I(int32 _x, int32 _y, int32 _z)
  : x(_x), y(_y), z(_z)
{}

/*
*/
FORCEINLINE Vector3I
Vector3I::operator*(const int32 & scalar) const {
  return { x * scalar, y * scalar, z * scalar };
}

/*
*/
FORCEINLINE Vector3I &
Vector3I::operator*=(const int32& scalar) {
  x *= scalar;
  y *= scalar;
  z *= scalar;
  return *this;

}

/*
*/
FORCEINLINE Vector3I
Vector3I::operator+(const Vector3I& vector3) const {
  return { x + vector3.x, y + vector3.y, z + vector3.z };

}

/*
*/
FORCEINLINE Vector3I &
Vector3I::operator+=(const Vector3I& vector3) {
  x += vector3.x;
  y += vector3.y;
  z += vector3.z;
  return *this;
}

/*
*/
FORCEINLINE Vector3I
Vector3I::operator-(const Vector3I& vector3) const {
  return { x - vector3.x, y - vector3.y, z - vector3.z };

}

/*
*/
FORCEINLINE Vector3I &
Vector3I::operator-=(const Vector3I& vector3) {
  x -= vector3.x;
  y -= vector3.y;
  z -= vector3.z;
  return *this;
}

/*
*/
FORCEINLINE Vector3I
Vector3I::operator-() const {
  return { -x, -y, -z };
}

/*
*/
FORCEINLINE bool
Vector3I::operator!=(const Vector3I& vector3) const {
  return x != vector3.x && y != vector3.y && z != vector3.z;
}

/*
*/
FORCEINLINE bool
Vector3I::operator==(const Vector3I& vector3) const {
  return x == vector3.x && y == vector3.y && z == vector3.z;

}

}

