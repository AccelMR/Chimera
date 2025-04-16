/************************************************************************/
/**
 * @file chLogger.h
 * @author AccelMR
 * @date 2025/04/15
 * @brief Logging system for Chimera Engine
 *
 * Provides a flexible, category-based logging system similar to Unreal Engine,
 * optimized for minimal string copies and runtime configuration.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chModule.h"
#include "chStringUtils.h"
#include "chLogDeclaration.h"

#ifdef CH_ENABLE_LOG_VERBOSE
#define CH_LOG_VERBOSE USE_IF(USING(CH_DEBUG_MODE))
#else
#define CH_LOG_VERBOSE NOT_IN_USE
#endif

namespace chEngineSDK {
class DataStream;

/**
 * @brief Configuration options for log categories
 */
struct LogCategoryConfig
{
  // Should this log category be compiled in non-debug builds
  bool compileInNonDebug = true;

  // Default verbosity level
  LogVerbosity defaultVerbosity = LogVerbosity::Info;

  // Runtime verbosity level (can be changed during execution)
  LogVerbosity runtimeVerbosity = LogVerbosity::Info;
};

/**
 * @brief Represents a log category in the Chimera Engine
 */
class CH_UTILITY_EXPORT LogCategory
{
 public:
  /**
   * @brief Constructs a log category
   * @param name Category name
   * @param config Configuration for this category
   */
  explicit LogCategory(const String& name,
                       const LogCategoryConfig& config = LogCategoryConfig());

  /**
   * @brief Gets the name of this log category
   * @return The category name
   */
  FORCEINLINE const String&
  getName() const
  {
    return m_name;
  }

  /**
   * @brief Checks if logging is enabled for the given verbosity
   * @param verbosity Verbosity level to check
   * @return True if logging is enabled for this verbosity
   */
  FORCEINLINE bool
  isEnabled(LogVerbosity verbosity) const
  {
    return verbosity <= m_config.runtimeVerbosity;
  }

  /**
   * @brief Set the runtime verbosity for this category
   * @param verbosity New verbosity level
   */
  FORCEINLINE void
  setVerbosity(LogVerbosity verbosity)
  {
    m_config.runtimeVerbosity = verbosity;
  }

  /**
   * @brief Reset the runtime verbosity to the default
   */
  FORCEINLINE void
  resetVerbosity()
  {
    m_config.runtimeVerbosity = m_config.defaultVerbosity;
  }

  /**
   * @brief Log a message with this category
   * @param verbosity Verbosity level
   * @param message Message to log
   * @param file Source file
   * @param line Line number
   * @param function Function name
   */
  void
  log(LogVerbosity verbosity, 
      const String& message, 
      const char* file = nullptr, int line = 0,
      const char* function = nullptr) const;

  /**
   * @brief Log a message with this category
   * @param verbosity Verbosity level
   * @param message Message to log
   * @param file Source file
   * @param line Line number
   * @param function Function name
   */
  void
  log(LogVerbosity verbosity, 
      const String&& message, 
      const char* file = nullptr, int line = 0,
      const char* function = nullptr) const;

 private:
  String m_name;
  LogCategoryConfig m_config;
};

/**
 * @brief Main logger class for Chimera Engine
 *
 * Singleton class that manages log categories and output destinations
 */
class CH_UTILITY_EXPORT Logger : public Module<Logger>
{
 public:
  friend class Module<Logger>;

  /**
   * @brief Register a log category with the logger
   * @param category Category to register
   */
  void
  registerCategory(LogCategory& category);

  /**
   * @brief Find a log category by name
   * @param name Category name
   * @return Pointer to category or nullptr if not found
   */
  LogCategory*
  findCategory(const String& name);

  /**
   * @brief Get all registered categories
   * @return Vector of registered categories
   */
  FORCEINLINE const Vector<LogCategory*>&
  getCategories() const {
    return m_categories;
  }

  /**
   * @brief Set global verbosity level for all categories
   * @param verbosity New verbosity level
   */
  void
  setGlobalVerbosity(LogVerbosity verbosity);

  /**
   * @brief Enable/disable console output
   * @param enabled True to enable, false to disable
   */
  FORCEINLINE void
  setConsoleOutput(bool enabled) {
    m_consoleOutput = enabled;
  }

  /**
   * @brief Enable/disable file output
   * @param enabled True to enable, false to disable
   * @param filename Optional filename to use
   */
  void
  setFileOutput(bool enabled, const String& filename = "Chimera.log");

  /**
   * @brief Write a message to all enabled outputs
   * @param category Log category
   * @param verbosity Verbosity level
   * @param message Message to log
   * @param file Source file
   * @param line Line number
   * @param function Function name
   */
  void
  writeLogMessage(const LogCategory& category, 
                  LogVerbosity verbosity, 
                  const String& message,
                  const char* file = nullptr, 
                  int line = 0, 
                  const char* function = nullptr);

  /**
   * @brief Write a message to all enabled outputs
   * @param category Log category
   * @param verbosity Verbosity level
   * @param message Message to log
   * @param file Source file
   * @param line Line number
   * @param function Function name
   */
  void
  writeLogMessage(const LogCategory& category, 
                  LogVerbosity verbosity, 
                  String&& message,
                  const char* file = nullptr, 
                  int line = 0, 
                  const char* function = nullptr);

 protected:
  /**
   * @brief Constructor
   */
  Logger();

  /**
   * @brief Destructor
   */
  ~Logger();

  /**
   * @brief Called when module starts up
   */
  void
  onStartUp() override;

  /**
   * @brief Called when module shuts down
   */
  void
  onShutDown() override;

 private:
  Vector<LogCategory*> m_categories;
  bool m_consoleOutput = true;
  bool m_fileOutput = false;
  String m_logFilename;
  SPtr<DataStream> m_logFile;
  RecursiveMutex m_mutex;
};

/**
 * @brief Get verbosity name as string
 * @param verbosity Verbosity level
 * @return String representation
 */
CH_UTILITY_EXPORT String
getVerbosityName(LogVerbosity verbosity);

} // namespace chEngineSDK

// Macros for defining and declaring log categories

/**
 * @brief Define a log category in a single file
 */
#define CH_LOG_DEFINE_CATEGORY(CategoryName, DefaultVerbosity)                                \
  chEngineSDK::LogCategory CategoryName(#CategoryName,                                        \
                                        {true, chEngineSDK::LogVerbosity::DefaultVerbosity,   \
                                         chEngineSDK::LogVerbosity::DefaultVerbosity})

/**
 * @brief Define a log category to be used across multiple files
 */
#define CH_LOG_DEFINE_CATEGORY_SHARED(CategoryName, DefaultVerbosity)                         \
  chEngineSDK::LogCategory CategoryName(#CategoryName,                                        \
                                        {true, chEngineSDK::LogVerbosity::DefaultVerbosity,   \
                                         chEngineSDK::LogVerbosity::DefaultVerbosity})

/**
 * @brief Declare an extern log category to use in a .cpp file
 */
//#define CH_LOG_DECLARE_EXTERN(CategoryName) extern chEngineSDK::LogCategory CategoryName

/**
 * @brief Declare a static log category for use in a single .cpp file
 */
#define CH_LOG_DECLARE_STATIC(CategoryName, DefaultVerbosity)                                 \
  static chEngineSDK::LogCategory CategoryName(                                               \
      #CategoryName, {true, chEngineSDK::LogVerbosity::DefaultVerbosity,                      \
                      chEngineSDK::LogVerbosity::DefaultVerbosity})

// Actual logging macros
#if USING(CH_LOG_VERBOSE)
  #define CH_LOG(Category, Verbosity, Format, ...) \
    do { \
      if ((Category).isEnabled(chEngineSDK::LogVerbosity::Verbosity)) { \
        (Category).log(chEngineSDK::LogVerbosity::Verbosity, \
          std::move(chEngineSDK::chString::format(Format, ##__VA_ARGS__)), \
          __FILE__, __LINE__, __PRETTY_FUNCTION__); \
      } \
    } while(0)
#else
  #define CH_LOG(Category, Verbosity, Format, ...) \
    do { \
      if ((Category).isEnabled(chEngineSDK::LogVerbosity::Verbosity)) { \
        (Category).log(chEngineSDK::LogVerbosity::Verbosity, \
          std::move(chEngineSDK::chString::format(Format, ##__VA_ARGS__)), \
          nullptr, 0, nullptr); \
      } \
    } while(0)
#endif

// Common logging helpers
#define CH_LOG_FATAL(Category, Format, ...) CH_LOG(Category, Fatal, Format, ##__VA_ARGS__)
#define CH_LOG_ERROR(Category, Format, ...) CH_LOG(Category, Error, Format, ##__VA_ARGS__)
#define CH_LOG_WARNING(Category, Format, ...) CH_LOG(Category, Warning, Format, ##__VA_ARGS__)
#define CH_LOG_INFO(Category, Format, ...) CH_LOG(Category, Info, Format, ##__VA_ARGS__)
#define CH_LOG_DEBUG(Category, Format, ...) CH_LOG(Category, Debug, Format, ##__VA_ARGS__)
#define CH_LOG_TRACE(Category, Format, ...) CH_LOG(Category, Trace, Format, ##__VA_ARGS__)