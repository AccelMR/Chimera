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

/**
 * One written log. The Logger creates it once and shares it as SPtr<const LogBufferEntry>
 * with its buffer and every listener, so no listener has to copy it.
 */
struct LogBufferEntry
{
  // "YYYY-MM-DD HH:MM:SS.mmm", kept inline so the entry does not allocate for it.
  ANSICHAR timestamp[24] = {};
  LogVerbosity verbosity = LogVerbosity::Info;
  String category;
  String message;
  // File name only, without its folder.
  String sourceFile;
  int32 sourceLine = 0;
  String sourceFunctionName;
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
 *
 * Every category adds itself to a list while it exists, so the Logger can change the
 * verbosity of all of them. The list keeps its address, so a category cannot be copied.
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

  ~LogCategory();

  LogCategory(const LogCategory&) = delete;

  LogCategory&
  operator=(const LogCategory&) = delete;

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
  log(LogVerbosity verbosity,
      String message,
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
 * Singleton class that manages log categories and output destinations. Its state lives
 * in chLogger.cpp so this header does not need <mutex>.
 *
 * CH_LOG can be used at any time. Before startUp messages go to the console and are kept,
 * then added to the buffer when the Logger starts; after shutDown they only go to the
 * console. Buffering is on by default, so the log file also gets the messages written
 * before it was opened.
 */
class CH_UTILITY_EXPORT Logger : public Module<Logger>
{
 public:
  friend class Module<Logger>;

  /**
   * @brief Find a log category by name
   * @param name Category name
   * @return Pointer to category or nullptr if not found
   */
  NODISCARD static LogCategory*
  findCategory(const String& name);

  /**
   * @brief Get all existing categories
   * @return Copy of the category list
   */
  NODISCARD static Vector<LogCategory*>
  getCategories();

  /**
   * @brief Set global verbosity level for all categories
   * @param verbosity New verbosity level
   */
  static void
  setGlobalVerbosity(LogVerbosity verbosity);

  /**
   * @brief Enable/disable console output
   * @param enabled True to enable, false to disable
   */
  void
  setConsoleOutput(bool enabled);

  /**
   * @brief Set Buffering for log messages
   * @param enabled True to enable buffering, false to disable
   * @param maxSize Maximum number of messages to buffer Default is 500
   */
  void
  setBufferingEnabled(bool enabled, uint32 maxSize = 500);

  /**
   * @brief Get the current log buffer
   * @return The buffered log entries, oldest first
   */
  NODISCARD Vector<SPtr<const LogBufferEntry>>
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
  writeLogMessage(const LogCategory& category,
                  LogVerbosity verbosity,
                  String message,
                  const ANSICHAR* file = nullptr,
                  int32 line = 0,
                  const ANSICHAR* function = nullptr);

  /**
   * @brief Event triggered when a log entry is written
   * @param callback Function to call when a log entry is written. It can be called from
   *        any thread that logs.
   * @param replayBuffered If true, the callback first receives every buffered entry, so
   *        no entry is missed or received twice
   * @return Event handle. Release it with disconnectLogListener.
   */
  NODISCARD HEvent
  onLogWritten(Function<void(const SPtr<const LogBufferEntry>&)> callback,
               bool replayBuffered = false);

  /**
   * @brief Disconnects a handle returned by onLogWritten
   *
   * When this returns the callback is not running on any thread and will not be called
   * again, so the object it uses can be destroyed. It can also be called after shutDown.
   */
  static void
  disconnectLogListener(HEvent& handle);

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
  struct Impl;

  UniquePtr<Impl> m_impl;
};

/**
 * @brief Get verbosity name as string
 * @param verbosity Verbosity level
 * @return String representation
 */
NODISCARD CH_UTILITY_EXPORT const ANSICHAR*
getVerbosityName(LogVerbosity verbosity) noexcept;

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
                     chEngineSDK::StringUtils::format(Format, ##__VA_ARGS__),                 \
                     __FILE__, __LINE__, CH_FUNCTION_SIGNATURE);                              \
    }                                                                                         \
  } while (0)
#else
#define CH_LOG(Category, Verbosity, Format, ...)                                              \
  do {                                                                                        \
    if ((Category).isEnabled(chEngineSDK::LogVerbosity::Verbosity)) {                         \
      (Category).log(chEngineSDK::LogVerbosity::Verbosity,                                    \
                     chEngineSDK::StringUtils::format(Format, ##__VA_ARGS__),                 \
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
