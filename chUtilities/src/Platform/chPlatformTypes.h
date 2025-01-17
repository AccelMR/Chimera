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
}