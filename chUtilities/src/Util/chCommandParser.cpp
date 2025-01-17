/************************************************************************/
/**
 * @file chCommandParser.cpp
 * @author AccelMR
 * @date 2022/08/27
 *
 *  Basic class for command line argument parsing to be used along the engine.
 */
 /************************************************************************/

/************************************************************************/
/**
 *                        Includes
 */
 /************************************************************************/
#include "chCommandParser.h"


namespace chEngineSDK {
using std::transform;
using std::tolower;

/*
*/
void 
CommandParser::parse(int32 argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    String arg = argv[i];
    String lowerArg = arg;
    transform(lowerArg.begin(), lowerArg.end(), lowerArg.begin(),
      [](unsigned char c) { return tolower(c); });

    SIZE_T pos = lowerArg.find('=');
    if (pos == String::npos) {
      flags.insert(lowerArg.substr(1));
    }
    else {
      params[lowerArg.substr(1, pos - 1)] = arg.substr(pos + 1);
    }
  }
}

/*
*/
String 
CommandParser::getParam(const String& param, const String defaultValue /*= ""*/) {
  String lowerParam = param;
  transform(lowerParam.begin(), lowerParam.end(), lowerParam.begin(),
    [](unsigned char c) { return tolower(c); });

  auto it = params.find(lowerParam);
  return (it != params.end()) ? it->second : defaultValue;
}

/*
*/
bool 
CommandParser::isFlagSet(const String& flag) {
  String lowerFlag = flag;
  transform(lowerFlag.begin(), lowerFlag.end(), lowerFlag.begin(),
    [](unsigned char c) { return tolower(c); });

  return flags.find(lowerFlag) != flags.end();
}

}
