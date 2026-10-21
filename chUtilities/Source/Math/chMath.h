/************************************************************************/
/**
 * @file chMath.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Math wrapper, to use the actual library needed for the system
 *
 * If it needs a different Math library depending on the OS, here it'd be defined.
 *
 * @bug No bug known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPrerequisitesUtilities.h"
#include "chPlatformMath.h"

namespace chEngineSDK {
// Code uses Math::, so a platform can swap in its own math class here.
using Math = PlatformMath;
}

