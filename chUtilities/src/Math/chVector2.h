/************************************************************************/
/**
 * @file chVector2.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/16
 * Vector for 2D spaces.
 *
 * A vector for 2D spaces composed by (X,Y) with floating points.
 *
 * @bug No bug known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK {
using std::ostream;

/**
 * Vector for 2D spaces.
 *
 * A vector for 2D spaces composed by (X,Y) with floating points.
 *
 * Sample usage:
 *   Vector2 vector2(0.0, 0.0, 1.0);
 *   float Magnitude = vector2.magnitude();
 *
 */
class Vector2
{
public:
  /*
   * Default constructor
   */
  FORCEINLINE Vector2() = default;

  /**
   * Constructor using initial given values.
   *
   * @param _x
   *   X value
   *
   * @param _y
   *   Y value
   */
  FORCEINLINE Vector2(float _x, float _y);

  /**
   * Constructor using initial given values as an C++ Array.
   *
   * @param vector2Array
   *   C++ array to be converted to this Vector2 type
   */
  FORCEINLINE explicit Vector2(const float vector2Array[2]);

  //FORCEINLINE explicit Vector2(const float* v2Arr);

  /**
   * Copy Constructor.
   */
  FORCEINLINE Vector2(const Vector2& toCopy);

  /**
   *
   **/
  Vector2(const Vector3& vector3);

  /*
   * Default destructor
   */
  ~Vector2() = default;

  /**
   * Computes the cross product from this to another vector.
   *
   * @param vector2
   *   Vector to compute cross product from this
   *
   * @return
   *  Float Scalar Z of the result vector2.
   */
  FORCEINLINE float
  cross(const Vector2& vector2) const;

  /**
   * Computes the dot product from this to another vector.
   *
   * @param vector2
   *   Vector to compute product from this.
   *
   * @return
   *  Float value of dot product.
   */
  FORCEINLINE float
  dot(const Vector2& vector2) const;

  /**
   * Projection of one vector over another.
   *
   * @param vector2
   *  Vector to project this
   *
   * @return
   *  Float Between 0.0f - 1.0f. Projection A over B.
   */
  FORCEINLINE  Vector2
  projection(const Vector2& vector2) const;

  /**
   * Magnitude of this vector.
   *
   * @return
   *  Float the magnitude of this vector.
   */
  FORCEINLINE float
  magnitude() const;

  /**
   * Fast magnitude that jumps the square root part.
   * Note: if you want a precise magnitude then prefer magnitude.
   *
   * @return
   *  Float magnitude before square root.
   */
  FORCEINLINE float
  sqrMagnitude() const;

  /**
   * Normalize this vector.
   */
  FORCEINLINE void
  normalize(float tolerance = Math::SMALL_NUMBER);

  /**
   * Creates a new normalized vector from this.
   *
   * @return
   *  Vector2 new Vector created from normalization
   */
  FORCEINLINE Vector2
  getNormalized(float tolerance = Math::SMALL_NUMBER) const;


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
  operator==(const Vector2& vector2) const;

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
  operator!=(const Vector2& vector2) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *   Scalar to be multiplied.
   *
   * @return
   *    Vector2 new Vector2 created from this operation.
   */
  FORCEINLINE Vector2
  operator*(float scalar) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *   Scalar to be multiplied.
   *
   * @return
   *   THIS reference. Note that this function modifies this Vector.
   */
  FORCEINLINE Vector2&
  operator*=(float scalar);

  /**
   *   Adds two vectors and returns a new one created.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  The new vector created from adding other two vector.
   **/
  FORCEINLINE Vector2
  operator+(const Vector2& vector3) const;

  /**
   *   Adds two vectors modifying this vector.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  This reference modified.
   **/
  FORCEINLINE Vector2&
  operator+=(const Vector2& vector3);

  /**
   *   Subtracts this vector against other and returns a new one.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  The new created vector.
   **/
  FORCEINLINE Vector2
  operator-(const Vector2& vector3) const;

  /**
   *   Subtracts this vector against other modifying this.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  This vector reference modified.
   **/
  FORCEINLINE Vector2&
  operator-=(const Vector2& vector3);

  /**
   *   Modifies sign of this vector.
   *
   * @return
   *  A new vector3 created from this vector.
   **/
  FORCEINLINE Vector2
  operator-() const;

  FORCEINLINE Vector2&
  operator=(const float* v2Arr);

  FORCEINLINE Vector2&
  operator=(const Vector2& vector2);

  /**
   *   Returns the xy as Array
   *
   * @return
   *  A new Array 2 x,y
   **/
  FORCEINLINE Array<float, 2>
  xy() const { return {x, y};}

private:
  /**
   * Logical string representations of this vector.
   *
   * @param _output
   *   The output stream where to write.
   *
   * @param
   *   Vector2 The reference to this vector to be out.
   *
   * @return
   *   Ostream of this class to string.
   */
  friend ostream& operator<<(ostream& output, const Vector2& vector2)
  {
    output << "(" << vector2.x << ", " << vector2.y << ")";
    return output;
  }

public:
  /************************************************************************/
  /*
   * Static Pre-constructed Vectors.
   */
   /************************************************************************/

   /**
    * Global 2D zero vector constant (0.0f, 0.0f).
    */
  static CH_UTILITY_EXPORT const Vector2 ZERO;

  /**
   * Global 2D Unit vector constant (1.0f, 1.0f).
   */
  static CH_UTILITY_EXPORT const Vector2 UNIT;

  /**
   * Global 2D Unit x vector constant (1.0f, 0.0f).
   */
  static CH_UTILITY_EXPORT const Vector2 UNIT_X;

  /**
   * Global 2D Unit y vector constant (0.0f, 1.0f).
   */
  static CH_UTILITY_EXPORT const Vector2 UNIT_Y;

public:

  float x;
  float y;
};

/************************************************************************/
/*
 * Implementations.
 */
 /************************************************************************/
 /*
 */
FORCEINLINE
Vector2::Vector2(float _x, float _y) : x(_x), y(_y)
{}

/*
*/
FORCEINLINE
Vector2::Vector2(const float vector2Array[2]) : x(vector2Array[0]), y(vector2Array[1])
{}

/*
*/
FORCEINLINE
Vector2::Vector2(const Vector2& copyVector) : x(copyVector.x), y(copyVector.y)
{}

/*
*/
FORCEINLINE float
Vector2::cross(const Vector2& vector2) const
{
  return x * vector2.y - y * vector2.x;
}

/*
*/
FORCEINLINE float
Vector2::dot(const Vector2& vector2) const
{
  return x * vector2.x + y * vector2.y;
}

/*
*/
FORCEINLINE Vector2
Vector2::projection(const Vector2& vector2) const
{
  const float dotP = dot(vector2);
  return vector2 * (dotP / vector2.dot(vector2));
}

/*
*/
FORCEINLINE float
Vector2::magnitude() const
{
  return Math::sqrt(sqrMagnitude());
}

/*
*/
FORCEINLINE float
Vector2::sqrMagnitude() const
{
  return x * x + y * y;
}

/*
*/
FORCEINLINE void
Vector2::normalize(float tolerance)
{
  const float SquareSum = x * x + y * y;
  if (SquareSum > tolerance) {
    const float Scale = Math::invSqrt(SquareSum);
    x *= Scale;
    y *= Scale;
    return;
  }
  x = 0.0f;
  y = 0.0f;
}

/*
*/
FORCEINLINE Vector2
Vector2::getNormalized(float tolerance) const
{
  Vector2 Res = *this;
  Res.normalize(tolerance);
  return Res;
}

/*
*/
FORCEINLINE bool
Vector2::operator==(const Vector2& vector2) const
{
  return x == vector2.x && y == vector2.y;
}

/*
*/
FORCEINLINE bool
Vector2::operator!=(const Vector2& vector2) const
{
  return x != vector2.x || y != vector2.y;
}


/*
*/
FORCEINLINE Vector2
Vector2::operator*(float scalar) const
{
  return {x * scalar, y * scalar};
}

/*
*/
FORCEINLINE Vector2&
Vector2::operator*=(float scalar)
{
  x *= scalar;
  y *= scalar;
  return *this;
}

/*
*/
FORCEINLINE Vector2
Vector2::operator+(const Vector2& vector3) const
{
  return { x + vector3.x, y + vector3.y };
}

/*
*/
FORCEINLINE Vector2&
Vector2::operator+=(const Vector2& vector3)
{
  x += vector3.x;
  y += vector3.y;
  return *this;
}

/*
*/
FORCEINLINE Vector2
Vector2::operator-(const Vector2& vector3) const
{
  return {x - vector3.x, y - vector3.y};
}

/*
*/
FORCEINLINE Vector2&
Vector2::operator-=(const Vector2& vector3)
{
  x -= vector3.x;
  y -= vector3.y;
  return *this;
}

/*
*/
FORCEINLINE Vector2
Vector2::operator-() const
{
  return {-x, -y};
}

/*
*/
FORCEINLINE Vector2&
Vector2::operator=(const float* v2Arr) {
  x = v2Arr[0];
  y = v2Arr[1];
  return *this;
}

/*
*/
FORCEINLINE Vector2&
Vector2::operator=(const Vector2& vector2) {
  x = vector2.x;
  y = vector2.y;
  return *this;
}


}
