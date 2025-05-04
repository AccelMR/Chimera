/************************************************************************/
/**
 * @file chVector4.h
 * @author AccelMR
 * @date 2022/02/15
 *   Vector 4D spaced composed by ( X, Y, Z, W ) with floating points.
 *
 * Coordinate system being X = right, Z = up, Y = front
 */
 /************************************************************************/
#pragma once


#ifndef  CH_VECTOR_4
#define CH_VECTOR_4
#endif // CH_VECTOR_4


/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK {
using std::ostream;

/*
 * Description:
 *     A Vector 4D spaced composed by ( X, Y, Z, W ) with floating points.
 *
 * Sample usage:
 *    Vector4 v4();
 *    v4.x = 10.0f;
 *    v4.normalize();
 */
class CH_UTILITY_EXPORT Vector4
{
public:
  /*
   * @brief Default constructor
   */
  FORCEINLINE Vector4() = default;

  /**
   *   Constructor from 4 floats.
   * NOTE: W can be ignored since it initializes as 1.0f
   **/
  FORCEINLINE Vector4(const float& _x, const float& _y, const float& _z, const float& _w = 1.0f);

  /**
   *   Constructor from an 4 Array.
   **/
  explicit FORCEINLINE Vector4(const float arrayV4[4]);


  /**
   *   Constructor from an 4 Array.
   **/
  explicit FORCEINLINE Vector4(const Array<float, 4> arrayV4);

  /**
   *   Constructor from Vector4
   * NOTE: W can be ignored since it initializes as 1.0f
   **/
  explicit Vector4(const Vector3& vector3, const float& _w = 1.0f);

  /**
   *   Copy constructor.
   **/
  FORCEINLINE Vector4(const Vector4& copyVector4);
  /*
   * @brief Default destructor
   */
  ~Vector4() = default;

  /************************************************************************/
  /*
   * Methods
   */
   /************************************************************************/

   /**
    *   Computes dot product from this vector to another.
    *
    * @param vector4
    *  The other vector to calculate dot product.
    *
    * @return float
    *  The dot product from this vector against param.
    **/
  FORCEINLINE float
  dot(const Vector4& vector4) const;

  /**
   *   Computes cross product.
   *
   * @param vector4
   *  The vector needed to calculate cross product.
   *
   * @return
   **/
  FORCEINLINE Vector4
  cross(const Vector4& vector4) const;

  /**
   *   Computes a new Vector that all its values are in absolute.
   *
   * @return Vector4
   *  The new created vector.
   **/
  FORCEINLINE Vector4
  getAbs() const;

  /**
   *   Computes the magnitude of this vector.
   *
   * @return float
   *  Magnitude computed from this vector.
   **/
  FORCEINLINE float
  magnitude() const;

  /**
   *   Computes the square magnitude of this vector.
   *
   * @return float
   *  Square magnitude computed from this vector.
   **/
  FORCEINLINE float
  sqrMagnitude() const;

  /**
   *   Changes this vector
   * @param
   *
   * @return
   **/
  FORCEINLINE void
  normalize(float tolerance = Math::SMALL_NUMBER);

  /**
   *   Creates a Vector4 normalized from this caller.
   *
   * @return Vector4
   *  The new normalized vector.
   **/
  FORCEINLINE Vector4
  getNormalized(float tolerance = Math::SMALL_NUMBER) const;

  NODISCARD FORCEINLINE bool
  nearEqual(const Vector4& v, const float& tolerance = Math::SMALL_NUMBER) const;

  /************************************************************************/
  /*
   * Operator overloads.
   */
   /************************************************************************/

   /**
    * Compares this vector against a given one.
    *
    * @param vector4
    *  Vector to compare against this.
    *
    * @return
    *  true if both vectors are equal.
    */
  FORCEINLINE bool
  operator==(const Vector4& vector4) const;

  /**
   * Compares this vector against a given one.
   *
   * @param vector4
   *  Vector to compare against this.
   *
   * @return
   *  true if both vectors are equal.
   */
  FORCEINLINE bool
  operator==(const Array<float, 4>& v4) const;

  /**
   * Compares this vector against a given one.
   *
   * @param vector4
   *  Vector to compare against this.
   *
   * @return
   *  true if both vectors are equal.
   */
  FORCEINLINE bool
  operator!=(const Vector4& vector4) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *  Scalar to be multiplied.
   *
   * @return
   *  Vector2 new Vector2 created from this operation.
   */
  FORCEINLINE Vector4
  operator*(const float& scalar) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *  Scalar to be multiplied.
   *
   * @return
   *  Vector2 new Vector2 created from this operation.
   */
  FORCEINLINE Vector4&
  operator*=(const float& scalar);

  /**
   *   Adds two vectors and returns a new one created.
   *
   * @param vector4
   *  The other vector to be added.
   *
   * @return Vecto3
   *  The new vector created from adding other two vector.
   **/
  FORCEINLINE Vector4
  operator+(const Vector4& vector4) const;

  /**
   *   Adds two vectors modifying this vector.
   *
   * @param vector4
   *  The other vector to be added.
   *
   * @return Vecto3
   *  This reference modified.
   **/
  FORCEINLINE Vector4&
  operator+=(const Vector4& vector4);

  /**
   *   Subtracts this vector against other and returns a new one.
   *
   * @param vector4
   *  The other vector to be subtracted.
   *
   * @return Vector4
   *  The new created vector.
   **/
  FORCEINLINE Vector4
  operator-(const Vector4& vector4) const;

  /**
   *   Subtracts this vector against other modifying this.
   *
   * @param vector4
   *  The other vector to be subtracted.
   *
   * @return Vector4
   *  This vector reference modified.
   **/
  FORCEINLINE Vector4&
  operator-=(const Vector4& vector4);

  /**
   *   Modifies sign of this vector.
   *
   * @return
   *  A new vector4 created from this vector.
   **/
  FORCEINLINE Vector4
  operator-() const;

  /**
   *   Copy operator. Assign operator
   *
   * @param vector3
   *  The other vector to be set.
   **/
  FORCEINLINE Vector4&
  operator=(const Vector4& vector4);


  /**
   *   Returns the xy as Array
   *
   * @return
   *  A new Array 2 x,y
   **/
  FORCEINLINE Array<float, 2>
  xy() const { return {x, y};}


  /**
   *   Returns the xyz as Array
   *
   * @return
   *  A new Array 3 x,y,z
   **/
  FORCEINLINE Array<float, 3>
  xyz() const { return {x, y, z};}

  /**
   *   Returns the xyzw as Array
   *
   * @return
   *  A new Array 4 x,y,z,w
   **/
  FORCEINLINE Array<float, 4>
  xyzw() const { return {x, y, z, w};}

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
  friend ostream& operator<<(ostream& output, const Vector4& vector4)
  {
    output << "(" << vector4.x << ", " << vector4.y << ", " << vector4.z << ", " << vector4.w << ")";
    return output;
  }

 public:

    /************************************************************************/
    /*
     * Static Pre-constructed Vectors.
     */
     /************************************************************************/

    /**
     * Global 4D zero vector constant (0.0f, 0.0f, 0.0f).
     */
    static const Vector4 ZERO;

    /**
     * Global 4D Unit vector constant (1.0f, 1.0f, 1.0f).
     */
    static const Vector4 UNIT;

public:

  float x;
  float y;
  float z;
  float w;

};
/************************************************************************/
/*
 * Implementations.
 */
 /************************************************************************/

 /*
 */
FORCEINLINE
Vector4::Vector4(const float& _x,
                 const float& _y,
                 const float& _z,
                 const float& _w)
  : x(_x),
    y(_y),
    z(_z),
    w(_w)
{}

/*
*/
FORCEINLINE
Vector4::Vector4(const float arayV4[4])
  : x(arayV4[0]),
    y(arayV4[1]),
    z(arayV4[2]),
    w(arayV4[3])
{}

/*
*/
FORCEINLINE
Vector4::Vector4(const Array<float, 4> arayV4)
  : x(arayV4[0]),
    y(arayV4[1]),
    z(arayV4[2]),
    w(arayV4[3])
{}

/*
*/
FORCEINLINE
Vector4::Vector4(const Vector4& copyVector)
  : x(copyVector.x),
    y(copyVector.y),
    z(copyVector.z),
    w(copyVector.w)
{}

/*
*/
FORCEINLINE float
Vector4::dot(const Vector4& vector4) const
{
  return x * vector4.x + y * vector4.y + z * vector4.z + w * vector4.w;
}

/*
*/
FORCEINLINE Vector4
Vector4::cross(const Vector4& vector4) const
{
  return {y * vector4.z - z * vector4.y,
          z * vector4.x - x * vector4.z,
          x * vector4.y - y * vector4.x,
          0.0f};
}

/*
*/
FORCEINLINE Vector4
Vector4::getAbs() const
{
  return {Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w)};
}

/*
*/
FORCEINLINE float
Vector4::magnitude() const
{
  return Math::sqrt(sqrMagnitude());
}

/*
*/
FORCEINLINE float
Vector4::sqrMagnitude() const
{
  return Math::pow(x, 2.0f) + Math::pow(y, 2.0f) + Math::pow(z, 2.0f) + Math::pow(w, 2.0f);
}

/*
*/
FORCEINLINE void
Vector4::normalize(float tolerance /*= Math::SMALL_NUMBER*/)
{
  const float SquareSum = x * x + y * y + z * z;
  if (SquareSum > tolerance) {
    const float Scale = Math::invSqrt(SquareSum);
    x *= Scale;
    y *= Scale;
    z *= Scale;
    w = 0.0f;
    return;
  }
  *this = Vector4::ZERO;
}

/*
*/
FORCEINLINE bool
Vector4::nearEqual(const Vector4& v, const float& tolerance /*= Math::SMALL_NUMBER*/) const
{
  return (Math::abs(v.x - x) <= tolerance) &&
         (Math::abs(v.y - y) <= tolerance) &&
         (Math::abs(v.z - z) <= tolerance) &&
         (Math::abs(v.w - w) <= tolerance);
}

/*
*/
FORCEINLINE Vector4
Vector4::getNormalized(float tolerance /*= Math::SMALL_NUMBER*/) const
{
  Vector4 Res = *this;
  Res.normalize(tolerance);
  return Res;
}

/*
*/
FORCEINLINE Vector4
Vector4::operator*(const float& scalar) const
{
  return {x * scalar, y * scalar, z * scalar, w *scalar};
}

/*
*/
FORCEINLINE Vector4&
Vector4::operator*=(const float& scalar)
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}

/*
*/
FORCEINLINE Vector4
Vector4::operator+(const Vector4& vector4) const
{
  return {x + vector4.x, y + vector4.y, z + vector4.z, w + vector4.w};
}

/*
*/
FORCEINLINE Vector4&
Vector4::operator+=(const Vector4& vector4)
{
  x += vector4.x;
  y += vector4.y;
  z += vector4.z;
  w += vector4.w;
  return *this;
}

/*
*/
FORCEINLINE Vector4
Vector4::operator-(const Vector4& vector4) const
{
  return {x - vector4.x, y - vector4.y, z - vector4.z, w - vector4.w};
}

/*
*/
FORCEINLINE Vector4&
Vector4::operator-=(const Vector4& vector4)
{
  x -= vector4.x;
  y -= vector4.y;
  z -= vector4.z;
  w -= vector4.w;
  return *this;
}

/*
*/
FORCEINLINE Vector4
Vector4::operator-() const
{
  return {-x, -y, -z, -w};
}

/*
*/
FORCEINLINE bool
Vector4::operator!=(const Vector4& vector4) const
{
  return x != vector4.x || y != vector4.y || z != vector4.z || w != vector4.w;
}

/*
*/
FORCEINLINE bool
Vector4::operator==(const Vector4& vector4) const
{
  return x == vector4.x && y == vector4.y && z == vector4.z && w == vector4.w;
}

/*
*/
FORCEINLINE bool
Vector4::operator==(const Array<float, 4>& v4) const
{
  return x == v4[0] && y == v4[1] && z == v4[2] && w == v4[3];
}

/*
*/
FORCEINLINE Vector4&
Vector4::operator=(const Vector4& vector4)
{
  x = vector4.x;
  y = vector4.y;
  z = vector4.z;
  w = vector4.w;
  return *this;
}
}
