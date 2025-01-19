/************************************************************************/
/**
 * @file chBox2D.cpp
 * @author AccelMR
 * @date 2022/06/10
 * @brief Holds any 2D Box representation.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chBox2D.h"
#include "chMath.h"

namespace chEngineSDK{
/*
*/
Box2D::Box2D(const Vector<Vector2>& points)
  : minPoint( 0.f, 0.f ),
    maxPoint( 0.f, 0.f )
{
  for (const Vector2& EachPoint : points) {
    *this += EachPoint;
  }
}

/*
*/
void
Box2D::clamp(const Box2D& other)
{
  minPoint.x = PlatformMath::clamp(minPoint.x, other.minPoint.x, other.maxPoint.x);
  minPoint.y = PlatformMath::clamp(minPoint.y, other.minPoint.y, other.maxPoint.y);
  maxPoint.x = PlatformMath::clamp(maxPoint.x, other.minPoint.x, other.maxPoint.x);
  maxPoint.y = PlatformMath::clamp(maxPoint.y, other.minPoint.y, other.maxPoint.y);
}
}


