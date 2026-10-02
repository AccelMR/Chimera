/************************************************************************/
/**
 * @file chPlatformDefines.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Macros that describe the compiler, platform and build type.
 *
 * Every feature flag is defined as IN_USE or NOT_IN_USE on every supported
 * platform, so it must be checked with USING() and never with #ifdef.
 *
 * @bug No bug known.
 */
/************************************************************************/
#pragma once

#include <cassert>

#include "chUsing.h"

/************************************************************************/
/**
 * Engine version
 */
/************************************************************************/
#define CH_VERSION_MAJOR 0
#define CH_VERSION_MINOR 2
#define CH_VERSION_PATCH 0
#define CH_VERSION_BUILD 1

#define CH_STRINGIFY(x) #x
#define CH_TOSTRING(x) CH_STRINGIFY(x)

#define CH_ENGINE_VERSION_STRING                                                              \
  CH_TOSTRING(CH_VERSION_MAJOR) "." CH_TOSTRING(CH_VERSION_MINOR) "."                         \
  CH_TOSTRING(CH_VERSION_PATCH) "." CH_TOSTRING(CH_VERSION_BUILD)

/************************************************************************/
/**
 * Compiler
 */
/************************************************************************/
// Clang must be checked first because it also defines __GNUC__ and, as clang-cl, _MSC_VER.
#if defined(__clang__)
# define CH_COMPILER_CLANG IN_USE
# define CH_COMP_VER (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
# define CH_COMPILER_GNUC IN_USE
# define CH_COMP_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
# define CH_COMPILER_MSVC IN_USE
# define CH_COMP_VER _MSC_VER
#else
# error "Unsupported compiler."
#endif

#if !defined(CH_COMPILER_CLANG)
# define CH_COMPILER_CLANG NOT_IN_USE
#endif
#if !defined(CH_COMPILER_GNUC)
# define CH_COMPILER_GNUC NOT_IN_USE
#endif
#if !defined(CH_COMPILER_MSVC)
# define CH_COMPILER_MSVC NOT_IN_USE
#endif

// MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is set, so read _MSVC_LANG there.
#if defined(_MSVC_LANG)
# define CH_CPP_VERSION _MSVC_LANG
#else
# define CH_CPP_VERSION __cplusplus
#endif

#if CH_CPP_VERSION < 202002L
# error "Chimera requires C++20 or later."
#endif

/************************************************************************/
/**
 * Platform
 */
/************************************************************************/
#if defined(_WIN32)
# define CH_PLATFORM_WIN32 IN_USE
#elif defined(__APPLE__)
# define CH_PLATFORM_OSX IN_USE
#elif defined(__linux__)
# define CH_PLATFORM_LINUX IN_USE
#else
# error "Unsupported platform."
#endif

#if !defined(CH_PLATFORM_WIN32)
# define CH_PLATFORM_WIN32 NOT_IN_USE
#endif
#if !defined(CH_PLATFORM_OSX)
# define CH_PLATFORM_OSX NOT_IN_USE
#endif
#if !defined(CH_PLATFORM_LINUX)
# define CH_PLATFORM_LINUX NOT_IN_USE
#endif

/************************************************************************/
/**
 * Architecture and endianness
 */
/************************************************************************/
#if defined(__x86_64__) || defined(_M_X64)
# define CH_ARCHITECTURE_X86_64 IN_USE
#elif defined(__aarch64__) || defined(_M_ARM64)
# define CH_ARCHITECTURE_ARM64 IN_USE
#elif defined(__i386__) || defined(_M_IX86)
# define CH_ARCHITECTURE_X86_32 IN_USE
#else
# error "Unsupported architecture."
#endif

#if !defined(CH_ARCHITECTURE_X86_64)
# define CH_ARCHITECTURE_X86_64 NOT_IN_USE
#endif
#if !defined(CH_ARCHITECTURE_ARM64)
# define CH_ARCHITECTURE_ARM64 NOT_IN_USE
#endif
#if !defined(CH_ARCHITECTURE_X86_32)
# define CH_ARCHITECTURE_X86_32 NOT_IN_USE
#endif

// Every supported platform and architecture is little endian.
#define CH_ENDIAN_LITTLE IN_USE
#define CH_ENDIAN_BIG NOT_IN_USE

/************************************************************************/
/**
 * Compiler specific keywords
 */
/************************************************************************/
#define NODISCARD [[nodiscard]]
#define RESTRICT __restrict
#define CH_PARAMETER_UNUSED(x) (void)(x)
#define CH_FALLTHROUGH [[fallthrough]]

// Windows headers may already define FORCEINLINE.
#if !defined(FORCEINLINE)
# if USING(CH_COMPILER_MSVC)
#   define FORCEINLINE __forceinline
# else
#   define FORCEINLINE __inline
# endif
#endif

#if USING(CH_COMPILER_MSVC)
# define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// Both forms give the same 8 byte aligned layout, which matters for structs written to disk.
#if USING(CH_COMPILER_MSVC)
# define MS_ALIGN(n) __declspec(align(n))
# define GCC_PACK(n)
# define GCC_ALIGN(n)
#else
# define MS_ALIGN(n)
# define GCC_PACK(n) __attribute__((aligned(n)))
# define GCC_ALIGN(n) __attribute__((aligned(n)))
#endif

/************************************************************************/
/**
 * Library export
 */
/************************************************************************/
#if USING(CH_PLATFORM_WIN32)
# define CH_DLL_EXPORT __declspec(dllexport)
# define CH_DLL_IMPORT __declspec(dllimport)
#else
# define CH_DLL_EXPORT __attribute__((visibility("default")))
# define CH_DLL_IMPORT
#endif

#if defined(CH_STATIC_LIB)
# define CH_UTILITY_EXPORT
#elif defined(CH_UTILITY_EXPORTS)
# define CH_UTILITY_EXPORT CH_DLL_EXPORT
#else
# define CH_UTILITY_EXPORT CH_DLL_IMPORT
#endif

#define CH_PLUGIN_EXPORT CH_DLL_EXPORT
#define CH_EXTERN extern "C"

/************************************************************************/
/**
 * Build type
 */
/************************************************************************/
// GCC and Clang never define _DEBUG, so a missing NDEBUG is what marks a debug build there.
#if !defined(NDEBUG) || defined(_DEBUG) || defined(DEBUG)
# define CH_DEBUG_MODE IN_USE
#else
# define CH_DEBUG_MODE NOT_IN_USE
#endif

#if USING(CH_DEBUG_MODE)
# define CH_ASSERT(x) assert(x)
# define CH_DEBUG_ONLY(x) x
#else
# define CH_ASSERT(x)
# define CH_DEBUG_ONLY(x)
#endif

#define CH_ENABLE_BACKTRACE CH_DEBUG_MODE
