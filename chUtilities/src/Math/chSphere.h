/************************************************************************/
/**
 * @file chSphere.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief This is the geometry representation of a 3D sphere.
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
 *     3D Sphere representation.
 * For any intersection please see Math class.
 *
 * Sample usage:
 *
 *  const Vector<Vector3> ArrayPoints = {
 *    {-100.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
 *    {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f},
 *    {-99.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
 *    {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f}
 *  };
 *  
 *  const Sphere FromPoints(ArrayPoints);
 */
class Sphere
{
 public:
 /*
  * @brief Default constructor
  */
  Sphere() = default;

  /** 
   *   Constructor that takes a center and a radius.
   * 
   * @param center
   *    The center point where this sphere is in a 3D world.
   * 
   * @param _radius
   *    The radius of this sphere.
   **/
  FORCEINLINE Sphere(const Vector3& _center, float _radius);

  /** 
   *   Constructor from a radius. Center of the sphere will be (0, 0, 0).
   * 
   * @param _radius
   *   The radius of this sphere.
   **/
  FORCEINLINE Sphere(float _radius);

  /** 
   *   Constructs a Sphere around a given array of 3D points.
   * 
   * @param points
   *    Vector array of Vector3 points.
   **/
  CH_UTILITY_EXPORT Sphere(const Vector<Vector3>& points);

 /*
  * @brief Default destructor
  */
  ~Sphere() = default;


 public:
  Vector3 center;
  float radius; 
};

/*
*/
FORCEINLINE
Sphere::Sphere(const Vector3& _center, float _radius) 
  : center(_center), 
    radius(_radius)
{}

/*
*/
FORCEINLINE
Sphere::Sphere(float _radius)
  : center(Vector3::ZERO),
    radius(_radius)
{}


}

