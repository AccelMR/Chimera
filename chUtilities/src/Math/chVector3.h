/************************************************************************/
/**
 * @file chVector3.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/10/31
 * Vector for 3D spaces.
 *
 * A vector for 3D spaces composed by (X, Y, Z) with floating points
 *
 * Coordinate system being X = right, Z = up, Y = front
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
 * A vector for 3D spaces composed by (X, Y, Z) with floating points.
 *
 *
 */
class CH_UTILITY_EXPORT Vector3
{
public:
  /*
   * Default constructor
   */
  FORCEINLINE Vector3() = default;

  /**
   * Constructor using initial given values.
   */
  FORCEINLINE Vector3(float _x, float _y, float _z);

  /**
   * Constructor from Vector2.
   *
   * By default z value equals  0.0f
   */
  explicit Vector3(const Vector2& vector2, float _z = 0.0f);

  /**
   *   Constructor from Array 3.
   **/
  explicit FORCEINLINE Vector3(const float f3[3]);

  /** 
   *   Copy constructor.
   **/
  FORCEINLINE Vector3(const Vector3& copyVector3);

  /**
   *    Constructor using the XYZ components from a 4D vector.
   * 
   * @param v 
   *    4D Vector to copy from.
   */
  explicit Vector3(const Vector4& v);

  /**
   *   Constructs a Vector 3 From Vector4.
   *  NOTE: This ignores w value.
   *
   * @param Vector4
   *  The Vector4 where this vector wil;l take values.
   **/
   //FORCEINLINE Vector3(const Vector4& vector4);

    /*
     * Default destructor
     */
  ~Vector3() = default;

  /************************************************************************/
  /*
   * Methods
   */
   /************************************************************************/

   /**
    *   Computes dot product from this vector to another.
    *
    * @param vector3
    *  The other vector to calculate dot product.
    *
    * @return float
    *  The dot product from this vector against param.
    **/
  FORCEINLINE float
  dot(const Vector3& vector3) const;

  /**
   *   Computes cross product.
   *
   * @param vector3
   *  The vector needed to calculate cross product.
   *
   * @return
   **/
  FORCEINLINE Vector3
  cross(const Vector3& vector3) const;

  /**
   *   Computes a new Vector that all its values are in absolute.
   *
   * @return Vector3
   *  The new created vector.
   **/
  FORCEINLINE Vector3
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
   *   Creates a Vector3 normalized from this caller.
   *
   * @return Vector3
   *  The new normalized vector.
   **/
  FORCEINLINE Vector3
  getNormalized(float tolerance = Math::SMALL_NUMBER) const;

  /** 
   *   Computes projection of a vector based on z.
   * 
   * @return Vector3
   *  Projected version of vector based on Z.
   **/
  FORCEINLINE Vector3
  projection(const Vector3& vector3) const;

  /** 
   *   Compares this Vector against another taking in count a tolerance.
   * 
   * @param v
   *    Vector to compare against.
   * 
   * @param tolerance
   *    Accepted tolerance to say both vectors are almost equal.
   * 
   * @return 
   *   True if the two vector are almost equal.
   **/
  FORCEINLINE bool
  nearEqual(const Vector3& v, const float& tolerance = Math::SMALL_NUMBER) const;

  /** 
   *   Computes the squared distance between this vector and a given one.
   *      For more exact distance use distance().
   * 
   * @param v
   *   The vector to compute distance.
   * 
   * @return
   *   Squared distance between two vectors.
   **/
  FORCEINLINE float
  sqrDistance(const Vector3& v) const;

  /** 
   *   Computes the distance between two points.
   * 
   * @param v
   *    The point to check the distance.
   * 
   * @return
   *  Distance between two points.
   **/
  FORCEINLINE float
  distance(const Vector3& v) const;

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
  *  true if both vectors are equal.
  */
  FORCEINLINE bool
  operator==(const Vector3& vector3) const;

  /**
   * Compares this vector against a given one.
   *
   * @param vector3
   *  Vector to compare against this.
   *
   * @return
   *  true if both vectors are equal.
   */
  FORCEINLINE bool
  operator!=(const Vector3& vector3) const;

  /**
   * Calculates the multiplication of this vector against a scalar.
   *
   * @param scalar
   *  Scalar to be multiplied.
   *
   * @return
   *  Vector2 new Vector2 created from this operation.
   */
  FORCEINLINE Vector3
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
  FORCEINLINE Vector3&
  operator*=(const float& scalar);

  /*
  */
  FORCEINLINE Vector3
  operator/(float scale) const;

  /**
   *   Adds two vectors and returns a new one created.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  The new vector created from adding other two vector.
   **/
  FORCEINLINE Vector3
  operator+(const Vector3& vector3) const;

  /**
   *   Adds two vectors modifying this vector.
   *
   * @param vector3
   *  The other vector to be added.
   *
   * @return Vecto3
   *  This reference modified.
   **/
  FORCEINLINE Vector3&
  operator+=(const Vector3& vector3);

  /**
   *   Subtracts this vector against other and returns a new one.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  The new created vector.
   **/
  FORCEINLINE Vector3
  operator-(const Vector3& vector3) const;

  /**
   *   Subtracts this vector against other modifying this.
   *
   * @param vector3
   *  The other vector to be subtracted.
   *
   * @return Vector3
   *  This vector reference modified.
   **/
  FORCEINLINE Vector3&
  operator-=(const Vector3& vector3);

  /**
   *   Modifies sign of this vector.
   *
   * @return
   *  A new vector3 created from this vector.
   **/
  FORCEINLINE Vector3
  operator-() const;

  /** 
   *   Copy operator. Assign operator
   * 
   * @param vector3
   *  The other vector to be set.
   **/
  FORCEINLINE Vector3&
  operator=(const Vector3& vector3);


  /**
   *   Returns the xy as Array
   *
   * @return
   *  A new Array 2 x,y
   **/
  FORCEINLINE Array<float, 2>
  xy() const { return {x, y};}


  /**
   *   Returns the xy as Array
   *
   * @return
   *  A new Array 3 x,y
   **/
  FORCEINLINE Array<float, 3>
  xyz() const { return {x, y, z};}

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
  friend ostream& operator<<(ostream& output, const Vector3& vector3) 
  {
    output << "(" << vector3.x << ", " << vector3.y << ", " << vector3.z << ")";
    return output;
  }
 public:

  /************************************************************************/
  /*
   * Static Pre-constructed Vectors.
   */
   /************************************************************************/

  /**
   * Global 3D zero vector constant (0.0f, 0.0f, 0.0f).
   */
  static const Vector3 ZERO;

  /**
   * Global 3D Unit vector constant (1.0f, 1.0f, 1.0f).
   */
  static const Vector3 UNIT;

  /**
   * Global 3D Unit x vector constant (1.0f, 0.0f, 0.0f).
   */
  static const Vector3 FORWARD;
  static const Vector3 BACKWARD;

  /**
   * Global 3D Unit y vector constant (0.0f, 1.0f, 0.0f).
   */
  static const Vector3 RIGHT;
  static const Vector3 LEFT;

  /**
   * Global 3D Unit y vector constant (0.0f, 0.0f, 1.0f).
   */
  static const Vector3 UP;
  static const Vector3 DOWN;

public:

  /**
   * Struct where you can access to x, y or z.
   */
  float x;
  float y;
  float z;

};

/**
 * @brief Multiplies a vector by a scaling factor.
 * @param scale Scaling factor.
 * @param v Vector to scale.
 * @return Result of multiplication.
 */
FORCEINLINE Vector3
operator*(float scale, const Vector3& v) {
  return v.operator*(scale);
}

/************************************************************************/
/*
 * Inline Implementations
 */
 /************************************************************************/

 /*
 */
FORCEINLINE
Vector3::Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
{}

/*
*/
FORCEINLINE
Vector3::Vector3(const float f3[3]) : x(f3[0]), y(f3[1]), z(f3[2])
{}

/*
*/
FORCEINLINE
Vector3::Vector3(const Vector3& copyVector) 
  : x(copyVector.x), 
    y(copyVector.y), 
    z(copyVector.z)
{}

/*
*/
FORCEINLINE float
Vector3::dot(const Vector3& vector3) const
{
  return {x * vector3.x + y * vector3.y + z * vector3.z};
}

/*
*/
FORCEINLINE Vector3
Vector3::cross(const Vector3& vector3) const
{
  return {y * vector3.z - z * vector3.y,
          z * vector3.x - x * vector3.z,
          x * vector3.y - y * vector3.x};
}

/*
*/
FORCEINLINE Vector3
Vector3::getAbs() const
{
  return {Math::abs(x), Math::abs(y), Math::abs(z)};
}

/*
*/
FORCEINLINE float
Vector3::magnitude() const
{
  return Math::sqrt(sqrMagnitude());
}

/*
*/
FORCEINLINE float
Vector3::sqrMagnitude() const
{
  return Math::pow(x, 2) + Math::pow(y, 2) + Math::pow(z, 2);
}

/*
*/
FORCEINLINE float
Vector3::sqrDistance(const Vector3& v) const
{
  return  (*this - v).sqrMagnitude();
}

/*
*/
FORCEINLINE float
Vector3::distance(const Vector3& v) const 
{
  return Math::sqrt(sqrDistance(v));
}

/*
*/
FORCEINLINE void
Vector3::normalize(float tolerance)
{
  const float SquareMag = sqrMagnitude();
  if (SquareMag > tolerance) {
    const float Scale = Math::invSqrt(SquareMag);
    x *= Scale;
    y *= Scale;
    z *= Scale;
    return;
  }
  *this = Vector3::ZERO;
}

/*
*/
FORCEINLINE Vector3
Vector3::getNormalized(float tolerance) const
{
  Vector3 Res = *this;
  Res.normalize(tolerance);
  return Res;
}

FORCEINLINE Vector3
Vector3::projection(const Vector3& vector3) const
{
  return (vector3 * ((dot(vector3)) / (vector3.dot( vector3))));
}

/*
*/
FORCEINLINE bool
Vector3::nearEqual( const Vector3 &v, const float &tolerance /*= Math::SMALL_NUMBER*/ ) const
{
  return (Math::abs(v.x - x) <= tolerance) &&
         (Math::abs(v.y - y) <= tolerance) &&
         (Math::abs(v.z - z) <= tolerance);
}

/*
*/
FORCEINLINE bool
Vector3::operator==(const Vector3& vector3) const
{
  return x == vector3.x && y == vector3.y && z == vector3.z;
}

/*
*/
FORCEINLINE bool
Vector3::operator!=(const Vector3& vector3) const
{
  return x != vector3.x || y != vector3.y || z != vector3.z;
}


/*
*/
FORCEINLINE Vector3
Vector3::operator*(const float& scalar) const
{
  return {x * scalar, y * scalar, z * scalar};
}

/*
*/
FORCEINLINE Vector3&
Vector3::operator*=(const float& scalar)
{
  x*= scalar;
  y*= scalar;
  z*= scalar;
  return *this;
}
/*
*/
FORCEINLINE Vector3
Vector3::operator+(const Vector3& vector3) const
{
  return {x + vector3.x, y + vector3.y, z + vector3.z};
}

/*
*/
FORCEINLINE Vector3&
Vector3::operator+=(const Vector3& vector3)
{
  x += vector3.x;
  y += vector3.y;
  z += vector3.z;
  return *this;
}

/*
*/
FORCEINLINE Vector3
Vector3::operator-(const Vector3& vector3) const
{
  return {x - vector3.x, y - vector3.y, z - vector3.z};
}

/*
*/
FORCEINLINE Vector3
Vector3::operator/(float scale) const {
  const float RScale = 1.f / scale;
  return { x * RScale, y * RScale, z * RScale };
}

/*
*/
FORCEINLINE Vector3&
Vector3::operator-=(const Vector3& vector3)
{
  x -= vector3.x;
  y -= vector3.y;
  z -= vector3.z;
  return *this;
}

/*
*/
FORCEINLINE Vector3
Vector3::operator-() const
{
  return {-x, -y, -z};
}

/*
*/
FORCEINLINE Vector3&
Vector3::operator=(const Vector3& vector3)
{
  x = vector3.x;
  y = vector3.y;
  z = vector3.z;
  return *this;
}


}