/************************************************************************/
/**
 * @file chSTDStreams.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2026/10/01
 * @brief Engine names for the standard string streams.
 *
 * Kept out of chSTDHeaders.h because <sstream> pulls in the whole iostream and
 * locale machinery, so only the files that build text with streams pay for it.
 *
 * @bug No bug known.
 */
/************************************************************************/
#pragma once

#include <sstream>

namespace chEngineSDK {

template<typename T>
using BasicStringStream = std::basic_stringstream<T, std::char_traits<T>, std::allocator<T>>;

using StringStream = std::stringstream;

} // namespace chEngineSDK
