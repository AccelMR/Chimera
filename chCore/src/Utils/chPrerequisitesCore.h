/************************************************************************/
/**
 * @file chPrerequisitesCore.h
 * @author AccelMR
 * @date 2022/09/11
 *   Prerequisites to be included along the Core project.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Include.
 */
/************************************************************************/
#include <chPrerequisitesUtilities.h>

#include "chFwdDeclCore.h"

#define INVALID_INDEX -1
#define INVALID_UNSIGNED_INDEX 0xFFFFFFFF

#ifdef CH_EDITOR_ENABLED
# define CH_EDITOR IN_USE
#else
# define CH_EDITOR NOT_IN_USE
#endif

#ifdef CH_SDL3_ENABLED
# define CH_DISPLAY_SDL3  IN_USE
#else
# define CH_DISPLAY_SDL3 NOT_IN_USE
#endif

#ifdef CH_CODECS_ENABLED
# define CH_CODECS IN_USE
#else
# define CH_CODECS NOT_IN_USE
#endif

#if defined(CH_STATIC_LIB)
# define CH_CORE_EXPORT
#elif defined(CH_CORE_EXPORTS)
# define CH_CORE_EXPORT CH_DLL_EXPORT
#else
# define CH_CORE_EXPORT CH_DLL_IMPORT
#endif
