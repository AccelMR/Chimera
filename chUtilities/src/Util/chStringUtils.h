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
class CH_UTILITY_EXPORT StringUtils
{
 public:
  
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

  /** 
   *   Converts a value to a string.
   * 
   * @param value
   *    The value to be converted.
   * 
   * @return String
   *    The new created string.
   **/
  template<typename T>
  static String
  toString(const T& value);

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
};

/*
*/
// template<typename... Args>
// String
// StringUtils::format(const String& _format, Args... args) {
//   //static_assert((std::is_convertible_v<Args, const char*> && ...), "All arguments must be convertible to const char*");
//   //return std::format(_format.c_str(), std::forward<Args>(args)...);
//   std::stringstream ss;
//   ((ss << args), ...);
//   return ss.str();
//   //chExternals::SFormat(_format.c_str(), std::forward<Args>(args)...);
// }

/*
*/
template<typename T>
String
StringUtils::toString(const T& value) {
    std::ostringstream oss;
    if constexpr (std::is_floating_point_v<T>) {
        oss.precision(6); // Matches the default precision of std::to_string
        oss << std::fixed; // Use fixed-point notation
    }
    oss << value;
    return oss.str();
}

/*
*/
template<typename... Args>
String
StringUtils::format(const String& _format, Args&&... args) {
    Vector<std::string> arguments{toString(std::forward<Args>(args))...};
    String result = _format;
    std::regex placeholderRegex("\\{(\\d+)\\}");
    std::smatch match;

    while (std::regex_search(result, match, placeholderRegex)) {
        size_t index = std::stoi(match[1].str());
        if (index >= arguments.size()) {
            throw std::out_of_range("Placeholder index out of range");
        }

        // Replace the placeholder with the corresponding argument
        result.replace(match.position(0), match.length(0), arguments[index]);
    }

    return result;
}

}

