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

#include "chModule.h"

namespace chEngineSDK {
class CH_UTILITY_EXPORT CommandParser : public Module<CommandParser> {
 public:
  /**
   * @brief Destructor.
   */
  ~CommandParser() override = default;

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

  /**
   * @brief Get the number of arguments.
   * @return The number of arguments.
   */
  int32
  getArgc() const {
    return m_argc;
  }
  /**
   * @brief Get the arguments.
   * @return The arguments.
   */
  char**
  getArgv() const {
    return m_argv;
  }

  FORCEINLINE int32
  getParamAsInt(const String& param, int32 defaultValue = 0) {
    return std::stoi(getParam(param, std::to_string(defaultValue)));
  }

 private:
  friend class Module<CommandParser>;
  /**
   * @brief Private constructor for singleton.
   */
  CommandParser() = default;

  UnorderedMap<String, String> params;
  UnorderedSet<String> flags;

  int32  m_argc;
  char** m_argv;
};
}
