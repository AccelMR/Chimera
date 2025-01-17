/************************************************************************/
/**
 * @file chBox.h
 * @author AccelMR
 * @date 2022/06/03
 *   
 *   Box classes along the engine. 
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
 #include "chPrerequisitesUtilities.h"

#include "chVector3.h"
#include "chVector4.h"
#include "chMatrix4.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Axis aligned box used to determine whether two entities are overlapping.
 *
 * Sample usage:
 *  AABox aabox(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
 *
 */
class AABox
{
 public:
 /*
  *  Default constructor
  */
  AABox() = default;

  /** 
   *    AABox constructor from min and max points.
   * 
   * @param _min
   *   Minimum point of this box.
   * 
   * @param _max
   *   Maximum point of this box;
   * 
   * @return
   **/
  FORCEINLINE AABox(const Vector3& _min, const Vector3& _max);

  /** 
   *   Creates and initializes a new box from an array of points.
   * 
   * @param points
   *    Vector of points to take in count to construct this Box.
   **/
  FORCEINLINE AABox(const Vector<Vector3>& points);
 
 /*
  *  Default destructor
  */
  ~AABox() = default;

  /** 
   *   Returns center point of this box.
   **/
  FORCEINLINE Vector3
  getCenter() const;

  /** 
   *   Returns the size of this box in each axis.
   **/
  FORCEINLINE Vector3
  getSize() const;

  /** 
   *   Returns the x, y, z distance from the center of the box
   **/
  FORCEINLINE Vector3
  getExtent() const;

  /** 
   *   Moves this box to a certain location.
   * 
   * @param destination
   *    Where to move this box.
   **/
  FORCEINLINE void
  moveTo(const Vector3& destination);

  /** 
   *   Shifts the bounding box position.
   * 
   * @param offset
   *   The offset to move this box.
   **/
  FORCEINLINE void
  shiftBy(const Vector3& offset);

  /** 
   *   Transforms ans projects a world bounding box to screen space.
   * 
   * @param projection
   *   Projection matrix to be used.
   * 
   * @return
   *   A new Bounding Box created from the transformation.
   **/
  FORCEINLINE AABox
  transformProjection(const Matrix4& projection) const;

  FORCEINLINE Array<Vector4, 8>
  generateVertices4() const;

  FORCEINLINE Array<Vector3, 8>
  generateVertices3() const;

  FORCEINLINE Array<uint16, 36>
  getConstIndices() const;
 
  /**
   *   Adds to this bounding box to include a given point.
   * 
   * @param v
   *    The point to increase the bounding volume to.
   * 
   * @return 
   *   Reference to this bounding box after resizing to include the other point.
   */
  FORCEINLINE AABox&
  operator+=(const Vector3& v);
 
 public:

  /*
  * Holds the box's minimum point.
  */
  Vector3 minPoint;
  
  /*
  * Holds the box's maximum point.
  */
  Vector3 maxPoint;
 
};
}

/************************************************************************/
/*
 * Implementation.
 */                                                                     
/************************************************************************/
namespace chEngineSDK{

/*
*/
FORCEINLINE
AABox::AABox(const Vector3& _min, const Vector3& _max)
  : minPoint(_min), 
    maxPoint(_max)
{}

/*
*/
FORCEINLINE
AABox::AABox(const Vector<Vector3>& points)
  : minPoint(0.0f, 0.0f, 0.0f),
    maxPoint(0.0f, 0.0f, 0.0f)
{
  for (auto& Point: points) {
    *this += Point;
  }
}

/*
*/
FORCEINLINE Vector3
AABox::getCenter() const
{
  return Vector3((minPoint+maxPoint) * 0.5f);
}

/*
*/
FORCEINLINE Vector3
AABox::getSize() const
{
  return (maxPoint - minPoint);
}

/*
*/
FORCEINLINE Vector3
AABox::getExtent() const
{
  return getSize() *.5f;
}



/*
*/
FORCEINLINE void
AABox::shiftBy(const Vector3& offset)
{
  minPoint += offset;
  maxPoint += offset;
}

/*
*/
FORCEINLINE void
AABox::moveTo(const Vector3& destination)
{
  const Vector3 Offset = destination - getCenter();
  minPoint += Offset;
  maxPoint += Offset;
}

/*
*/
FORCEINLINE AABox
AABox::transformProjection(const Matrix4& projection) const
{
  AABox NewBox = *this;
  for (auto& Vertice : generateVertices4()) {
    Vector4 ProjectedVertex = projection.transformVector4(Vertice);
    NewBox += (static_cast<Vector3>(ProjectedVertex)) / ProjectedVertex.w;
  }
  
  return NewBox;
}

/*
*/
FORCEINLINE Array<Vector4, 8>
AABox::generateVertices4() const{
  Array<Vector4, 8> ret;

  ret[0] = Vector4(minPoint, 1.0f);
  ret[1] = Vector4({minPoint.x, maxPoint.y, minPoint.z}, 1.0f);
  ret[2] = Vector4({maxPoint.x, maxPoint.y, minPoint.z}, 1.0f);
  ret[3] = Vector4({maxPoint.x, minPoint.y, minPoint.z}, 1.0f);
  ret[4] = Vector4({minPoint.x, minPoint.y, maxPoint.z}, 1.0f);
  ret[5] = Vector4({minPoint.x, maxPoint.y, maxPoint.z}, 1.0f);
  ret[6] = Vector4({maxPoint}, 1.0f);
  ret[7] = Vector4({maxPoint.x, minPoint.y, maxPoint.z}, 1.0f);

  return ret;
}

/*
*/
FORCEINLINE Array<Vector3, 8>
AABox::generateVertices3() const{
  Array<Vector3, 8> ret;

  ret[0] = minPoint;
  ret[1] = Vector3(minPoint.x, maxPoint.y, minPoint.z);
  ret[2] = Vector3(maxPoint.x, maxPoint.y, maxPoint.z);
  ret[3] = Vector3(maxPoint.x, minPoint.y, minPoint.z);
  ret[4] = Vector3(minPoint.x, minPoint.y, maxPoint.z);
  ret[5] = Vector3(minPoint.x, maxPoint.y, maxPoint.z);
  ret[6] = maxPoint;
  ret[7] = Vector3(maxPoint.x, minPoint.y, maxPoint.z);

  return ret;
}

/*
*/
FORCEINLINE AABox &
AABox::operator+=(const Vector3& v)
{
  minPoint.x = Math::min(minPoint.x, v.x);
  minPoint.y = Math::min(minPoint.y, v.y);
  minPoint.z = Math::min(minPoint.z, v.z);
                               
  maxPoint.x = Math::max(maxPoint.x, v.x);
  maxPoint.y = Math::max(maxPoint.y, v.y);
  maxPoint.z = Math::max(maxPoint.z, v.z);

  return *this;
}

/*
*/
FORCEINLINE Array<uint16, 36>
AABox::getConstIndices() const {
  static Array<uint16, 36> ret{
    0, 1, 2, 
    0, 2, 3,
    4, 6, 5,
    4, 7, 6,
    4, 5, 1,
    4, 1, 0,
    3, 2, 6, 
    3, 6, 7,
    1, 5, 6,
    1, 6, 2,
    4, 0, 3, 
    4, 3, 7
  };
  return ret;
}
}
