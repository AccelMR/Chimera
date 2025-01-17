/************************************************************************/
/**
 * @file chSphereBox.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief Sphere Box combination that will cover basic culling part.
 * 
 * NOTE: This file includes Box and Sphere, there's not need to include them again.
 * This could even end up bringing issues.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chBox.h"
#include "chSphere.h"

namespace chEngineSDK{
/*
 * Description: 
 *     SphereBoxBonds class is a mix of Sphere and Box that will handle the bondings
 * of an object.
 *
 * Sample usage:
 *
 * const SphereBoxBounds FromSphereBox(Vector3::UNIT, Vector3::UNIT * 5, 6.f);
 *
 */
class SphereBoxBounds
{
 public:
 /*
  *  Default constructor
  */
  SphereBoxBounds() = default;

  /** 
   *   Constructor from a center, extent and radius.
   * 
   * @param _center
   *   Center where both box and 3D box are.
   * 
   * @param _extent
   *   Extent of the 3D box, which is the x, y, z size from center to each side.
   * 
   * @param _radius
   *   Radius of the inner sphere.
   **/
  FORCEINLINE SphereBoxBounds(const Vector3& _center, const Vector3& _extent, float _radius);

  /** 
   *   Creates a Sphere-box from a set of points. This will cover all of them.
   * 
   * @param points
   *   Dynamic array of points.
   * 
   * @return
   **/
  FORCEINLINE SphereBoxBounds(const Vector<Vector3>& points);

  /** 
   *   Constructor from a aabox and sphere.
   **/
  FORCEINLINE SphereBoxBounds(const AABox& aabox, const Sphere& sphere);

  /** 
   *   Constructs a Sphere-box from a 3DBox and sets radius as extent of the box.
   * 
   * @param aabox
   * The Aabox to be taken.
   **/
  FORCEINLINE SphereBoxBounds(const AABox& aabox);

  /** 
   *   Constructor from a sphere.
   */
  FORCEINLINE SphereBoxBounds(const Sphere& sphere);

  /**
   *   Gets the bounding box.
   * 
   * @return 
   *  The bounding box.
   */
  FORCEINLINE AABox
  getBox() const;
  
  /**
   *    Gets the bounding sphere.
   * @return 
   *   The bounding sphere.
   */
  FORCEINLINE Sphere
  getSphere() const;

 /*
  * @brief Default destructor
  */
  ~SphereBoxBounds() = default;


 public:
  Vector3 center;
  Vector3 boxExtent;
  float sphereRadius;
 
};

/*
*/
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const Vector3& _center, const Vector3& _extent, float _radius)
  : center(_center),
    boxExtent(_extent),
    sphereRadius(_radius)
{}

/*
*/
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const Vector<Vector3>& points)
{
  AABox BoundingBox(Vector3::ZERO, Vector3::UNIT);

  for (const auto &point : points) {
    BoundingBox += point;
  }

  center = BoundingBox.getCenter();
  boxExtent = BoundingBox.getExtent();
  sphereRadius = 0.f;

  for (const auto& point : points) {
    sphereRadius = Math::max(sphereRadius, (point - center).magnitude());
  }
}

/*
*/
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const AABox& aabox, const Sphere& sphere)
{
  center = aabox.getCenter();
  boxExtent = aabox.getExtent();
  sphereRadius = Math::min(boxExtent.magnitude(),
                           (sphere.center - center).magnitude() + sphere.radius);
}

/*
*/
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const AABox& aabox)
{
  center = aabox.getCenter();
  boxExtent = aabox.getExtent();
  sphereRadius = boxExtent.magnitude();
}

/*
*/
FORCEINLINE
SphereBoxBounds::SphereBoxBounds(const Sphere& sphere) 
  : center(sphere.center),
    boxExtent(Vector3(sphere.radius, sphere.radius, sphere.radius)),
    sphereRadius(sphere.radius)
{
}

/*
*/
FORCEINLINE AABox
SphereBoxBounds::getBox() const
{
  return AABox(center - boxExtent, center + boxExtent);
}

/*
*/
FORCEINLINE Sphere
SphereBoxBounds::getSphere() const
{
  return Sphere(center, sphereRadius);
}

}

