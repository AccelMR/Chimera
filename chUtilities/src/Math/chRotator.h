/************************************************************************/
/**
 * @file chRotator.h
 * @author AccelMR
 * @date 2022/03/17
 *   Rotator Declaration file.
 * 
 *
 * Coordinate system being X = front, Z = up, Y = right
 */
 /************************************************************************/
#pragma once


/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chDegree.h"
#include "chMath.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Implements a container for 3 Degree values as rotation. 
 *
 * Sample usage:
 *  Rotator r(90,90,90);
 *  r.normalize();
 *  Rotator r2 = r.getNormalized();
 *
 */
class CH_UTILITY_EXPORT Rotator
{
 public:
 /*
  * @brief Default constructor
  */
  FORCEINLINE Rotator() = default;

  /** 
   *   Constructor from degrees.
   **/
  FORCEINLINE Rotator(const Degree& _pitch, const Degree& _yaw, const Degree& _roll);

  /**
   *   Constructor from floats as degree.
   **/
  FORCEINLINE Rotator(float _pitch, float _yaw, float _roll);

 /*
  * @brief Default destructor
  */
  ~Rotator() = default;

  /************************************************************************/
  /*
   * Methods.
   */                                                                     
  /************************************************************************/

  /** 
   *   Makes a diagnostic for this rotator. If any element in the rotator is 
   *  NaN or non finite number then it'll throw a warning IF is in debug mode.
   * 
   * NOTE:  This checker executes ONLY in Debug mode. 
   * 
   * @param 
   * 
   * @return
   **/
  FORCEINLINE void 
  diagnosticNaN() const;

  /** 
   *   Normalizes this Rotator.Removes all winding and creates the shortest 
   *      route rotation.
   *
   **/
  FORCEINLINE void
  normalize();

  /** 
   *   Creates a new Rotator normalized from this one.
   * 
   * @return Rotator
   *   The new rotator created from this.
   **/
  FORCEINLINE Rotator
  getNormalized() const;

  /** 
   *   Modifies this rotator clamping each axis to 0 - 360.
   **/
  FORCEINLINE void 
  denormalize();

  /** 
   *   Creates a new copy Rotator from this, and clamped the axis to 0 -360.
   * 
   * @return Rotator
   *   A new rotator clamped from this rotator.
   **/
  FORCEINLINE Rotator
  getDenormalized() const;

  /** 
   *   Compares Rotator against another rotator taking in count a threshold to be accepted.
   * 
   * @param r
   *   The rotator to be compared against.
   * 
   * @param threshold
   *   The number to be used in the comparative.
   * 
   * @return
   **/
  FORCEINLINE bool
  nearEqual(const Rotator& r, const float threshold = Math::KINDA_SMALL_NUMBER) const;

  /** 
   *   Creates a new quaternion taking this rotator as in parameter.
   * 
   * @return Quaternion
   *  New quaternion generated from this.
   **/
  Quaternion 
  toQuaternion() const;

  /************************************************************************/
  /*
   * Operator overloads.
   */                                                                     
  /************************************************************************/
  
  /** 
   *   Comparator operator.
   * 
   * @param other
   *  The other rotator to compare to.
   * 
   * @return bool
   *  TRue if both Rotators are exactly the same.
   **/
  FORCEINLINE bool
  operator==(const Rotator& other) const;

  /**
   *   Get the result of adding a rotator to this.
   * 
   * @param R 
   *  The other rotator.
   * 
   * @return
   *  The result of adding a rotator to this.
   */
  FORCEINLINE Rotator
  operator+(const Rotator & R) const;

  /**
   * @brief Get the result of subtracting a rotator from this.
   * 
   * @param R 
   *  The other rotator.
   * 
   * @return The result of subtracting a rotator from this.
   */
  FORCEINLINE Rotator
  operator-(const Rotator & R) const;

  /**
   * @brief Get the result of scaling this rotator.
   * 
   * @param Scale 
   *  The scaling factor.
   * 
   * @return The result of scaling.
   */
  FORCEINLINE Rotator
  operator*(float Scale) const;

  /**
   * @brief Multiply this rotator by a scaling factor.
   * 
   * @param Scale 
   *  The scaling factor.
   * 
   * @return Copy of the rotator after scaling.
   */
  FORCEINLINE Rotator&
  operator*=(float Scale);

  /************************************************************************/
  /*
   * Static methods.
   */                                                                     
  /************************************************************************/

  /** 
   *   Clamps an angle to the range of [0, 360).
   * 
   * @param Angle
   * Angle as degree.
   * 
   * @return Degree
   *  The new generated Degree.
   **/
  static FORCEINLINE Degree
  clampAxis(const Degree& angle);

  /** 
   *   Clamps an angle to the range of (-180, 180].
   * 
   * @param Angle
   *  The angle to be clamped.
   * 
   * @return Degree
   *  The new degree generated.
   **/
  static FORCEINLINE Degree
  normalizeAxis(const Degree& angle);

 /** 
  *   Checks if this rotator contains NaN.
  * 
  * @return bool
  *    True if it does not contains NaN.
  **/
  FORCEINLINE bool 
  checkIfNaN() const;

 public:
 /**
  * @brief Rotation around the right axis (around Y axis), Looking up and down
  *        (0=Straight Ahead, +Up, -Down)
  */
  Degree pitch;

 /**
  * @brief Rotation around the up axis (around Z axis), Running in circles
  *        0=East, +North, -South.
  */
  Degree yaw;

 /**
  * @brief Rotation around the forward axis (around X axis), Tilting your head,
  *        0=Straight, +Clockwise, -CCW.
  */
  Degree roll;


 /**
  * @brief A rotator of zero degrees on each axis.
  */
  static const Rotator ZERO;
 
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
Rotator::Rotator(const Degree& _pitch, const Degree& _yaw, const Degree& _roll)
  : pitch(_pitch), yaw(_yaw), roll(_roll)
{
  diagnosticNaN();
}

/*
*/
FORCEINLINE
Rotator::Rotator(float _pitch, float _yaw, float _roll)
  : pitch(_pitch), yaw(_yaw), roll(_roll)
{
  diagnosticNaN();
}

/*
*/
FORCEINLINE void
Rotator::diagnosticNaN() const
{
#if USING(CH_DEBUG_MODE)
  if (checkIfNaN()) {
    //Reset this Rotator
    *const_cast<Rotator*>(this) = Rotator::ZERO;
  }
#endif // CH_DEBUG_MODE
}

/*
*/
FORCEINLINE bool
Rotator::checkIfNaN() const
{
  return !Math::isFinite(pitch.valueDegree()) || 
         !Math::isFinite(yaw.  valueDegree()) || 
         !Math::isFinite(roll. valueDegree());
}

/*
*/
FORCEINLINE void
Rotator::normalize()
{
  pitch = normalizeAxis(pitch);
  yaw = normalizeAxis(yaw);
  roll = normalizeAxis(roll);
  diagnosticNaN();
}

/*
*/
FORCEINLINE Rotator
Rotator::getNormalized() const
{
  Rotator ret = *this;
  ret.normalize();
  return ret;
}

/*
*/
FORCEINLINE void
Rotator::denormalize()
{
  pitch = clampAxis(pitch);
  yaw = clampAxis(yaw);
  roll = clampAxis(roll);
  diagnosticNaN();
}

/*
*/
FORCEINLINE Rotator
Rotator::getDenormalized() const
{
  Rotator ret = *this;
  ret.denormalize();
  return ret;
}

/*
*/
FORCEINLINE bool
Rotator::nearEqual( const Rotator &r, const float threshold /*= Math::KINDA_SMALL_NUMBER*/ ) const
{
  return (Math::abs(r.roll.valueDegree() - roll.valueDegree()) <= threshold)   &&
         (Math::abs(r.pitch.valueDegree() - pitch.valueDegree()) <= threshold) &&
         (Math::abs(r.yaw.valueDegree() - yaw.valueDegree()) <= threshold);
}


/*
*/
FORCEINLINE bool
Rotator::operator==(const Rotator& other) const
{
  return nearEqual(other, 0.0f);
}

/*
*/
FORCEINLINE Rotator
Rotator::operator+(const Rotator& R) const
{
  return Rotator(pitch + R.pitch, yaw + R.yaw, roll + R.roll);
}

/*
*/
FORCEINLINE Rotator
Rotator::operator-(const Rotator& R) const
{
  return Rotator(pitch - R.pitch, yaw - R.yaw, roll - R.roll);
}

/*
*/
FORCEINLINE Rotator
Rotator::operator*(float Scale) const
{
  return Rotator(pitch * Scale, yaw * Scale, roll * Scale);
}

/*
*/
FORCEINLINE Rotator&
Rotator::operator*=(float Scale)
{
  pitch = pitch * Scale; 
  yaw = yaw * Scale; 
  roll = roll * Scale;
  diagnosticNaN();
  return *this;
}

/*
*/
FORCEINLINE Degree
Rotator::clampAxis(const Degree& angle)
{
  float ret = Math::fmod(angle.valueDegree(), 360.f);

  if (0.f > ret) {
    //shift to [0, 360) range
    ret += 360.f;
  }

  return Degree(ret);
}

/*
*/
FORCEINLINE Degree
Rotator::normalizeAxis(const Degree& angle)
{
  return Degree(Math::unwindDegrees(angle.valueDegree()));
}

}
