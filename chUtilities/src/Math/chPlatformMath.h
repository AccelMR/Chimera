/************************************************************************/
/**
 * @file chPlatformMath.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 *  Math class wrapper, using the STD. compatible with Windows, Linux and OSX
 *
 * This class never uses double since we don't need that much precision.
 *
 * @bug Not bug Known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {
/**
 *  Math class wrapper, using the STD. compatible with Windows, Linux and OSX.
 *
 * This class never uses double since we don't need that much precision.
 */
class CH_UTILITY_EXPORT PlatformMath
{
public:
  /*
   *  Default constructor
   */
  PlatformMath() = default;

  /*
   *  Default destructor
   */
  ~PlatformMath() = default;
  /************************************************************************/
  /*
   * Algebra.
   */
   /************************************************************************/

  /**
  *  Computes the square root of a given number.
  *
  * @param value value to compute.
  *
  * @return float The result of the square root of value.
  */
  static float
  sqrt(float value);

  /**
   *  Inverse square root of a value.
   *
   * @param value value to calculate inverse square root.
   *
   * @return float The inverse square root of value.
   */
  static float
  invSqrt(float value);

  /**
   *   Computes pow of a given value.
   *
   * @param value
   *  The value to apply pow.
   *
   * @param  vPow
   *  Number of times to be multiplied.
   *
   * @return float
   *  The calculated pow.
   **/
  static float
  pow(float value, const float vPow);

  /**
   *   Computes absolute value.
   *
   * @param value
   *  the value to get absolute.
   *
   * @return float
   *  The abs calculated.
   **/
  static float
  abs(float value);

  /**
   *   Computes the interpolation between val1 and val2 with a given time.
   * @param val1
   *  First value.
   *
   * @param val2
   *  Second value.
   *
   * @param time
   *  Time to calculate lerp.
   *
   * @return
   **/
  FORCEINLINE static float
  lerp(float val1, const float& val2, const float& time);

  /**
   *   Clamp a value within an inclusive range.
   */
  template<class T>
  FORCEINLINE static T
  clamp(const T X, const T Min, const T Max){
    return X < Min ? Min : X < Max ? X : Max;
  }

  /**
   *   Computes the inverse interpolation between val1 and val2 with given time.
   *
   * @param val1
   *  First value.
   *
   * @param val2
   *  Second value.
   *
   * @param time
   *  Time to calculate lerp.
   *
   * @return   * @param val
   *
   * @return
   **/
  FORCEINLINE static float
  invLerp(float val1, const float& val2, const float& time);

  /**
   *   Checks if given float is neither Nan nor infinite.
   *
   * @param value
   *  The value to check.
   *
   * @return bool
   *  False if value is Nan or infinite.
   **/
  static bool
  isFinite(float value);

  /**
   *   Returns the lowest between two values.
   *
   * @param x
   *    First value.
   *
   * @param y
   *    Second value.
   **/
  static float
  min(const float x, const float y);

  /**
   *   Returns the highest between two values.
   *
   * @param x
   *    First value.
   *
   * @param y
   *    Second value.
   **/
  static float
  max(const float x, const float y);

  /**
   *   Computes the floating point remainder of the division operation value1/value2.
   *
   * @param value1
   *  Float point value.
   *
   * @param value2
   *  Float point value.
   *
   * @return float
   * The reminder of the division.
   **/
  static float
  fmod(float value1, float value2);

  /************************************************************************/
  /*
   * Trigonometric functions.
   */
   /************************************************************************/
   /**
    *  Cosine result from Radian.
    *
    * @param radian Radian value to calculate cosine.
    *
    * @return Cosine from value.
    */
  static float
  cos(const Radian& radian);

  /**
   *  Cosine result from Degree.
   *
   * @param degree Degree value to calculate cosine.
   *
   * @return Cosine from value.
   */
  static float
  cos(const Degree& degree);

  /**
   *  Sine result from Radian.
   *
   * @param radian Radian value to calculate sine.
   *
   * @return Sine from value.
   */
  static float
  sin(const Radian& radian);

  /**
   *  Sine result from Degree.
   *
   * @param degree Degree value to calculate sine.
   *
   * @return Sine from value.
   */
  static float
  sin(const Degree& degree);

  /**
   *    Computes the sine and cosine of a scalar float.
   *
   * @param ScalarSin
   *    Pointer to where the sin result should be stored
   *
   * @param ScalarCos
   *    Pointer to where the cos result should be stored
   *
   * @param Value
   *    input angles
   */
  static FORCEINLINE void
  sin_cos(float* ScalarSin, float* ScalarCos, float Value);

  /**
   *  Tangent result from Radian.
   *
   * @param radian Radian value to calculate Tangent.
   *
   * @return Tangent from value.
   */
  static float
  tan(const Radian& radian);

  /**
   *  Tangent result from Degree.
   *
   * @param degree Degree value to calculate Tangent.
   *
   * @return Tangent from value.
   */
  static float
  tan(const Degree& degree);

  /**
   *  Hyperbolic Cosine result from Radian.
   *
   * @param radian Radian value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Cosine from value.
   */
  static float
  cosh(const Radian& radian);

  /**
   *  Hyperbolic Cosine result from Degree.
   *
   * @param degree Degree value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Cosine from value.
   */
  static float
  cosh(const Degree& degree);

  /**
   *  Hyperbolic Sine result from Radian.
   *
   * @param radian Radian value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Sine from value.
   */
  static float
  sinh(const Radian& radian);

  /**
   *  Hyperbolic Sine result from Degree.
   *
   * @param degree Degree value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Sine from value.
   */
  static float
  sinh(const Degree& degree);

  /**
   *  Hyperbolic Tangent result from Radian.
   *
   * @param radian Radian value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Tangent from value.
   */
  static float
  tanh(const Radian& radian);

  /**
   *  Hyperbolic Tangent result from Degree.
   *
   * @param degree Degree value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Tangent from value.
   */
  static float
  tanh(const Degree& degree);

  /************************************************************************/
  /*
   * Arc Functions.
   */
   /************************************************************************/

   // !This all functions return Radian but the programmer can use the = overload to get a Degree.
   // !Anyway Degree constructor will cal Radian run twice

   /**
    *  Arc-Cosine result from float value.
    *
    * @param valuealue float value to calculate Arc-Cosine.
    *
    * @return Angle from value.
    */
  static Radian
  acos(float valuealue);

  /**
   *  Arc-Sine result from float value.
   *
   * @param valuealue float value to calculate Arc-Sine.
   *
   * @return Angle from value.
   */
  static Radian
  asin(float valuealue);

  /**
   *  Arc-Tangent result from float value.
   *
   * @param valuealue Degree value to calculate Arc-Tangent.
   *
   * @return Angle from value.
   */
  static Radian
  atan(float valuealue);

  /**
   *  Arc-Tangent 2 result from float value.
   *
   * @param _x Degree value to calculate Arc-Tangent 2.
   * @param _y Degree value to calculate Arc-Tangent.
   *
   * @return Angle from value.
   */
  static Radian
  atan2(float _x, const float& _y);

  /**
   *  Hyperbolic Arc-Cosine result from Degree.
   *
   * @param valuealue Degree value to calculate Hyperbolic Cosine.
   *
   * @return Hyperbolic Arc-Cosine from value.
   */
  static Radian
  acosh(float valuealue);

  /**
   *  Hyperbolic Arc-Sine result from Degree.
   *
   * @param valuealue Degree value to calculate Hyperbolic Sine.
   *
   * @return Hyperbolic Arc-Sine from value.
   */
  static Radian
  asinh(float valuealue);

  /**
   *  Hyperbolic Arc-Tangent result from Degree.
   *
   * @param valuealue Degree value to calculate Hyperbolic Tangent.
   *
   * @return Hyperbolic Arc-Tangent from value.
   */
  static Radian
  atanh(float valuealue);

  /**
   *  Ensure that a Degree angle is between +/- 180 by unwinding it.
   *
   * @return degree float as unwind degree angle value.
   */
  FORCEINLINE static float
  unwindDegrees(float degree);

  /**
   *  Ensures that the radian angle sent is between +/- PI range.
   *
   * @param radian float angle value as radian.
   */
  FORCEINLINE static float
  unwindRadians(float radian);

  NODISCARD FORCEINLINE static bool
  nearEqual(float a, const float& b, const float& epsilon = SMALL_NUMBER);

  /************************************************************************/
  /*
   * Shapes
   */
  /************************************************************************/

  /**
   *   Checks if a point intersects with a AABox.
   *
   * @param point
   *  A point to be checked.
   *
   * @param aabox
   *   The box to check.
   *
   * @return
   *  True if there's an intersection.
   **/
  static bool
  pointAABIntersection(const Vector3& point, const AABox& aabox);

  /**
   *   Checks if theres any intersection between two AAboxes.
   *
   * @param aabox1
   *   The fist aabox to check.
   *
   * @param aabox2
   *   The second aabox to check.
   *
   * @return
   *   True if there's any intersection.
   **/
  static bool
  aabAABIntersection(const AABox& aabox1, const AABox& aabox2);

  /**
   *   Checks if theres any intersection between a AABox and a Plane.
   *
   * @param aabox
   *   The aabox to check.
   *
   * @param plane
   *   The plane to check.
   *
   * @return
   *   True if there's an interaction.
   **/
  static bool
  aabPlaneIntersection(const AABox& aabox, const Plane& plane);

  /**
   *   Checks if theres any intersection between a point and a Sphere.
   *
   * @param point
   *   The point to check.
   *
   * @param sphere
   *   Sphere to be checked.
   *
   * @return
   *   True if there's any intersection.
   **/
  static bool
  pointSphereIntersect(const Vector3& point, const Sphere& sphere);

  /**
   *   Checks if theres any intersection between two spheres.
   *
   * @param sphere1
   *   Sphere 1 to be checked.
   *
   * @param sphere2
   *   Sphere 2 to be checked.
   *
   * @return
   *   True if there's any intersection.
   **/
  static bool
  sphereSphereIntersect(const Sphere& sphere1, const Sphere& sphere2);

  /**
   *   Checks if theres any intersection between a AABox and a spheres.
   *
   * @param aabox
   *   The box to check.
   *
   * @param sphere
   *   Sphere to be checked.
   *
   * @return
   *   True if there's any intersection.
   **/
  static bool
  aabSphereintersection(const AABox& aabox, const Sphere& sphere);

  /**
   *   Test whether the spheres from two BoxSphereBounds intersect/overlap.
   *
   * @param sphereboxbounds1
   *   The first SphereBoxBounds.
   *
   * @param sphereboxbounds2
   *   The second SphereBoxBounds.
   *
   * @return
   *   True if there's an intersection.
   **/
  static bool
  spheresIntersect(const SphereBoxBounds& sphereboxbounds1,
                   const SphereBoxBounds& sphereboxbounds2,
                   float Tolerance = KINDA_SMALL_NUMBER);

  /**
   *   Test whether the boxes from two BoxSphereBounds intersect/overlap.
   *
   * @param sphereboxbounds1
   *   The first SphereBoxBounds.
   *
   * @param sphereboxbounds2
   *   The second SphereBoxBounds.
   *
   * @return
   *   True if there's an intersection.
   **/
  static bool
  boxesIntersect(const SphereBoxBounds& sphereboxbounds1,
                 const SphereBoxBounds& sphereboxbounds2);

  /************************************************************************/
  /*
   * Variables.
   */
   /************************************************************************/
public:

  /**
   *  PI value.
   */
  static const float PI;

  /**
   *  This Multiplied by a radian value equals Degree angle.
   */
  static const float RAD2DEG;

  /**
   *  This Multiplied by a degree value equals Radian angle.
   */
  static const float DEG2RAD;

  /**
   *  Two PI.
   */
  static const float TWO_PI;

  /**
   *  Half PI.
   */
  static const float HALF_PI;

  /**
   *  Forth PI.
   */
  static const float FORTH_PI;

  /**
   *  Small number.
   */
  static const float SMALL_NUMBER;

  /**
   *  Kind a small number.
   */
  static const float KINDA_SMALL_NUMBER;

  /**
   *   Inverse PI
   **/
  static const float INV_PI;
};

/************************************************************************/
/*
 * Implementations.
 */
 /************************************************************************/

 /*
 */
FORCEINLINE float
PlatformMath::unwindDegrees(float degree)
{
  while (180.f < degree) {
    degree -= 360.f;
  }
  while (-180.f > degree) {
    degree += 360.f;
  }
  return degree;
}

/*
*/
FORCEINLINE float
PlatformMath::unwindRadians(float radian)
{
  while (PI < radian) {
    radian -= TWO_PI;
  }
  while (-PI > radian) {
    radian += TWO_PI;
  }
  return radian;
}

/*
*/
FORCEINLINE float
PlatformMath::lerp(float val1, const float& val2, const float& time)
{
  return val1 + time * (val2 - val1);
}

/*
*/
FORCEINLINE float
PlatformMath::invLerp(float val1, const float& val2, const float& time)
{
  return (time - val1) / (val2 - val1);
}

/*
*/
FORCEINLINE void
PlatformMath::sin_cos( float *ScalarSin, float *ScalarCos, float Value )
{
  //Map Value to y in [-pi, pi], x = 2*pi*quotient + remainder.
  float quotient = (INV_PI * 0.5f) * Value;
  if ( 0.0f <= Value ) {
    quotient = static_cast<float>(static_cast<int>(quotient + 0.5f));
  }
  else {
    quotient = static_cast<float>(static_cast<int>(quotient - 0.5f));
  }

  float y = Value - TWO_PI * quotient;

  //Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
  float fSign;
  if ( HALF_PI < y ) {
    y = PI - y;
    fSign = -1.0f;
  }
  else if ( -HALF_PI > y ) {
    y = -PI - y;
    fSign = -1.0f;
  }
  else {
    fSign = +1.0f;
  }

  float y2 = y * y;

  //11-degree minimax approximation
  *ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f)
    * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

  //10-degree minimax approximation
  float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f)
    * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;

  *ScalarCos = fSign * p;
}

/*
*/
FORCEINLINE bool
PlatformMath::nearEqual(float a, const float& b, const float& epsilon) {
  return (abs(a - b) <= epsilon);
}

} // namespace chEngineSDK
