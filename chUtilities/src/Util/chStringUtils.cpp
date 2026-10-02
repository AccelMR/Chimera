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

namespace chEngineSDK {

namespace {
constexpr StringView kWhitespace = " \n\r\t\f\v";
}

/*
 */
String
chString::replaceAllChars(const String& toReplace, ANSICHAR from, ANSICHAR to)
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
chString::replaceAllSubStr(const String& toReplace, const String& from, const String& to)
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
chString::splitString(const String& toSplit, ANSICHAR separator)
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
chString::splitString(const String& toSplit, const String& separator)
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
chString::join(const Vector<String>& toJoin, const String& separator)
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
String
chString::toLower(const String& str)
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
chString::toUpper(const String& str)
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
chString::lTrim(const String& str)
{
  const SIZE_T start = str.find_first_not_of(kWhitespace);
  return (start == String::npos) ? String() : str.substr(start);
}

/*
 */
String
chString::rTrim(const String& str)
{
  const SIZE_T end = str.find_last_not_of(kWhitespace);
  return (end == String::npos) ? String() : str.substr(0, end + 1);
}

/*
 */
String
chString::trim(const String& str)
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
chString::formatArgs(StringView format, const StringView* args, SIZE_T count)
{
  SIZE_T size = format.size();
  for (SIZE_T i = 0; i < count; ++i) {
    size += args[i].size();
  }

  String result;
  result.reserve(size);

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

    SIZE_T index = 0;
    bool valid = true;
    if (close == i + 1) {
      index = automaticIndex++;
    }
    else {
      for (SIZE_T d = i + 1; d < close; ++d) {
        if (format[d] < '0' || format[d] > '9') {
          valid = false;
          break;
        }
        index = index * 10 + static_cast<SIZE_T>(format[d] - '0');
      }
    }

    if (valid && index < count) {
      result.append(args[index]);
    }
    else {
      result.append(format.substr(i, close - i + 1));
    }

    i = close + 1;
  }

  return result;
}

} // namespace chEngineSDK
