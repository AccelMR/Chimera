/************************************************************************/
/**
 * @file chUnicode.cpp
 * @author AccelMR
 * @date 2022/06/23
 * @brief Conversions between UTF-8 and the other Unicode encodings.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chUnicode.h"

namespace chEngineSDK {

namespace {

NODISCARD constexpr bool
isSurrogate(WCHAR32 c) noexcept
{
  return c >= 0xD800 && c <= 0xDFFF;
}

NODISCARD constexpr WCHAR32
validOrReplacement(WCHAR32 c) noexcept
{
  return (c > 0x10FFFF || isSurrogate(c)) ? UTF8::REPLACEMENT_CHAR : c;
}

/**
 * Reads one character and moves it past it. A bad sequence gives U+FFFD and only skips
 * the bytes that were part of it, so the next valid character is not lost.
 */
template<typename Iterator>
NODISCARD WCHAR32
decodeUTF8(Iterator& it, Iterator end) noexcept
{
  const uint8 first = static_cast<uint8>(*it);
  ++it;
  if (first < 0x80) {
    return first;
  }

  uint32 continuationBytes = 0;
  WCHAR32 minValue = 0;
  WCHAR32 output = 0;
  if ((first & 0xE0) == 0xC0) {
    continuationBytes = 1;
    minValue = 0x80;
    output = first & 0x1F;
  }
  else if ((first & 0xF0) == 0xE0) {
    continuationBytes = 2;
    minValue = 0x800;
    output = first & 0x0F;
  }
  else if ((first & 0xF8) == 0xF0) {
    continuationBytes = 3;
    minValue = 0x10000;
    output = first & 0x07;
  }
  else {
    // A continuation byte without a lead byte, or the 5 and 6 byte forms that UTF-8
    // no longer allows.
    return UTF8::REPLACEMENT_CHAR;
  }

  for (uint32 i = 0; i < continuationBytes; ++i) {
    if (it == end) {
      return UTF8::REPLACEMENT_CHAR;
    }
    const uint8 byte = static_cast<uint8>(*it);
    if ((byte & 0xC0) != 0x80) {
      return UTF8::REPLACEMENT_CHAR;
    }
    output = (output << 6) | (byte & 0x3F);
    ++it;
  }

  // A value written with more bytes than it needs is rejected, so the same character
  // always has a single encoding.
  if (output < minValue) {
    return UTF8::REPLACEMENT_CHAR;
  }
  return validOrReplacement(output);
}

template<typename Container>
void
encodeUTF8(WCHAR32 c, Container& output)
{
  c = validOrReplacement(c);

  uint8 bytes[4];
  uint32 numBytes = 0;
  if (c < 0x80) {
    bytes[0] = static_cast<uint8>(c);
    numBytes = 1;
  }
  else if (c < 0x800) {
    bytes[0] = static_cast<uint8>(0xC0 | (c >> 6));
    bytes[1] = static_cast<uint8>(0x80 | (c & 0x3F));
    numBytes = 2;
  }
  else if (c < 0x10000) {
    bytes[0] = static_cast<uint8>(0xE0 | (c >> 12));
    bytes[1] = static_cast<uint8>(0x80 | ((c >> 6) & 0x3F));
    bytes[2] = static_cast<uint8>(0x80 | (c & 0x3F));
    numBytes = 3;
  }
  else {
    bytes[0] = static_cast<uint8>(0xF0 | (c >> 18));
    bytes[1] = static_cast<uint8>(0x80 | ((c >> 12) & 0x3F));
    bytes[2] = static_cast<uint8>(0x80 | ((c >> 6) & 0x3F));
    bytes[3] = static_cast<uint8>(0x80 | (c & 0x3F));
    numBytes = 4;
  }

  for (uint32 i = 0; i < numBytes; ++i) {
    output.push_back(static_cast<ANSICHAR>(bytes[i]));
  }
}

/**
 * Reads one character and moves it past it. A lone surrogate gives U+FFFD; when a high
 * surrogate is not followed by a low one, the next unit is left to be read on its own.
 */
template<typename Iterator>
NODISCARD WCHAR32
decodeUTF16(Iterator& it, Iterator end) noexcept
{
  const WCHAR32 first = static_cast<uint16>(*it);
  ++it;
  if (!isSurrogate(first)) {
    return first;
  }
  if (first >= 0xDC00 || it == end) {
    return UTF8::REPLACEMENT_CHAR;
  }

  const WCHAR32 second = static_cast<uint16>(*it);
  if (second < 0xDC00 || second > 0xDFFF) {
    return UTF8::REPLACEMENT_CHAR;
  }
  ++it;
  return ((first - 0xD800) << 10) + (second - 0xDC00) + 0x10000;
}

template<typename Container>
void
encodeUTF16(WCHAR32 c, Container& output)
{
  using Unit = typename Container::value_type;

  c = validOrReplacement(c);
  if (c < 0x10000) {
    output.push_back(static_cast<Unit>(c));
    return;
  }

  c -= 0x10000;
  output.push_back(static_cast<Unit>((c >> 10) + 0xD800));
  output.push_back(static_cast<Unit>((c & 0x3FF) + 0xDC00));
}

} // namespace

/*
 */
String
UTF8::fromWide(const WString& wideString)
{
  String output;
  output.reserve(wideString.size());

  auto it = wideString.begin();
  while (it != wideString.end()) {
    if constexpr (sizeof(WIDECHAR) == 4) {
      encodeUTF8(static_cast<WCHAR32>(*it), output);
      ++it;
    }
    else {
      encodeUTF8(decodeUTF16(it, wideString.end()), output);
    }
  }

  return output;
}

/*
 */
WString
UTF8::toWide(const String& str)
{
  // A wide string never needs more units than the UTF-8 text has bytes.
  WString output;
  output.reserve(str.size());

  auto it = str.begin();
  while (it != str.end()) {
    const WCHAR32 c = decodeUTF8(it, str.end());
    if constexpr (sizeof(WIDECHAR) == 4) {
      output.push_back(static_cast<WIDECHAR>(c));
    }
    else {
      encodeUTF16(c, output);
    }
  }

  return output;
}

/*
 */
String
UTF8::fromUTF16(const U16String& input)
{
  String output;
  output.reserve(input.size());

  auto it = input.begin();
  while (it != input.end()) {
    encodeUTF8(decodeUTF16(it, input.end()), output);
  }

  return output;
}

/*
 */
U16String
UTF8::toUTF16(const String& input)
{
  U16String output;
  output.reserve(input.size());

  auto it = input.begin();
  while (it != input.end()) {
    encodeUTF16(decodeUTF8(it, input.end()), output);
  }

  return output;
}

/*
 */
String
UTF8::fromUTF32(const U32String& input)
{
  String output;
  output.reserve(input.size());

  for (const WCHAR32 c : input) {
    encodeUTF8(c, output);
  }

  return output;
}

/*
 */
U32String
UTF8::toUTF32(const String& input)
{
  U32String output;
  output.reserve(input.size());

  auto it = input.begin();
  while (it != input.end()) {
    output.push_back(decodeUTF8(it, input.end()));
  }

  return output;
}

} // namespace chEngineSDK
