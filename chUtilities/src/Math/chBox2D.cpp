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
}


