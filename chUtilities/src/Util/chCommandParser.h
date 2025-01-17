/************************************************************************/
/**
 * @file chCommandParser.h
 * @author AccelMR
 * @date 2022/08/27
 *
 *  Basic class for command line argument parsing to be used along the engine.
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {
class CH_UTILITY_EXPORT CommandParser {
 public:

  /**
   * @brief Singleton pattern to ensure one instance of CommandParser.
   * @return The instance of this CommandParser.
   */
  static CommandParser& 
  getInstance() {
    static CommandParser instance;
    return instance;
  }

  /**
   * @brief Deleted copy constructor.
   */
  CommandParser(CommandParser const&) = delete;

  /**
   * @brief Deleted assignment operator.
   */
  void 
  operator=(CommandParser const&) = delete;

  /**
   * @brief Parse the command line arguments.
   * @param argc The number of arguments.
   * @param argv The arguments.
   */
  void 
  parse(int32 argc, char** argv);

  /**
   * @brief Get the value of a parameter.
   * @param param The parameter name.
   * @param defaultValue The default value to return if the parameter is not found.
   * @return The value of the parameter.
   */
  String 
  getParam(const String& param, const String defaultValue = "");

  /**
   * @brief Check if a flag is set.
   * @param flag The flag to check.
   * @return True if the flag is set, otherwise false.
   */
  bool 
  isFlagSet(const String& flag);

 private:
  /**
   * @brief Private constructor for singleton.
   */
  CommandParser() = default;

  UnorderedMap<String, String> params;
  UnorderedSet<String> flags;
};
}
