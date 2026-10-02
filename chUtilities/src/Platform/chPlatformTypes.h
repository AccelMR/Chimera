/************************************************************************/
/**
 * @file chPlatformTypes.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Define basic platform types.
 *
 * Define the basic platform type variable. For porting, this might
 * be the place to start.
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
#include <cstdint>
#include <cstddef>
#include "chPlatformDefines.h"

/**
 * @brief Here we define a "intermediate" language. If anything changes it just changes here.
 */
namespace chEngineSDK {
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

/************************************************************************/
/**
 * Basic unsigned types
 */
 /************************************************************************/
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;


/************************************************************************/
/**
 * Basic signed types
 */
 /************************************************************************/
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

/************************************************************************/
/**
 * Character types
 */
 /************************************************************************/
using WCHAR16 = char16_t;
using WCHAR32 = char32_t;
using ANSICHAR = char;  //ANSI character type
using UNICHAR = WCHAR16;  //UNICODE character type
using WIDECHAR = wchar_t; //16 bits on Windows, 32 bits on Linux
using unchar = unsigned char;

/************************************************************************/
/**
 * NULL data type
 */
 /************************************************************************/
using TYPE_OF_NULL = int32;

/************************************************************************/
/**
* SIZE_T is an architecture dependent data type
*/
/************************************************************************/
using SIZE_T = std::size_t;

/************************************************************************/
/**
 * Size checks. Files and memory layouts rely on these sizes, so the build stops if a
 * platform or a change to an alias breaks them.
 */
/************************************************************************/
static_assert(sizeof(uint8) == 1, "uint8 must be 1 byte.");
static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes.");
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes.");
static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes.");
static_assert(sizeof(int8) == 1, "int8 must be 1 byte.");
static_assert(sizeof(int16) == 2, "int16 must be 2 bytes.");
static_assert(sizeof(int32) == 4, "int32 must be 4 bytes.");
static_assert(sizeof(int64) == 8, "int64 must be 8 bytes.");

static_assert(sizeof(ANSICHAR) == 1, "ANSICHAR must be 1 byte.");
static_assert(sizeof(unchar) == 1, "unchar must be 1 byte.");
static_assert(sizeof(WCHAR16) == 2, "WCHAR16 must be 2 bytes.");
static_assert(sizeof(WCHAR32) == 4, "WCHAR32 must be 4 bytes.");
#if USING(CH_PLATFORM_WIN32)
static_assert(sizeof(WIDECHAR) == 2, "WIDECHAR must be 2 bytes on Windows.");
#else
static_assert(sizeof(WIDECHAR) == 4, "WIDECHAR must be 4 bytes on Linux and macOS.");
#endif

static_assert(sizeof(float) == 4, "float must be 4 bytes.");
static_assert(sizeof(double) == 8, "double must be 8 bytes.");
static_assert(sizeof(SIZE_T) == sizeof(void*), "SIZE_T must be as wide as a pointer.");
}