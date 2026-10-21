/************************************************************************/
/**
 * @file chRotator.cpp
 * @author AccelMR
 * @date 2022/03/17
 * @brief Rotator conversion to Quaternion.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chRotator.h"

#include "chQuaternion.h"

namespace chEngineSDK {

/*
 */
Quaternion
Rotator::toQuaternion() const noexcept
{
  return Quaternion(*this);
}
} // namespace chEngineSDK
