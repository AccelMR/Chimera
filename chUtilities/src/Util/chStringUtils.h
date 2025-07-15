/************************************************************************/
/**
 * @file chStringUtils.h
 * @author AccelMR
 * @date 2022/06/23
 * @brief String utilities file.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <format>
#include <regex>

namespace chEngineSDK{
/*
 * Description:
 *     Static class that will contain helper functions to use on Strings.
 *
 * Sample usage:
 * String replaced = StringUtils::replaceAllChars("A test string", " ", "");
 *
 * // Where replaced = "Ateststring";
 *"%{wks.location}/
 */
class CH_UTILITY_EXPORT chString
{
 public:

  static String
  fromInt32(int32 value) {
    return std::to_string(value);
  }

  /**
   *   Creates a new string by replacing every character that matches 'from' with 'to'.
   *
   * @param toReplace
   *   The string from where the new string will be created.
   *
   * @param from
   *   Character to be evaluated to change.
   *
   * @param to
   *   Character that is going to be placed instead of 'from'.
   *
   * @return String
   *   New string created by this replace.
   **/
  static String
  replaceAllChars(const String& toReplace, const char& from, const char& to);

  /**
   *   Creates a new string by replacing every substring found into another given substring.
   *
   * @param toReplace
   *   The string from where the new string will be created.
   *
   * @param from
   *    Substring that is going to be looked to be changed.
   *
   * @param to
   *   Substring that will replace the 'from' substring.
   *
   * @return String
   *  New string created from this replacing.
   **/
  static String
  replaceAllSubStr(const String& toReplace, const String& from, const String& to);

  /**
   *   Creates a vector by splitting a given string into multiples string if separator
   *  is found in string.
   *
   * @param separator.
   *   The character to be compared.
   *
   * @return Vector<String>
   **/
  static Vector<String>
  splitString(const String& toSplit, const char& separator);

  /**
   *   Creates a vector by splitting a given string into multiples string if separator
   *  is found in string.
   *
   * @param separator
   *   The string to be compared.
   *
   * @return Vector<String>
   **/
  static Vector<String>
  splitString(const String& toSplit, const String& separator);

  /**
   *   Constructs a single string by a vector of Strings by adding a character between each string.
   *
   * @param toJoin
   *    The vector to be merged.
   *
   * @param separator
   *    The String that goes between each string.
   *
   * @return
   *   New constructed string from a list.
   **/
  static String
  join(const Vector<String>& toJoin,const String& separator);

  /**
   *   Formats a string given with its respective arguments.
   *
   * @param format
   *    String to be modified.
   *
   * @return
   *  New string created from a formatted string.
   **/
  template<typename... Args>
  static String
  format(const String& _format, Args&&... args);

  template<typename Arg>
  static String
  format(const String& _format, Arg&& arg);

  FORCEINLINE static String
  format(const String& _format) {
      return _format;
  }

  template<typename T>
  static String
  toString(T&& value);

  /**
   *   Creates a new all characters in a string to lower case.
   *
   * @param str
   *    The string to take reference.
   *
   * @return String
   *    The new created string
   **/
   static String
   toLower(const String& str);

   /**
   *   Copies a string to a buffer of ANSICHAR.
   * @param dest
   *    The destination buffer to copy the string.
   * @param src
   *    The source string to copy.
   * @param size
   *    The size of the destination buffer. If 0, it will copy the entire string.
   **/
   FORCEINLINE
   static void
   copyToANSI(ANSICHAR* dest, const String& src, SIZE_T size = 0) {
      if (size == 0) {
        size = src.size();
      }
      std::memcpy(dest, src.c_str(), size * sizeof(ANSICHAR));
      dest[size] = '\0'; // Ensure null-termination
   }

  /**
   *   Creates a new all characters in a string to upper case.
   *
   * @param str
   *    The string to take reference.
   *
   * @return String
   *    The new created string
   **/
   static String
   toUpper(const String& str);

   /**
    *   Left trims a string of any whitespace.
    *
    * @param str
    *    The string to take reference.
    **/
   static String
   lTrim(const String& str);

   /**
    *   Right trims a string of any whitespace.
    *
    * @param str
    *    The string to take reference.
    **/
   static String
   rTrim(const String& str);

   /**
    *   Trims a string of any whitespace for both right and left part.
    *
    * @param str
    *    The string to take reference.
    **/
   static String
   trim(const String& str);


 public:
  static const String WHITESPACE;
  static const String EMPTY;
};

/*
*/
template <typename T>
String
chString::toString(T&& value)
{
  // Para strings, permitir movimiento directo
  if constexpr (std::is_same_v<std::decay_t<T>, String> ||
                std::is_same_v<std::decay_t<T>, std::string>) {
    if constexpr (std::is_lvalue_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>) {
      return value;
    }
    else if constexpr (std::is_lvalue_reference_v<T>) {
      return value;
    }
    else {
      return std::move(value);
    }
  }
  else if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
    return std::to_string(static_cast<std::decay_t<T>>(value));
  }
  else {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }
}

template <typename... Args>
String
chString::format(const String& _format, Args&&... args) {
  if (_format.find('{') == String::npos) {
    return _format;
  }

  Array<String, sizeof...(Args)> arguments{toString(std::forward<Args>(args))...};

  SIZE_T estimatedSize = _format.size();
  for (const auto& arg : arguments) {
    estimatedSize += arg.size();
  }

  estimatedSize = std::min(estimatedSize, _format.size() * 3);

  String result;
  result.reserve(estimatedSize);

  SIZE_T lastPos = 0;
  SIZE_T pos = 0;

  while ((pos = _format.find('{', lastPos)) != String::npos) {
    result.append(_format, lastPos, pos - lastPos);

    SIZE_T closePos = _format.find('}', pos);
    if (closePos == String::npos) {
      result.append(_format, pos, String::npos);
      break;
    }

    if (closePos == pos + 1) {
      CH_EXCEPT(InvalidArgumentException, "Empty placeholder in format string");
    }

    SIZE_T index = 0;
    bool validIndex = true;
    for (SIZE_T i = pos + 1; i < closePos; ++i) {
      char c = _format[i];
      if (c >= '0' && c <= '9') {
        index = index * 10 + (c - '0');
      }
      else {
        validIndex = false;
        break;
      }
    }

    if (!validIndex) {
      result.append(_format, pos, closePos - pos + 1);
    }
    else if (index >= arguments.size()) {
      throw std::out_of_range("Placeholder index out of range");
    }
    else {
      result.append(arguments[index]);
    }

    lastPos = closePos + 1;
  }

  if (lastPos < _format.size()) {
    result.append(_format, lastPos, String::npos);
  }

  return result;
}

template<typename Arg>
String
chString::format(const String& _format, Arg&& arg) {
  SIZE_T pos = _format.find("{0}");
  if (pos == String::npos) {
    return _format;
  }

  String argStr = toString(std::forward<Arg>(arg));
  String result;
  result.reserve(_format.size() + argStr.size());

  result.append(_format, 0, pos);
  result.append(argStr);
  result.append(_format, pos + 3, String::npos);

  return result;
}
}
