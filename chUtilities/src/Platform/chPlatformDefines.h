/************************************************************************/
/**
 * @file chPlatformDefines.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Basic definitions to describe target platform.
 *
 * This definitions are meant to describe the target platform.
 *
 * @bug No bug known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/**
 * Initial platform/compiler-related stuff to set.
*/
/************************************************************************/
#include <cassert>
#include "chUsing.h"

//Define the actual endian type (little endian for Windows, Linux, Apple and PS4)
#define CH_ENDIAN_LITTLE                   IN_USE
#define CH_ENDIAN_BIG                      NOT_IN_USE

#define CH_VERSION_MAJOR    0                 //Engine version major
#define CH_VERSION_MINIOR   2
#define CH_VERSION_PATCH    0
#define CH_VERSION_BUILD    1

/************************************************************************/
/**
 * Compiler type and version
 */
 /************************************************************************/

#if defined(__clang__)
#   define CH_COMPILER_MSVC               NOT_IN_USE
#   define CH_COMPILER_GNUC               NOT_IN_USE
#   define CH_COMPILER_INTEL              NOT_IN_USE
#   define CH_COMPILER_CLANG              IN_USE
#   define CH_COMP_VER __cland_version__
#   define CH_THREADLOCAL __thread
#   define CH_STDCALL __attribute__((stdcall))
#   define CH_CDECL __attribute__((cdecl))
#   define CH_FALLTHROUHG [[clang::fallthrough]];
#elif defined (__GNUC__) //Check after Cland, as Clang defines this too
#   define CH_COMPILER_MSVC               NOT_IN_USE
#   define CH_COMPILER_GNUC               IN_USE
#   define CH_COMPILER_INTEL              NOT_IN_USE
#   define CH_COMPILER_CLANG              NOT_IN_USE
#   define CH_COMP_VER (((__GNUC__)*100)+(__GNUC_MINOR__*10)+__GNUC_PATCHLEVEL__)
#   define CH_THREADLOCAL __thread
#   define CH_STDCALL __attribute__((stdcall))
#   define CH_CDECL __attribute__((cdecl))
#   define CH_FALLTHROUHG __attribute__((fallthrough));
#elif defined (__INTEL_COMPILER)
#   define CH_COMPILER_MSVC               NOT_IN_USE
#   define CH_COMPILER_GNUC               NOT_IN_USE
#   define CH_COMPILER_INTEL              IN_USE
#   define CH_COMPILER_CLANG              NOT_IN_USE
#   define CH_COMPILER CH_COMPILER_INTEL
#   define CH_COMP_VER __INTEL_COMPILER
#   define CH_STDCALL __stdcall
#   define CH_CDECL __cdecl
#   define CH_FALLTHROUHG 

/**
 * CH_THREADLOCAL define is down below because Intel compiler defines it
 * differently based on platform
 */

 //Check after Clang end Intel, we could be building with either with Vs
#elif defined (_MSC_VER)
#   define CH_COMPILER_MSVC               IN_USE
#   define CH_COMPILER_GNUC               NOT_IN_USE
#   define CH_COMPILER_INTEL              NOT_IN_USE
#   define CH_COMPILER_CLANG              NOT_IN_USE
#   define CH_COMP_VER                    _MSC_VER
#   define CH_THREADLOCAL                 __declspec
#   define CH_STDCALL                     __stdcall
#   define CH_CDECL                       __cdecl
#   define CH_FALLTHROUHG
#   undef                                 __PRETTY_FUNCTION__
#   define                                __PRETTY_FUNCTION__ __FUNCSIG__
#else
//No know compiler found, send the error to the output (if any)
#   define CH_COMPILER_MSVC               NOT_IN_USE
#   define CH_COMPILER_MSVC               NOT_IN_USE
#   define CH_COMPILER_GNUC               NOT_IN_USE
#   define CH_COMPILER_INTEL              NOT_IN_USE
#   define CH_COMPILER_CLANG              NOT_IN_USE
#   define CH_COMP_VER                    _MSC_VER
#   pragma error "No known compIler. "
#endif

/************************************************************************/
/**
 * See if we can use __forceinline or if we need to use __inline instead
 */
 /************************************************************************/
#if USING(CH_COMPILER_MSVC)
#  define CH_CPP17_OR_LATER              USE_IF(_MSVC_LANG >= 201703L)
# if CH_COMP_VER >= 1920
#  define  NODISCARD [[nodiscard]]
# else
#  define  NODISCARD
#  define CH_CPP17_OR_LATER                  USE_IF(__cplusplus >= 201703L)
# endif
# if CH_COMP_VER >= 1200
#   define FORCEINLINE                     __forceinline
#   ifndef RESTRICT 
#     define RESTRICT                      __restrict
#   endif
# endif
#elif defined (__MINGW32__)
# if !defined (FORCEINLINE)
#   define FORCEINLINE                      __inline
#   ifndef RESTRICT
#     define RESTRICT
#   endif
# endif
#else

#  define CH_CPP17_OR_LATER              USE_IF(__cplusplus >= 201703L)
# define NODISCARD [[nodiscard]]
# define FORCEINLINE                         __inline
# ifndef RESTRICT
#   define RESTRICT                          __restrict
# endif
#endif

/************************************************************************/
/**
 * Finds the current platform
 */
 /************************************************************************/
#if defined (__WIN32__) || defined (_WIN32)
#define CH_PLATFORM_LINUX                   NOT_IN_USE         //Linux Platform
#define CH_PLATFORM_OSX                     NOT_IN_USE
#define CH_PLATFORM_WIN32                   IN_USE
#elif defined (__APPLE_CC__ )
#define CH_PLATFORM_LINUX                   NOT_IN_USE         //Linux Platform
#define CH_PLATFORM_OSX                     IN_USE
#define CH_PLATFORM_WIN32                   NOT_IN_USE
#elif defined (__linux__)
#define CH_PLATFORM_LINUX                   IN_USE             //Linux Platform
#define CH_PLATFORM_OSX                     NOT_IN_USE
#define CH_PLATFORM_WIN32                   NOT_IN_USE
#endif

/************************************************************************/
/**
 * Find the architecture type
 */
 /************************************************************************/
#if defined (__x86_x64__) || defined(_M_X64)    //If this is a x64 compile
# define CH_ARCHITECTURE_X86_64             IN_USE
# define CH_ARCHITECTURE_X86_32             NOT_IN_USE
#else                                         //If it's a x86 compile
# define CH_ARCHITECTURE_X86_64             NOT_IN_USE
# define CH_ARCHITECTURE_X86_32             IN_USE
#endif

/************************************************************************/
/**
 * Memory Alignment macros
 */
 /************************************************************************/
#if USING(CH_COMPILER_MSVC)
# define MS_ALIGN(n)                        __declspec(align(n))
# ifndef GCC_PACK
#   define GCC_PACK(n)
# endif
# ifndef GCC_ALIGN
#   define GCC_ALIGN(n)
# endif
#elif USING(CH_COMPILER_GNUC)
# define MS_ALIGN(n)
# define GCC_PACK(n)
# define GCC_ALIGN(n)                      __attribute__( (__aligned__(n)) )
#else
# define MS_ALIGN(n)
# define GCC_PACK(n)                      __attribute__((packed, aligned(n)))
# define GCC_ALIGN(n)                     __attribute__( (__aligned__(n)) )
#endif

/************************************************************************/
/**
 * For throw override (deprecated on c++11 but VS does not have handle )
 */
 /************************************************************************/
#if USING(CH_COMPILER_MSVC)
# define _NOEXCEPT                        noexcept
#elif USING(CH_COMPILER_INTEL)
# define _NOEXCEPT                        noexcept
#elif USING( CH_COMPILER_GNUC)
# define _NOEXCEPT                        noexcept
#else
# define _NOEXCEPT
#endif

/************************************************************************/
/**
 * Library export specifics
 */
 /************************************************************************/
#if USING(CH_PLATFORM_WIN32)
# if USING(CH_COMPILER_MSVC) 
#   if defined( CH_STATIC_LIB )
#     define CH_UTILITY_EXPORT
#   else
#     if defined ( CH_UTILITY_EXPORTS )
#       define CH_UTILITY_EXPORT          __declspec( dllexport )
#     else
#       define CH_UTILITY_EXPORT          __declspec( dllimport )
#     endif
#   endif
#else //Any other compiler
#   if defined( CH_STATIC_LIB )
#     define CH_UTILITY_EXPORT
#   else
#     if defined( CH_UTILITY_EXPORTS )
#       define CH_UTILITY_EXPORT         // __attribute__ ((dllexport))
#     else
#       define CH_UTILITY_EXPORT         // __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define CH_UTILITY_HIDDEN
#else
# if defined( CH_UTILITY_EXPORTS )
#   define CH_UTILITY_EXPORT __attribute__ ((visibility ("default")))
# else
#   define CH_UTILITY_EXPORT
# endif
# define CH_UTILITY_HIDDEN                __attribute__ ((visibility ("hidden")))
#endif

 //DLL export plug ins
#if USING(CH_PLATFORM_WIN32)
# if USING(CH_COMPILER_MSVC)
#   define CH_PLUGIN_EXPORT               __declspec(dllexport)
# else
#   define CH_PLUGIN_EXPORT               __attribute__ ((dllexport))
# endif
#else
#  define CH_PLUGIN_EXPORT                __attribute__((visibility("default")))
#endif

/************************************************************************/
/**
 * Window specific Settings
 */
 /************************************************************************/
 //Win32
#if USING(CH_PLATFORM_WIN32)
# if defined(_DEBUG) || defined(DEBUG)
#   define CH_DEBUG_MODE                  IN_USE
# else
#   define CH_DEBUG_MODE                  NOT_IN_USE
# endif
# if USING(CH_COMPILER_INTEL)
#   define CH_THREADLOCAL                 __declspec(thread)
# endif
#endif //CH_PLATFORM

/************************************************************************/
/**
 * LINUX-Apple specific Settings
 */
 /************************************************************************/

 //
#if USING(CH_PLATFORM_LINUX) || USING(CH_PLATFORM_OSX) 
//if we're on debug mode
# if defined(_DEBUG) || defined(DEBUG)
#   define CH_DEBUG_MODE                  IN_USE
# else
#   define CH_DEBUG_MODE                  NOT_IN_USE
# endif
# if USING(CH_COMPILER_INTEL)
#   define CH_THREADLOCAL                 thread
# endif
#endif //CH_PLATFORM

/************************************************************************/
/**
 * Definition of Debug macros
 */
 /************************************************************************/
#if USING(CH_DEBUG_MODE)
# define CH_ASSERT(x)                    assert(x)

#else
# define CH_DEBUG_ONLY(x)
# define CH_ASSERT(x)
#endif

/************************************************************************/
/*
 * Extern.
 */
 /************************************************************************/
#define CH_EXTERN extern "C"

/************************************************************************/
/**
 * Disable some compiler warnings
 */
 /************************************************************************/

//If we are compiling with Visual Studio
#if USING(CH_COMPILER_MSVC)
  /**
   * TODO:  This is not deactivated anywhere, therefore it applies to any file
   * that includes this header. Right now I don't have an easier way to apply
   * these warnings globally so I'm keeping it this way.
   */

   //Secure versions aren't multi platform, so we won't be using them
# define _CRT_SECURE_NO_WARNINGS

  /**
   * Disable: nonstandard extension used: nameless struct/union.
   * Happens when a struct or union is defined without a name.
   */
# pragma warning(disable : 4201)

   /**
    * Disable: "<type> needs to have DLL interface to be used by clients'
    * Happens on STL member variables which are not public therefore is ok
    */
# pragma warning (disable: 4251)

    //Disable: 'X' Function call with parameters that may be unsafe
# pragma warning(disable: 4996) 

  /**
   * Disable: decorated name length exceeded, name was truncated. Happens with
   * really long type names. Even fairly standard use of std::unordered_map
   * with custom parameters, meaning I can't really do much to avoid it.
   * It shouldn't effect execution but might cause problems if you compile
   * library with one compiler and use it in another.
   */
# pragma warning(disable: 4503)

   /**
    * Disable: nonstandard extension used: override specifier 'keyword'.
    * Happens when a keyword was used that is not in the C++ standard, for
    * example, one of the override specifiers that also works under /clr.
    */
    //# pragma warning(disable : 4481)
#endif