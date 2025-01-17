/************************************************************************/
/**
 * @file chPlatformMath.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Math class wrapper, using the STD. compatible with Windows, Linux and OSX
 *
 * This class never uses double since we don't need that much precision.
 *
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPlatformMath.h"

#include "chRadian.h"
#include "chDegree.h"
#include "chVector3.h"
#include "chVector4.h"
#include "chMatrix4.h"
#include "chPlane.h"
#include "chSphereBoxBounds.h"

namespace chEngineSDK {

/************************************************************************/
/*
 * Static variables.
 */
 /************************************************************************/
const float PlatformMath::PI = acosf(-1.0f);

const float PlatformMath::RAD2DEG = 180.0f / PI;

const float PlatformMath::DEG2RAD = PI / 180.0f;

const float PlatformMath::TWO_PI = PI * 2.0f;

const float PlatformMath::HALF_PI = PI * 0.5f;

const float PlatformMath::FORTH_PI = PI / 4.0f;

const float PlatformMath::SMALL_NUMBER = (1.e-6f);

const float PlatformMath::KINDA_SMALL_NUMBER = (1.e-4f);

const float PlatformMath::INV_PI = 1.0f / PI;

/************************************************************************/
/*
 * Functions.
 */
 /************************************************************************/

/*
*/
float
PlatformMath::sqrt(const float& value)
{
  return std::sqrt(value);
}

/*
*/
float
PlatformMath::invSqrt(const float& value)
{
  return 1.0f / sqrtf(value);
}

/*
*/
float
PlatformMath::pow(const float& value, const float vPow)
{
  return std::pow(value, vPow);
}

/*
*/
float
PlatformMath::abs(const float& value)
{
  return std::abs(value);
}

/*
*/
bool
PlatformMath::isFinite(float value)
{
  return std::isfinite(value);
}

/*
*/
float
PlatformMath::min(const float x, const float y)
{
  return std::min(x, y);
}

/*
*/
float
PlatformMath::max( const float x, const float y )
{
  return std::max(x, y);
}

/*
*/
float
PlatformMath::fmod(float value1, float value2)
{
  return fmodf(value1, value2);
}

/*
*/
float
PlatformMath::cos(const Radian& radian)
{
  return cosf(radian.valueRadian());
}

/*
*/
float
PlatformMath::cos(const Degree& degree)
{
  return cosf(degree.valueRadian());
}

/*
*/
float
PlatformMath::sin(const Radian& radian)
{
  return sinf(radian.valueRadian());
}

/*
*/
float
PlatformMath::sin(const Degree& degree)
{
  return sinf(degree.valueRadian());
}

/*
*/
float
PlatformMath::tan(const Radian& radian)
{
  return tanf(radian.valueRadian());
}

/*
*/
float
PlatformMath::tan(const Degree& degree)
{
  return tanf(degree.valueRadian());
}

/*
*/
float
PlatformMath::cosh(const Radian& radian)
{
  return coshf(radian.valueRadian());
}

/*
*/
float
PlatformMath::cosh(const Degree& degree)
{
  return coshf(degree.valueRadian());
}

/*
*/
float
PlatformMath::sinh(const Radian& radian)
{
  return sinhf(radian.valueRadian());
}

/*
*/
float
PlatformMath::sinh(const Degree& degree)
{
  return sinhf(degree.valueRadian());
}

/*
*/
float
PlatformMath::tanh(const Radian& radian)
{
  return tanhf(radian.valueRadian());
}

/*
*/
float
PlatformMath::tanh(const Degree& degree)
{
  return tanhf(degree.valueRadian());
}

/*
*/
Radian
PlatformMath::acos(const float& valuealue)
{
  return Radian(acosf(valuealue));
}

/*
*/
Radian
PlatformMath::asin(const float& valuealue)
{
  return Radian(asinf(valuealue));
}

/*
*/
Radian
PlatformMath::atan(const float& valuealue)
{
  return Radian(atanf(valuealue));
}
/*
*/
Radian
PlatformMath::atan2(const float& y, const float& x)
{
  return Radian(atan2f(y, x));
}

/*
*/
Radian
PlatformMath::acosh(const float& valuealue)
{
  return Radian(std::acoshf(valuealue));
}

/*
*/
Radian
PlatformMath::asinh(const float& valuealue)
{
  return Radian(std::asinhf(valuealue));
}

/*
*/
Radian
PlatformMath::atanh(const float& valuealue)
{
  return Radian(std::atanhf(valuealue));
}
}


/************************************************************************/
/*
 * Intersections
 */                                                                     
/************************************************************************/
namespace chEngineSDK{
/*
*/
bool
PlatformMath::pointAABIntersection(const Vector3& point, const AABox& aabox)
{
  if (point.x >= aabox.minPoint.x && point.x <= aabox.maxPoint.x &&
      point.y >= aabox.minPoint.y && point.y <= aabox.maxPoint.y &&
      point.z >= aabox.minPoint.z && point.z <= aabox.maxPoint.z) {
    return true;
  }
  return false;
}

/*
*/
bool
PlatformMath::aabAABIntersection(const AABox& aabox1, const AABox& aabox2)
{
  if ( (aabox1.minPoint.x > aabox2.maxPoint.x) || (aabox2.minPoint.x > aabox1.maxPoint.x) ) {
    return false;
  }

  if ( (aabox1.minPoint.y > aabox2.maxPoint.y) || (aabox2.minPoint.y > aabox1.maxPoint.y) ) {
    return false;
  }

  if ( (aabox1.minPoint.z > aabox2.maxPoint.z) || (aabox2.minPoint.z > aabox1.maxPoint.z) ) {
    return false;
  }

  return true;
}

/*
*/
bool
PlatformMath::aabPlaneIntersection(const AABox& aabox, const Plane& plane)
{
  const Vector3 Center = aabox.getCenter();
  const Vector3 Extends = aabox.maxPoint - Center;

  const float DotProject = (Extends.x * Math::abs(plane.x))+
                           (Extends.y * Math::abs(plane.y)) +
                           (Extends.z * Math::abs(plane.z));

  const float Box2PlaneDistance = plane.planeDot(Center) - plane.w;

  // Intersection occurs when distance s falls within [-r,+r] interval
  return Math::abs(Box2PlaneDistance) <= DotProject;
}

/*
*/
bool
PlatformMath::pointSphereIntersect(const Vector3& point, const Sphere& sphere)
{
  const float Distance = point.distance(sphere.center);
  return Distance < sphere.radius;
}

/*
*/
bool
PlatformMath::sphereSphereIntersect(const Sphere& sphere1, const Sphere& sphere2)
{
  const float Distance = sphere1.center.distance(sphere2.center);
  return Distance < (sphere1.radius + sphere2.radius);
}

/*
*/
bool
PlatformMath::aabSphereintersection(const AABox& aabox, const Sphere& sphere)
{
  const float x = Math::max(aabox.minPoint.x, Math::min(sphere.center.x, aabox.maxPoint.x));
  const float y = Math::max(aabox.minPoint.y, Math::min(sphere.center.y, aabox.maxPoint.y));
  const float z = Math::max(aabox.minPoint.z, Math::min(sphere.center.z, aabox.maxPoint.z));
  const Vector3 ClosesPoint(x, y, z);
  const float Distance = ClosesPoint.distance(sphere.center);

  return Distance < sphere.radius;
}

/*
*/
bool
PlatformMath::spheresIntersect(const SphereBoxBounds &sphereboxbounds1,
                               const SphereBoxBounds &sphereboxbounds2,
                               float Tolerance /*= Math::KINDA_SMALL_NUMBER*/)
{
  return (sphereboxbounds1.center - sphereboxbounds2.center).sqrMagnitude() <=
          Math::sqrt(Math::max(0.f,
            sphereboxbounds1.sphereRadius +
            sphereboxbounds2.sphereRadius + Tolerance));
}

/*
*/
bool
PlatformMath::boxesIntersect(const SphereBoxBounds &sphereboxbounds1,
                             const SphereBoxBounds &sphereboxbounds2)
{
  return Math::aabAABIntersection(sphereboxbounds1.getBox(),
                                  sphereboxbounds2.getBox());
}

}