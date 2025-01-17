/************************************************************************/
/**
 * @file chUnicode.h
 * @author AccelMR
 * @date 2022/06/23
 * @brief Utilities Unicode to translate between UTF-8 and other encodings.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Entirely static class that will wrap any Unicode helper function.
 *
 * Sample usage:
 * String newString = UnicodeUtil::fromWide(L"WideString to be converted to a string");
 *
 */
class CH_UTILITY_EXPORT UTF8
{
 public:

 /** 
  *   Creates a new string from a wide character array to a UTF-8 encoding.
  * 
  * @param wideString
  *   The wide string to transform.
  * 
  * @return String
  *   The new string created from a wide char.
  **/
  static String
  fromWide(const WString& wideString);

  /** 
   *   Creates a new Wide string based on a given string.
   * 
   * @param str
   *  String to be converted.
   * 
   * @return WString
   * New Wide string created.
   **/
  static WString
  toWide(const String& str);

  /**
   *   Converts from an UTF-16 encoding into UTF-8.
   * 
   * @param input 
   *    String encoded as UTF-16.
   * 
   * @return String
   *    UTF-8 encoded string.
   */
  static String
  fromUTF16(const U16String& input);

  /**
   *  Converts from an UTF-8 encoding into UTF-16.
   * 
   * @param input 
   *    String encoded as UTF-8.
   * 
   * @return  U16String 
   *    UTF-16 encoded string.
   */
  static U16String
  toUTF16(const String& input);

  /**
   *    Converts from an UTF-32 encoding into UTF-8.
   * 
   * @param input 
   *    String encoded as UTF-32.
   * 
   * @return String
   *    UTF-8 encoded string.
   */
  static String
  fromUTF32(const U32String& input);

  /**
   *    Converts from an UTF-8 encoding into UTF-32.
   * @param input 
   *    String encoded as UTF-8.
   * 
   * @return  input 
   *    UTF-32 encoded string.
   */
  static U32String
  toUTF32(const String& input);
 
};
}

