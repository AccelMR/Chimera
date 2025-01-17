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
class Vector2I {
 public:
 /*
  *   Default constructor
  */
  Vector2I() = default;
  FORCEINLINE Vector2I(int32 _x, int32 _y);

 /*
  *   Default destructor
  */
  ~Vector2I() = default;

  /************************************************************************/
  /*
   * Operator overloads.
   */
   /************************************************************************/

   /**
    * Compares this vector against a given one.
    *
    * @param vector2
    *  Vector to compare against this.
    *
    * @return
    *  True if both vectors are equal.
    */
  FORCEINLINE bool
  operator==(const Vector2I& vector2) const;

  /**
   * Compares this vector against a given one.
   *
   * @param vector2
   *   Vector to compare against this.
   *
   * @return
   *   true if both vectors are equal.
   */
  FORCEINLINE bool
  operator!=(const Vector2I& vector2) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *   Scalar to be multiplied.
   *
   * @return
   *    Vector2I new Vector2I created from this operation.
   */
  FORCEINLINE Vector2I
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
  FORCEINLINE Vector2I&
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
  FORCEINLINE Vector2I
  operator+(const Vector2I& vector2) const;

  /**
   *   Adds two vectors modifying this vector.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  This reference modified.
   **/
  FORCEINLINE Vector2I&
  operator+=(const Vector2I& vector2);

  /**
   *   Subtracts this vector against other and returns a new one.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  The new created vector.
   **/
  FORCEINLINE Vector2I
  operator-(const Vector2I& vector2) const;

  /**
   *   Subtracts this vector against other modifying this.
   *
   * @param vector2
   *  The other vector to be subtracted.
   *
   * @return vector2
   *  This vector reference modified.
   **/
  FORCEINLINE Vector2I&   
  operator-=(const Vector2I& vector2);

  /**
   *   Modifies sign of this vector.
   *
   * @return
   *  A new vector3 created from this vector.
   **/
  FORCEINLINE Vector2I
  operator-() const;


 public:
  int32 x;
  int32 y;

 public:
  CH_UTILITY_EXPORT static const Vector2I ZERO;
  CH_UTILITY_EXPORT static const Vector2I UNIT;
  CH_UTILITY_EXPORT static const Vector2I UNIT_X;
  CH_UTILITY_EXPORT static const Vector2I UNIT_Y;
};

/*
*/
FORCEINLINE
Vector2I::Vector2I(int32 _x, int32 _y)
  : x(_x), y(_y)
{}

/*
*/
FORCEINLINE Vector2I
Vector2I::operator*(const int32 & scalar) const {
  return { x * scalar, y * scalar };
}

/*
*/
FORCEINLINE Vector2I &
Vector2I::operator*=(const int32& scalar) {
  x *= scalar;
  y *= scalar;
  return *this;

}

/*
*/
FORCEINLINE Vector2I
Vector2I::operator+(const Vector2I& vector2) const {
  return { x + vector2.x, y + vector2.y };

}

/*
*/
FORCEINLINE Vector2I &
Vector2I::operator+=(const Vector2I& vector2) {
  x += vector2.x;
  y += vector2.y;
  return *this;
}

/*
*/
FORCEINLINE Vector2I
Vector2I::operator-(const Vector2I& vector2) const {
  return { x - vector2.x, y - vector2.y };

}

/*
*/
FORCEINLINE Vector2I &
Vector2I::operator-=(const Vector2I& vector2) {
  x -= vector2.x;
  y -= vector2.y;
  return *this;
}

/*
*/
FORCEINLINE Vector2I
Vector2I::operator-() const {
  return { -x, -y };
}

/*
*/
FORCEINLINE bool
Vector2I::operator!=(const Vector2I& vector2) const {
  return x != vector2.x && y != vector2.y;
}

/*
*/
FORCEINLINE bool
Vector2I::operator==(const Vector2I& vector2) const {
  return x == vector2.x && y == vector2.y;

}

}

