/************************************************************************/
/**
 * @file chSphere.cpp
 * @author AccelMR
 * @date 2022/06/10
 * @brief This is the geometry representation of a 3D sphere.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chSphere.h"

#include "chBox.h"
#include "chVector3.h"



namespace chEngineSDK{
/*
*/
Sphere::Sphere(const Vector<Vector3>& points)
  : center(0.f,0.f,0.f), radius(0)
{
  if (!points.empty()) {
    const AABox Box(points);
    *this = Sphere((Box.minPoint + Box.maxPoint) / 2, 0 );

    for (const auto& point : points) {
      const float Dist = point.sqrDistance(center);
      if (Dist > radius) {
        radius = Dist;
      }
    }

    radius = Math::sqrt(radius) * 1.001f;
  }
}
}


