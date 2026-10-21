/************************************************************************/
/**
 * @file chPrerequisitesUtilities.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Utilities include with the most basic defines needed.
 *
 * Utilities include with the most basic defines needed for compilation
 * This is the file that must be included for use on the external objects.
 *
 * @bug No bug known.
 */
 /************************************************************************/
#pragma once


/************************************************************************/
/*
 * Platform specific stuff
 */
 /************************************************************************/
#include "chPlatformDefines.h"

//Platform Types
#include "chPlatformTypes.h"

//STD headers and wrapper
#include "chSTDHeaders.h"

//Forward Declarations for Utilities Project
#include "chForwardDecUtil.h"

#include "chException.h"

// Off in shipping game builds, so profile scopes and render stats compile to nothing.
#ifdef CH_PROFILER_ENABLED
# define CH_PROFILER IN_USE
#else
# define CH_PROFILER NOT_IN_USE
#endif
