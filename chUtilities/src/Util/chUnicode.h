/************************************************************************/
/**
 * @file chUnicode.h
 * @author AccelMR
 * @date 2022/06/23
 * @brief Conversions between UTF-8 and the other Unicode encodings.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Converts between UTF-8, which is what String holds, and UTF-16, UTF-32 or the platform
 * wide string, for files and OS calls that use another encoding. Invalid input (bad byte
 * sequences, lone surrogates, values above U+10FFFF) becomes U+FFFD instead of being
 * dropped, so the output length still matches what was read.
 *
 * Sample usage:
 * String text = UTF8::fromWide(L"Wide text");
 */
class CH_UTILITY_EXPORT UTF8
{
 public:
  static constexpr WCHAR32 REPLACEMENT_CHAR = 0xFFFD;

  NODISCARD static String
  fromWide(const WString& wideString);

  NODISCARD static WString
  toWide(const String& str);

  NODISCARD static String
  fromUTF16(const U16String& input);

  NODISCARD static U16String
  toUTF16(const String& input);

  NODISCARD static String
  fromUTF32(const U32String& input);

  NODISCARD static U32String
  toUTF32(const String& input);
};

} // namespace chEngineSDK
