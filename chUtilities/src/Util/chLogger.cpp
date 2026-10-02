/************************************************************************/
/**
 * @file chLogger.cpp
 * @author AccelMR
 * @date 2025/04/15
 * @brief Implementation of the Chimera Engine logging system
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chLogger.h"

#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>

#include "chFileSystem.h"
#include "chPath.h"
#include "chSTDStreams.h"
#include "chSTDThreading.h"
#include "chStringUtils.h"

namespace chEngineSDK {

/**
 * @brief Get current timestamp string
 * @return Formatted timestamp
 */
NODISCARD static String
getCurrentTimeString()
{
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

/**
 * @brief Get verbosity name as string
 * @param verbosity Verbosity level
 * @return String representation
 */
NODISCARD String
getVerbosityName(LogVerbosity verbosity)
{
  switch (verbosity) {
  case LogVerbosity::Fatal:
    return "FATAL";
  case LogVerbosity::Error:
    return "ERROR";
  case LogVerbosity::Warning:
    return "WARNING";
  case LogVerbosity::Info:
    return "INFO";
  case LogVerbosity::Debug:
    return "DEBUG";
  case LogVerbosity::NoLogging:
  default:
    return "NONE";
  }
}

/**
 * @brief Get verbosity color escape code for console output
 * @param verbosity Verbosity level
 * @return ANSI color code
 */
NODISCARD static String
getVerbosityColor(LogVerbosity verbosity)
{
  switch (verbosity) {
  case LogVerbosity::Fatal:
    return "\033[1;31m"; // Bold Red
  case LogVerbosity::Error:
    return "\033[31m"; // Red
  case LogVerbosity::Warning:
    return "\033[33m"; // Yellow
  case LogVerbosity::Info:
    return "\033[0m"; // Default
  case LogVerbosity::Debug:
    return "\033[36m"; // Cyan
  case LogVerbosity::NoLogging:
  default:
    return "\033[0m"; // Default
  }
}

/**
 * @brief ANSI escape code to reset colors
 */
static const String COLOR_RESET = "\033[0m";

//--------------------------------------------------------------------------
// LogCategory Implementation
//--------------------------------------------------------------------------

LogCategory::LogCategory(const String& name, const LogCategoryConfig& config)
 : m_name(name),
   m_config(config)
{}

/*
 */
void
LogCategory::log(LogVerbosity verbosity,
                 const String& message,
                 const ANSICHAR* file,
                 int32 line,
                 const ANSICHAR* function) const
{
  if (!isEnabled(verbosity)) {
    return;
  }

  Logger::instance().writeLogMessage(*this, verbosity, message, file, line, function);
}

//--------------------------------------------------------------------------
// Logger Implementation
//--------------------------------------------------------------------------

// Recursive because a callback of logWrittenEvent runs with the lock held and may log.
struct Logger::Impl
{
  Vector<LogCategory*> categories;
  bool consoleOutput = true;
  bool fileOutput = false;
  String logFilename;
  SPtr<DataStream> logFile;
  RecursiveMutex mutex;
  Event<void(const LogBufferEntry&)> logWrittenEvent;

  Vector<LogBufferEntry> logBuffer;
  uint32 maxBufferSize = 500;
  bool bufferingEnabled = false;
};

/*
 */
Logger::Logger()
 : m_impl(chMakeUnique<Impl>())
{}

/*
 */
Logger::~Logger()
{
  if (m_impl->fileOutput && m_impl->logFile) {
    m_impl->logFile->close();
    m_impl->logFile.reset();
  }
}

/*
 */
void
Logger::onStartUp()
{}

/*
 */
void
Logger::onShutDown()
{
  RecursiveLock lock(m_impl->mutex);

  if (m_impl->fileOutput && m_impl->logFile) {
    m_impl->logFile->close();
    m_impl->logFile.reset();
  }

  m_impl->categories.clear();
}

/*
 */
void
Logger::registerCategory(LogCategory& category)
{
  RecursiveLock lock(m_impl->mutex);

  for (auto* existingCategory : m_impl->categories) {
    if (existingCategory == &category) {
      return;
    }
  }

  m_impl->categories.push_back(&category);
}

/*
 */
LogCategory*
Logger::findCategory(const String& name)
{
  RecursiveLock lock(m_impl->mutex);

  for (auto* category : m_impl->categories) {
    if (category->getName() == name) {
      return category;
    }
  }

  return nullptr;
}

/*
 */
Vector<LogCategory*>
Logger::getCategories() const
{
  RecursiveLock lock(m_impl->mutex);
  return m_impl->categories;
}

/*
 */
void
Logger::setGlobalVerbosity(LogVerbosity verbosity)
{
  RecursiveLock lock(m_impl->mutex);

  for (auto* category : m_impl->categories) {
    category->setVerbosity(verbosity);
  }
}

/*
 */
void
Logger::setConsoleOutput(bool enabled)
{
  RecursiveLock lock(m_impl->mutex);
  m_impl->consoleOutput = enabled;
}

/*
 */
void
Logger::setBufferingEnabled(bool enabled, uint32 maxSize)
{
  RecursiveLock lock(m_impl->mutex);
  m_impl->bufferingEnabled = enabled;
  m_impl->maxBufferSize = maxSize;

  if (!enabled) {
    m_impl->logBuffer.clear();
  }

  m_impl->logBuffer.reserve(m_impl->maxBufferSize);
}

/*
 */
Vector<LogBufferEntry>
Logger::getBufferedLogs() const
{
  RecursiveLock lock(m_impl->mutex);
  return m_impl->logBuffer;
}

/*
 */
void
Logger::setFileOutput(bool enabled, const String& filename)
{
  RecursiveLock lock(m_impl->mutex);

  if (m_impl->fileOutput && m_impl->logFile) {
    m_impl->logFile->close();
    m_impl->logFile.reset();
  }

  m_impl->fileOutput = enabled;
  if (!enabled) {
    return;
  }

  m_impl->logFilename = filename;
  m_impl->logFile = FileSystem::createAndOpenFile(Path(m_impl->logFilename));

  if (!m_impl->logFile) {
    m_impl->fileOutput = false;
    if (m_impl->consoleOutput) {
      std::cerr << "Failed to open log file: " << m_impl->logFilename << std::endl;
    }
  }
}

/*
 */
HEvent
Logger::onLogWritten(Function<void(const LogBufferEntry&)> callback)
{
  return m_impl->logWrittenEvent.connect(std::move(callback));
}

/*
 */
void
Logger::writeLogMessage(const LogCategory& category,
                        LogVerbosity verbosity,
                        const String& message,
                        const ANSICHAR* file,
                        int32 line,
                        const ANSICHAR* function)
{
  RecursiveLock lock(m_impl->mutex);

  const String timestamp = getCurrentTimeString();

  String sourceLocation;
  if (file != nullptr && line > 0) {
    const ANSICHAR* shortFile = file;
    for (const ANSICHAR* p = file; *p != '\0'; ++p) {
      if (*p == '/' || *p == '\\') {
        shortFile = p + 1;
      }
    }

    sourceLocation = chString::format(" [{0}:{1}]", String(shortFile), line);

    if (function != nullptr) {
      sourceLocation += chString::format(" {0}", function);
    }
  }

  const String formattedMessage =
      chString::format("[{0}] [{1}] [{2}]{3}:\n\t{4}",
                       timestamp,
                       getVerbosityName(verbosity),
                       category.getName(),
                       sourceLocation,
                       message);

  if (m_impl->consoleOutput) {
    const String colorCode = getVerbosityColor(verbosity);
    std::cout << colorCode << formattedMessage << COLOR_RESET << std::endl;

    if (verbosity == LogVerbosity::Fatal) {
      std::cerr << colorCode << formattedMessage << COLOR_RESET << std::endl;
    }
  }

  if (m_impl->fileOutput && m_impl->logFile && m_impl->logFile->isWriteable()) {
    m_impl->logFile->write(formattedMessage.data(), formattedMessage.size());
    static constexpr char newline = '\n';
    m_impl->logFile->write(&newline, 1);
  }

  LogBufferEntry entry(timestamp,
                       verbosity,
                       category.getName(),
                       message,
                       file ? file : "",
                       line,
                       function ? function : "");

  m_impl->logWrittenEvent(entry);

  if (m_impl->bufferingEnabled) {
    m_impl->logBuffer.push_back(std::move(entry));
    if (m_impl->logBuffer.size() > m_impl->maxBufferSize) {
      m_impl->logBuffer.erase(m_impl->logBuffer.begin());
    }
  }
}

} // namespace chEngineSDK
