/************************************************************************/
/**
 * @file chRotator.cpp
 * @author AccelMR
 * @date 2022/03/17
 *   Rotator Implementation file.
 * 
 * 
 * Coordinate system: X = forward, Y = right, Z = up, left-handed.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chRotator.h"

#include "chQuaternion.h"



namespace chEngineSDK{
const Rotator Rotator::ZERO(0.0f, 0.0f, 0.0f);

/*
*/
Quaternion
Rotator::toQuaternion() const
{
  return Quaternion(*this);
}
}
