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

#include "chEventSystem.h"
#include "chLogDeclaration.h"
#include "chModule.h"
#include "chStringUtils.h"

// Todo: change this to show may be verbose on some categories but not globally
#define CH_LOG_VERBOSE IN_USE

namespace chEngineSDK {
class DataStream;

/**
 * @brief Log entry structure for display
 */
struct LogBufferEntry {
  String timestamp;
  LogVerbosity verbosity;
  String category;
  String message;
  String sourceFile;
  int32 sourceLine;
  String sourceFunctionName;

  LogBufferEntry() = default;
  LogBufferEntry(const String& inTimestamp,
           LogVerbosity inVerbosity,
           const String& inCategoryName,
           const String& inLogMessage,
           const String& inSourceFileName = "",
           int32 inSourceLineNumber = 0,
           const String& inSourceFunctionName = "")
   : timestamp(inTimestamp),
     verbosity(inVerbosity),
     category(inCategoryName),
     message(inLogMessage),
     sourceFile(inSourceFileName),
     sourceLine(inSourceLineNumber),
     sourceFunctionName(inSourceFunctionName) {}
};

/**
 * @brief Configuration options for log categories
 */
struct LogCategoryConfig {

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
  NODISCARD FORCEINLINE const String&
  getName() const {
    return m_name;
  }

  /**
   * @brief Checks if logging is enabled for the given verbosity
   * @param verbosity Verbosity level to check
   * @return True if logging is enabled for this verbosity
   */
  NODISCARD FORCEINLINE bool
  isEnabled(LogVerbosity verbosity) const {
    return verbosity <= m_config.runtimeVerbosity;
  }

  /**
   * @brief Set the runtime verbosity for this category
   * @param verbosity New verbosity level
   */
  FORCEINLINE void
  setVerbosity(LogVerbosity verbosity) {
    m_config.runtimeVerbosity = verbosity;
  }

  /**
   * @brief Reset the runtime verbosity to the default
   */
  FORCEINLINE void
  resetVerbosity() {
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
  log(LogVerbosity verbosity, const
      String& message,
      const ANSICHAR* file = nullptr,
      int32 line = 0,
      const ANSICHAR* function = nullptr) const;

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
      const ANSICHAR* file = nullptr,
      int32 line = 0,
      const ANSICHAR* function = nullptr) const;

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
  NODISCARD LogCategory*
  findCategory(const String& name);

  /**
   * @brief Get all registered categories
   * @return Vector of registered categories
   */
  NODISCARD FORCEINLINE const Vector<LogCategory*>&
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
   * @brief Set Buffering for log messages
   * @param enabled True to enable buffering, false to disable
   * @param maxSize Maximum number of messages to buffer Default is 500
   */
  void
  setBufferingEnabled(bool enabled, uint32 maxSize = 500);

  /**
   * @brief Get the current log buffer
   * @return Vector of buffered log entries
   */
  NODISCARD const Vector<LogBufferEntry>&
  getBufferedLogs() const;

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
  writeLogMessage(const LogCategory& category, LogVerbosity verbosity, const String& message,
                  const ANSICHAR* file = nullptr, int32 line = 0, const ANSICHAR* function = nullptr);

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
  writeLogMessage(const LogCategory& category, LogVerbosity verbosity, String&& message,
                  const ANSICHAR* file = nullptr, int32 line = 0, const ANSICHAR* function = nullptr);


  /**
   * @brief Event triggered when a log entry is written
   * @param callback Function to call when a log entry is written
   * @return Event handle
  */
  HEvent
  onLogWritten(Function<void(const LogBufferEntry&)> callback) {
    return m_logWrittenEvent.connect(callback);
  }

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
  Event<void(const LogBufferEntry&)> m_logWrittenEvent;


  Vector<LogBufferEntry> m_logBuffer;
  uint32 m_maxBufferSize = 500;  // Keep last 500 logs
  bool m_bufferingEnabled = false;
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
                                        {chEngineSDK::LogVerbosity::DefaultVerbosity,   \
                                         chEngineSDK::LogVerbosity::DefaultVerbosity})

/**
 * @brief Define a log category to be used across multiple files
 */
#define CH_LOG_DEFINE_CATEGORY_SHARED(CategoryName, DefaultVerbosity)                         \
  chEngineSDK::LogCategory CategoryName(#CategoryName,                                        \
                                        {chEngineSDK::LogVerbosity::DefaultVerbosity,   \
                                         chEngineSDK::LogVerbosity::DefaultVerbosity})

/**
 * @brief Declare an extern log category to use in a .cpp file
 */
// #define CH_LOG_DECLARE_EXTERN(CategoryName) extern chEngineSDK::LogCategory CategoryName

/**
 * @brief Declare a static log category for use in a single .cpp file
 */
#define CH_LOG_DECLARE_STATIC(CategoryName, DefaultVerbosity)                                 \
  static chEngineSDK::LogCategory CategoryName(                                               \
      #CategoryName, {chEngineSDK::LogVerbosity::DefaultVerbosity,                      \
                      chEngineSDK::LogVerbosity::DefaultVerbosity})

// Actual logging macros
#if USING(CH_LOG_VERBOSE)
#define CH_LOG(Category, Verbosity, Format, ...)                                              \
  do {                                                                                        \
    if ((Category).isEnabled(chEngineSDK::LogVerbosity::Verbosity)) {                         \
      (Category).log(chEngineSDK::LogVerbosity::Verbosity,                                    \
                     std::move(chEngineSDK::chString::format(Format, ##__VA_ARGS__)),         \
                     __FILE__, __LINE__, __PRETTY_FUNCTION__);                                \
    }                                                                                         \
  } while (0)
#else
#define CH_LOG(Category, Verbosity, Format, ...)                                              \
  do {                                                                                        \
    if ((Category).isEnabled(chEngineSDK::LogVerbosity::Verbosity)) {                         \
      (Category).log(chEngineSDK::LogVerbosity::Verbosity,                                    \
                     std::move(chEngineSDK::chString::format(Format, ##__VA_ARGS__)),         \
                     nullptr, 0, nullptr);                                                    \
    }                                                                                         \
  } while (0)
#endif

// Common logging helpers
#define CH_LOG_FATAL(Category, Format, ...) CH_LOG(Category, Fatal, Format, ##__VA_ARGS__)
#define CH_LOG_ERROR(Category, Format, ...) CH_LOG(Category, Error, Format, ##__VA_ARGS__)
#define CH_LOG_WARNING(Category, Format, ...) CH_LOG(Category, Warning, Format, ##__VA_ARGS__)
#define CH_LOG_INFO(Category, Format, ...) CH_LOG(Category, Info, Format, ##__VA_ARGS__)
#define CH_LOG_DEBUG(Category, Format, ...) CH_LOG(Category, Debug, Format, ##__VA_ARGS__)
#define CH_LOG_TRACE(Category, Format, ...) CH_LOG(Category, Trace, Format, ##__VA_ARGS__)
