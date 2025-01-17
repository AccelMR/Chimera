/************************************************************************/
/**
 * @file chBox2D.h
 * @author AccelMR
 * @date 2022/06/10
 * @brief Holds any 2D Box representation.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chVector2.h"

namespace chEngineSDK{
/*
 * Description: 
 *     2D Box representation.
 *
 * Sample usage:
 *
 *  const Box2D UnityBox(Vector2::ZERO, Vector2::UNIT);
 */
class Box2D
{
 public:
 /*
  * @brief Default constructor
  */
  Box2D() = default;

  /** 
   *   Constructs Box2D by a minimum and maximum point.
   * 
   * @param _minPoint
   *   Minimum point.
   * 
   * @param _maxPoint
   *   Maximum point.
   **/
  FORCEINLINE Box2D(const Vector2& _minPoint, const Vector2& _maxPoint);

  /** 
   *    Constructs a Box2D that will cover a set of given points.
   * 
   * @param points
   *   Dynamic array of points.
   **/
  CH_UTILITY_EXPORT Box2D(const Vector<Vector2>& points);

 /*
  * @brief Default destructor
  */
  ~Box2D() = default;

  /** 
   *   Returns the center point of this 2D Box.
   **/
  FORCEINLINE Vector2
  getCenter() const;

  /** 
   *   Returns a Vector2 with the X, Y sizeof this 2D Box.
   **/
  FORCEINLINE Vector2
  getSize() const;

  /** 
   *   Returns the extent of this box around the center.
   * 
   **/
  FORCEINLINE Vector2
  getExtent() const;

  /**
   *    Adds to this bounding box to include a given point.
   * 
   * @param Other 
   *   The point to increase the bounding volume to.
   * 
   * @return 
   *   Reference to this bounding box after resizing to include the other point.
   */
  FORCEINLINE Box2D&
  operator+=(const Vector2 &Other);

 public:

  Vector2 minPoint;
  Vector2 maxPoint;
 
};

/*
*/
FORCEINLINE
Box2D::Box2D(const Vector2& _minPoint, const Vector2& _maxPoint)
  : minPoint(_minPoint),
    maxPoint(_maxPoint)
{}

/*
*/
FORCEINLINE Box2D &
Box2D::operator+=(const Vector2& Other)
{
  minPoint.x = Math::min(minPoint.x, Other.x);
  minPoint.y = Math::min(minPoint.y, Other.y);
  maxPoint.x = Math::max(maxPoint.x, Other.x);
  maxPoint.y = Math::max(maxPoint.y, Other.y);
  return *this;
}

/*
*/
FORCEINLINE Vector2
Box2D::getCenter() const
{
  return Vector2((minPoint + maxPoint) * 0.5f);
}

/*
*/
FORCEINLINE Vector2
Box2D::getSize() const
{
  return (maxPoint - minPoint);
}

/*
*/
FORCEINLINE Vector2
Box2D::getExtent() const
{
  return getSize() * .5f;
}


}

