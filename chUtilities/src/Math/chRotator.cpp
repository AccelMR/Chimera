/************************************************************************/
/**
 * @file chRotator.cpp
 * @author AccelMR
 * @date 2022/03/17
 *   Rotator Implementation file.
 * 
 * 
 * Coordinate system being X = right, Z = up, Y = front
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chRotator.h"

#include "chQuaternion.h"
#include "chRadian.h"
#include "chVector3.h"



namespace chEngineSDK{
const Rotator Rotator::ZERO(0.0f, 0.0f, 0.0f);

/*
*/
Quaternion
Rotator::toQuaternion() const
{
  diagnosticNaN();
  const Radian PitchHalves(pitch.valueRadian() * .5f);
  const Radian YawHalves(yaw.valueRadian() * .5f);
  const Radian RollhHalves(roll.valueRadian() * .5f);

  const float SP = Math::sin(PitchHalves);
  const float CP = Math::cos(PitchHalves);

  const float SY = Math::sin(YawHalves);
  const float CY = Math::cos(YawHalves);

  const float SR = Math::sin(RollhHalves);
  const float CR = Math::cos(RollhHalves);

  //Fast Multiplications
//   const float CYCP = CY * CP;
//   const float CYSP = CY * SP;
//   const float SYSP = SY * SP;
//   const float SYCP = SY * CP;

  //Bank = Roll
  //Attitude = Pitch
  //Heading = Yaw
  //http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm
  Quaternion RotationQuat;
  RotationQuat.w = SY * SP * SR + CY * CP * CR;
  RotationQuat.x = CY * CP * SR - SY * SP * CR;
  RotationQuat.y = SY * CP * SR + CY * SP * CR;
  RotationQuat.z = SY * CP * CR - CY * SP * SR;

#if USING(CH_DEBUG_MODE)
  //Very large inputs can cause NaN's. Want to catch this here
  CH_ASSERT(!RotationQuat.containsNaN() && 
            "Invalid input to Rotator::toQuaternion - generated NaN output");
#endif //CH_DEBUG_MODE
  return RotationQuat;

//   Quaternion qRoll(Vector3::FORWARD, roll);
//   Quaternion qPitch(Vector3::RIGHT, pitch);
//   Quaternion qYaw(Vector3::UP, yaw);
// 
//   return qPitch * (qYaw * qRoll);

}
}
