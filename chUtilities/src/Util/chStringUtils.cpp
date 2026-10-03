/************************************************************************/
/**
 * @file chStringUtils.cpp
 * @author AccelMR
 * @date 2022/06/23
 * @brief String helpers and the format function used by the logger.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chStringUtils.h"

#include <charconv>

namespace chEngineSDK {

namespace {
constexpr StringView kWhitespace = " \n\r\t\f\v";

// Enough for any number: a fixed double can need 309 digits, the point, the sign and
// FormatSpec::MAX_PRECISION decimals; a binary uint64 needs 64 digits.
constexpr SIZE_T kNumberBufferSize = 512;
constexpr SIZE_T kNumberSizeGuess = 16;

template<typename T>
StringView
writeInteger(ANSICHAR (&buffer)[kNumberBufferSize], T value, ANSICHAR type) noexcept
{
  int32 base = 10;
  if (type == 'x' || type == 'X') {
    base = 16;
  }
  else if (type == 'b') {
    base = 2;
  }
  else if (type == 'o') {
    base = 8;
  }

  const std::to_chars_result result =
      std::to_chars(buffer, buffer + kNumberBufferSize, value, base);
  if (type == 'X') {
    for (ANSICHAR* c = buffer; c != result.ptr; ++c) {
      if (*c >= 'a' && *c <= 'f') {
        *c = static_cast<ANSICHAR>(*c - 'a' + 'A');
      }
    }
  }
  return StringView(buffer, static_cast<SIZE_T>(result.ptr - buffer));
}

// Without type or precision a float gets the shortest text that reads back as the same
// value, like std::format.
StringView
writeFloat(ANSICHAR (&buffer)[kNumberBufferSize],
           double value,
           const FormatSpec& spec) noexcept
{
  ANSICHAR* const end = buffer + kNumberBufferSize;
  std::to_chars_result result;
  if (spec.type == '\0' && spec.precision < 0) {
    result = std::to_chars(buffer, end, value);
  }
  else {
    const std::chars_format format = spec.type == 'f'   ? std::chars_format::fixed
                                     : spec.type == 'e' ? std::chars_format::scientific
                                                        : std::chars_format::general;
    const int32 precision = spec.precision < 0 ? 6 : spec.precision;
    result = std::to_chars(buffer, end, value, format, precision);
  }

  if (result.ec != std::errc()) {
    return "?";
  }
  return StringView(buffer, static_cast<SIZE_T>(result.ptr - buffer));
}

void
appendPadded(String& output, StringView text, const FormatSpec& spec, bool isNumber)
{
  if (spec.width <= text.size()) {
    output.append(text);
    return;
  }

  const SIZE_T padding = spec.width - text.size();
  if (isNumber && spec.zeroPad && spec.align == '\0') {
    const SIZE_T signSize = text[0] == '-' ? 1 : 0;
    output.append(text.substr(0, signSize));
    output.append(padding, '0');
    output.append(text.substr(signSize));
    return;
  }

  const ANSICHAR align = spec.align != '\0' ? spec.align : (isNumber ? '>' : '<');
  const SIZE_T before = align == '>' ? padding : (align == '^' ? padding / 2 : 0);
  output.append(before, spec.fill);
  output.append(text);
  output.append(padding - before, spec.fill);
}
}

/*
 */
String
StringUtils::replaceAllChars(const String& toReplace, ANSICHAR from, ANSICHAR to)
{
  String output = toReplace;
  for (auto& c : output) {
    if (c == from) {
      c = to;
    }
  }
  return output;
}

/*
 */
String
StringUtils::replaceAllSubStr(const String& toReplace, const String& from, const String& to)
{
  // An empty 'from' matches everywhere and would never stop.
  if (from.empty()) {
    return toReplace;
  }

  String output;
  output.reserve(toReplace.size());

  SIZE_T lastPos = 0;
  SIZE_T pos = 0;
  while ((pos = toReplace.find(from, lastPos)) != String::npos) {
    output.append(toReplace, lastPos, pos - lastPos);
    output.append(to);
    lastPos = pos + from.size();
  }
  output.append(toReplace, lastPos, String::npos);

  return output;
}

/*
 */
Vector<String>
StringUtils::splitString(const String& toSplit, ANSICHAR separator)
{
  Vector<String> ret;
  SIZE_T start = 0;

  while (start <= toSplit.size()) {
    SIZE_T end = toSplit.find(separator, start);
    if (end == String::npos) {
      end = toSplit.size();
    }

    if (end > start) {
      ret.emplace_back(toSplit, start, end - start);
    }
    start = end + 1;
  }

  return ret;
}

/*
 */
Vector<String>
StringUtils::splitString(const String& toSplit, const String& separator)
{
  Vector<String> ret;

  // An empty separator would never move forward.
  if (separator.empty()) {
    if (!toSplit.empty()) {
      ret.push_back(toSplit);
    }
    return ret;
  }

  SIZE_T start = 0;
  while (start <= toSplit.size()) {
    SIZE_T end = toSplit.find(separator, start);
    if (end == String::npos) {
      end = toSplit.size();
    }

    if (end > start) {
      ret.emplace_back(toSplit, start, end - start);
    }
    start = end + separator.size();
  }

  return ret;
}

/*
 */
String
StringUtils::join(const Vector<String>& toJoin, const String& separator)
{
  if (toJoin.empty()) {
    return String();
  }

  SIZE_T size = separator.size() * (toJoin.size() - 1);
  for (const String& word : toJoin) {
    size += word.size();
  }

  String out;
  out.reserve(size);
  out += toJoin[0];
  for (SIZE_T i = 1; i < toJoin.size(); ++i) {
    out += separator;
    out += toJoin[i];
  }

  return out;
}

/*
 */
bool
StringUtils::containsIgnoreCase(StringView text, StringView search) noexcept
{
  auto toLowerASCII = [](ANSICHAR c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<ANSICHAR>(c - 'A' + 'a') : c;
  };

  if (search.size() > text.size()) {
    return false;
  }

  const SIZE_T lastStart = text.size() - search.size();
  for (SIZE_T start = 0; start <= lastStart; ++start) {
    SIZE_T i = 0;
    while (i < search.size() && toLowerASCII(text[start + i]) == toLowerASCII(search[i])) {
      ++i;
    }
    if (i == search.size()) {
      return true;
    }
  }
  return false;
}

/*
 */
String
StringUtils::toLower(const String& str)
{
  String ret = str;
  for (auto& c : ret) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<ANSICHAR>(c - 'A' + 'a');
    }
  }
  return ret;
}

/*
 */
String
StringUtils::toUpper(const String& str)
{
  String ret = str;
  for (auto& c : ret) {
    if (c >= 'a' && c <= 'z') {
      c = static_cast<ANSICHAR>(c - 'a' + 'A');
    }
  }
  return ret;
}

/*
 */
String
StringUtils::lTrim(const String& str)
{
  const SIZE_T start = str.find_first_not_of(kWhitespace);
  return (start == String::npos) ? String() : str.substr(start);
}

/*
 */
String
StringUtils::rTrim(const String& str)
{
  const SIZE_T end = str.find_last_not_of(kWhitespace);
  return (end == String::npos) ? String() : str.substr(0, end + 1);
}

/*
 */
String
StringUtils::trim(const String& str)
{
  const SIZE_T start = str.find_first_not_of(kWhitespace);
  if (start == String::npos) {
    return String();
  }

  const SIZE_T end = str.find_last_not_of(kWhitespace);
  return str.substr(start, end - start + 1);
}


/*
 */
String
StringUtils::formatArgs(StringView format, const FormatArg* args, SIZE_T count)
{
  SIZE_T size = format.size();
  for (SIZE_T i = 0; i < count; ++i) {
    size += args[i].type == FormatArgType::Text ? args[i].text.size() : kNumberSizeGuess;
  }

  String result;
  result.reserve(size);
  ANSICHAR numberBuffer[kNumberBufferSize];

  // FormatString already checked the placeholders at compile time. Anything unexpected is
  // still written as plain text instead of failing.
  SIZE_T automaticIndex = 0;
  SIZE_T i = 0;
  while (i < format.size()) {
    const SIZE_T brace = format.find_first_of("{}", i);
    if (brace == StringView::npos) {
      result.append(format.substr(i));
      break;
    }

    result.append(format.substr(i, brace - i));
    i = brace;

    const bool doubled = i + 1 < format.size() && format[i + 1] == format[i];
    if (format[i] == '}' || doubled) {
      result += format[i];
      i += doubled ? 2 : 1;
      continue;
    }

    const SIZE_T close = format.find('}', i + 1);
    if (close == StringView::npos) {
      result.append(format.substr(i));
      break;
    }

    const StringView content = format.substr(i + 1, close - i - 1);
    const SIZE_T colon = content.find(':');
    const StringView indexText = content.substr(0, colon);

    SIZE_T index = 0;
    bool valid = true;
    if (indexText.empty()) {
      index = automaticIndex++;
    }
    else {
      for (const ANSICHAR digit : indexText) {
        if (digit < '0' || digit > '9') {
          valid = false;
          break;
        }
        index = index * 10 + static_cast<SIZE_T>(digit - '0');
      }
    }

    FormatSpec spec;
    if (colon != StringView::npos) {
      valid = valid && FormatSpec::parse(content.substr(colon + 1), spec);
    }

    if (!valid || index >= count || !spec.fits(args[index].type)) {
      result.append(format.substr(i, close - i + 1));
      i = close + 1;
      continue;
    }

    const FormatArg& arg = args[index];
    StringView text;
    bool isNumber = true;
    switch (arg.type) {
    case FormatArgType::Text:
      text = arg.text;
      isNumber = false;
      break;
    case FormatArgType::Char:
      numberBuffer[0] = arg.charValue;
      text = StringView(numberBuffer, 1);
      isNumber = false;
      break;
    case FormatArgType::Bool:
      text = arg.boolValue ? "true" : "false";
      isNumber = false;
      break;
    case FormatArgType::Signed:
      text = writeInteger(numberBuffer, arg.signedValue, spec.type);
      break;
    case FormatArgType::Unsigned:
      text = writeInteger(numberBuffer, arg.unsignedValue, spec.type);
      break;
    case FormatArgType::Float:
      text = writeFloat(numberBuffer, arg.floatValue, spec);
      break;
    }
    appendPadded(result, text, spec, isNumber);

    i = close + 1;
  }

  return result;
}

} // namespace chEngineSDK
