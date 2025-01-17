/************************************************************************/
/**
 * @file chPlane.h
 * @author AccelMR
 * @date 2022/06/09
 * @brief   Way to represent three dimensional planes.
 * 
 *  Three dimensional plane.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chVector3.h"

namespace chEngineSDK{
/*
 * Description: 
 *     This class stores the coefficients as Xx+Yy+Zz=W. 
 * NOTE: This is different from many other Plane classes that use Xx+Yy+Zz+W=0.
 *
 * Sample usage:
 *
 */
class Plane : public Vector3
{
 public:
 /*
  * @brief Default constructor
  */
  FORCEINLINE Plane() = default;

  /** 
   *   Constructor from a vector 4.
   * 
   * @param v
   *  Vector to construct this plane
   **/
  Plane(const Vector4& v);

  /** 
   *   Constructor from 4 components.
   * 
   * @param _x
   *   X component.
   *
   * @param _y
   *   Y component.
   *
   * @param _z
   *   Z component.
   *
   * @param _w
   *   W component.
   **/
  FORCEINLINE Plane(float _x, float _y, float _z, float _w);

  /** 
   *   Construct from a normal and a point
   * 
   * @param  point
   *   Any point inside the plane.
   * 
   * @param normal
   *   Normal direction of the plane.
   **/
  FORCEINLINE Plane(const Vector3& point, const Vector3& normal);

  /** 
   *   Constructor from three points that will create a plane.
   * 
   * @param p1
   *   First point to take in count.
   *
   * @param p2
   *   Second point to take in count.
   *
   * @param p3
   *   Third point to take in count.
   * 
   * @return
   **/
  FORCEINLINE Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3);

 /*
  * @brief Default destructor
  */
  ~Plane() = default;

  
  /**
   *     Calculates distance between plane and a point.
   * 
   * @param P
   *   The other point.
   * @return 
   *   >0: point is in front of the plane, <0: behind, =0: on the plane.
   */
  FORCEINLINE float
  planeDot(const Vector3& p) const;

 public:
 
 float w;
 
};

/*
*/
FORCEINLINE
Plane::Plane(const Vector4& v) : Vector3(v), w(v.w) {}

/*
*/
FORCEINLINE
Plane::Plane(float _x, float _y, float _z, float _w)
  : Vector3(_x, _y, _z),
    w(_w)
{}

/*
*/
FORCEINLINE
Plane::Plane(const Vector3& point, const Vector3& normal)
  : Vector3(normal),
    w(point.dot(normal))
{}

/*
*/
FORCEINLINE
Plane::Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3)
  :   Vector3((p2 - p1).cross((p3 - p1)).getNormalized())
{
  w = p1.dot(Vector3(*this));
}

/*
*/
FORCEINLINE float
Plane::planeDot(const Vector3& p) const
{
  return x * p.x + y * p.y + z * p.z - w;
}

}

