/************************************************************************/
/**
 * @file chConsoleVariable.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief Named settings that config files and the command line can change.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Who set the value of a console variable, from lowest to highest priority. A value only
 * replaces one set by the same or a lower source.
 */
enum class ConsoleVariableSource : uint8
{
  Default,
  EngineConfig,
  ProjectConfig,
  UserConfig,
  CommandLine,
  Code,
  Console
};

/**
 * Base of every ConsoleVariable, so the registry can find, list and set them by name
 * without knowing their type.
 */
class CH_UTILITY_EXPORT IConsoleVariable
{
 public:
  IConsoleVariable(const IConsoleVariable&) = delete;

  IConsoleVariable&
  operator=(const IConsoleVariable&) = delete;

  /**
   * Parses text into the value. Returns false, and keeps the value, when the text is not
   * valid for the type or when the current value came from a higher source.
   */
  bool
  setFromString(StringView text, ConsoleVariableSource source);

  NODISCARD virtual String
  toString() const = 0;

  NODISCARD FORCEINLINE const String&
  getName() const noexcept
  {
    return m_name;
  }

  NODISCARD FORCEINLINE const String&
  getHelp() const noexcept
  {
    return m_help;
  }

  NODISCARD FORCEINLINE const String&
  getCommandLineAlias() const noexcept
  {
    return m_commandLineAlias;
  }

  NODISCARD FORCEINLINE ConsoleVariableSource
  getSource() const noexcept
  {
    return m_source;
  }

 protected:
  IConsoleVariable(StringView name, StringView help, StringView commandLineAlias);

  virtual ~IConsoleVariable();

  /**
   * Called by the derived constructor once its value exists, because registering may
   * already set it from the config files and the command line.
   */
  void
  registerVariable();

  NODISCARD FORCEINLINE bool
  canBeSetBy(ConsoleVariableSource source) const noexcept
  {
    return source >= m_source;
  }

  FORCEINLINE void
  setSource(ConsoleVariableSource source) noexcept
  {
    m_source = source;
  }

  virtual bool
  parse(StringView text) = 0;

  /**
   * bool accepts true/false, 1/0, yes/no and on/off, in any case.
   */
  static bool
  parseValue(StringView text, bool& value);

  static bool
  parseValue(StringView text, int32& value);

  static bool
  parseValue(StringView text, float& value);

  static bool
  parseValue(StringView text, String& value);

  NODISCARD static String
  valueToString(bool value);

  NODISCARD static String
  valueToString(int32 value);

  NODISCARD static String
  valueToString(float value);

  NODISCARD static String
  valueToString(const String& value);

 private:
  String m_name;
  String m_help;
  String m_commandLineAlias;
  ConsoleVariableSource m_source = ConsoleVariableSource::Default;
};

/**
 * A setting with a name, declared as a global or static where it is used, so adding a
 * setting needs no change anywhere else. Its value comes, from lowest to highest priority,
 * from the default given here, the config files ("Window.Width" is Width= in [Window]),
 * the command line ("-Window.Width=1280", or "-Window.VSync" for true) and code.
 *
 * get() reads a plain member, so it is cheap enough for every frame. Values are only set
 * on the main thread.
 *
 * Sample usage:
 * ConsoleVariable<int32> CVarWindowWidth("Window.Width", 2560, "Window width.", "Width");
 * CVarWindowWidth.get();
 */
template<typename T>
  requires(std::is_same_v<T, bool> || std::is_same_v<T, int32> || std::is_same_v<T, float> ||
           std::is_same_v<T, String>)
class ConsoleVariable final : public IConsoleVariable
{
 public:
  /**
   * commandLineAlias is a second, shorter name accepted on the command line and by find.
   */
  ConsoleVariable(StringView name,
                  T defaultValue,
                  StringView help,
                  StringView commandLineAlias = StringView())
   : IConsoleVariable(name, help, commandLineAlias),
     m_value(defaultValue),
     m_defaultValue(std::move(defaultValue))
  {
    registerVariable();
  }

  ~ConsoleVariable() override = default;

  NODISCARD FORCEINLINE const T&
  get() const noexcept
  {
    return m_value;
  }

  NODISCARD FORCEINLINE const T&
  getDefault() const noexcept
  {
    return m_defaultValue;
  }

  /**
   * Returns false, and keeps the value, when the current value came from a higher source.
   */
  bool
  set(T value, ConsoleVariableSource source = ConsoleVariableSource::Code)
  {
    if (!canBeSetBy(source)) {
      return false;
    }
    m_value = std::move(value);
    setSource(source);
    return true;
  }

  NODISCARD String
  toString() const override
  {
    return valueToString(m_value);
  }

 protected:
  bool
  parse(StringView text) override
  {
    T value{};
    if (!parseValue(text, value)) {
      return false;
    }
    m_value = std::move(value);
    return true;
  }

 private:
  T m_value;
  T m_defaultValue;
};

/**
 * Registry of every ConsoleVariable that exists, also those of plugins. Values read from
 * config files are kept by name, so a variable registered later (when its plugin loads)
 * still gets them.
 */
class CH_UTILITY_EXPORT ConsoleVariables
{
 public:
  /**
   * Looks by name or command line alias, ignoring case. Returns nullptr when none matches.
   */
  NODISCARD static IConsoleVariable*
  find(StringView name);

  /**
   * Sorted by name.
   */
  NODISCARD static Vector<IConsoleVariable*>
  getAll();

  /**
   * Keeps a value read from a config file. A value from a lower source than the one kept
   * for the same name is ignored. Before applyStartupValues it is only kept; after it, it
   * is also given to the variable.
   */
  static void
  setStartupValue(StringView name, StringView value, ConsoleVariableSource source);

  /**
   * Gives every variable its config value and its command line option. Variables
   * registered after this get them when they register.
   */
  static void
  applyStartupValues();

 private:
  friend class IConsoleVariable;

  static void
  add(IConsoleVariable& variable);

  static void
  remove(IConsoleVariable& variable);
};

} // namespace chEngineSDK
