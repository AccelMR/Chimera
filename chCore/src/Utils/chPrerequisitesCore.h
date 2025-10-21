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

//DLL export
#if USING(CH_PLATFORM_WIN32)
# if USING(CH_COMPILER_MSVC)
#   if defined( CH_STATIC_LIB )
#     define CH_CORE_EXPORT
#   else
#     if defined( CH_CORE_EXPORTS )
#       define CH_CORE_EXPORT __declspec( dllexport )
#     else
#       define CH_CORE_EXPORT __declspec( dllimport )
#     endif
#   endif
# else  //Any other Compiler
#   if defined( CH_STATIC_LIB )
#     define CH_CORE_EXPORT
#   else
#     if defined( CH_CORE_EXPORTS )
#       define CH_CORE_EXPORT __attribute__ ((dllexport))
#     else
#       define CH_CORE_EXPORT __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define CH_CORE_HIDDEN
#else //Linux/Mac settings
# if defined( CH_CORE_EXPORTS )
#   define CH_CORE_EXPORT __attribute__ ((visibility ("default")))
# else
#   define CH_CORE_EXPORT __attribute__ ((visibility ("default")))
# endif

# define CH_CORE_HIDDEN __attribute__ ((visibility ("hidden")))
#endif
