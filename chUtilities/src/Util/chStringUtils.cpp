/************************************************************************/
/**
 * @file chStringUtils.cpp
 * @author AccelMR
 * @date 2022/06/23
 * @brief String utilities file.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chStringUtils.h"

namespace chEngineSDK{
using std::transform;
using std::tolower;

/*
*/
const String StringUtils::WHITESPACE = " \n\r\t\f\v";

/*
*/
String
StringUtils::replaceAllChars(const String& toReplace, const char& from, const char& to) {
  String output = toReplace;
  for(auto& c: output) {
    if (c == from) {
      c = to; 
    }
  }
  return output;
}

/*
*/
String
StringUtils::replaceAllSubStr(const String& toReplace, 
                              const String& from, 
                              const String& to) {
  String output = toReplace;
  SIZE_T startPos = 0;

  while((startPos = output.find(from, startPos)) != std::string::npos) {
    output.replace(startPos, from.length(), to);
    startPos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return output;
}

/*
*/
Vector<String>
StringUtils::splitString(const String& toSplit, const char& separator) {
  Vector<String> ret;
  String tempWord;
  for (auto& c: toSplit) {
    if (c == separator) {
      if (tempWord.empty()) { continue; }
      ret.push_back(tempWord);
      tempWord.clear();
      continue;
    }
    tempWord += c;
  }

  if (!tempWord.empty()) {
    ret.push_back(tempWord);
  }

  return ret;
}

/*
*/
Vector<String>
StringUtils::splitString(const String& toSplit, const String& separator) {  
  Vector<String> ret = {};
  SIZE_T prevPos = 0;
  SIZE_T currentPos = 0;
  while ((currentPos = toSplit.find(separator, currentPos)) != std::string::npos) {
    String tmp = toSplit.substr(prevPos, currentPos - prevPos);
    ret.push_back(tmp);
    currentPos += separator.length();
    prevPos = currentPos;
  }

  String tmp(toSplit.substr(prevPos, toSplit.length()));
  if (!tmp.empty()) {
    ret.push_back(tmp);
  }
  return ret;
}

/*
*/
String
StringUtils::join(const Vector<String>& toJoin, const String& separator) {
  String out;
  for (auto& word : toJoin) {
    out += word;
    out += separator;
  }

  out.erase(out.end() - separator.length(), out.end());
  return out;
}

/*
*/
String
StringUtils::toLower(const String& str) {
  String ret = str;
  for (auto& c : ret) {
    c = std::tolower(c, std::locale());
  }
  return ret;
}

/*
*/
String
StringUtils::toUpper( const String &str ) {
  String ret = str;
  for (auto& c : ret) {
    c = std::toupper(c, std::locale());
  }
  return ret;
}

/*
*/
String
StringUtils::lTrim(const String& str) {
  SIZE_T start = str.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : str.substr(start);
}

/*
*/
String
StringUtils::rTrim(const String& str) {
  SIZE_T end = str.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

/*
*/
String
StringUtils::trim(const String& str) {
  return rTrim(lTrim(str));
}

}


