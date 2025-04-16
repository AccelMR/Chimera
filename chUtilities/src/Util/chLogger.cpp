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

#include "chFileSystem.h"
#include "chStringUtils.h"
#include "chPath.h"

namespace chEngineSDK {

/**
 * @brief Get current timestamp string
 * @return Formatted timestamp
 */
static String
getCurrentTimeString() {
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
String
getVerbosityName(LogVerbosity verbosity) {
  switch (verbosity)
  {
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
static String
getVerbosityColor(LogVerbosity verbosity) {
  switch (verbosity)
  {
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
   m_config(config) {
  if (Logger::isStarted()) {
    Logger::instance().registerCategory(*this);
  }
}

void
LogCategory::log(LogVerbosity verbosity, const String& message, const char* file, int line,
                 const char* function) const
{
  if (!isEnabled(verbosity)) {
    return;
  }

  Logger::instance().writeLogMessage(*this, verbosity, message, file, line, function);
}

/*
*/
void
LogCategory::log(LogVerbosity verbosity, 
                 const String&& message, 
                 const char* file, 
                 int line,
                 const char* function) const
{
  if (!isEnabled(verbosity)) {
    return;
  }

  Logger::instance().writeLogMessage(*this, verbosity, std::move(message), file, line, function);
}
//--------------------------------------------------------------------------
// Logger Implementation
//--------------------------------------------------------------------------

Logger::Logger() : m_consoleOutput(true), m_fileOutput(false) {}

Logger::~Logger() {
  if (m_fileOutput && m_logFile) {
    m_logFile->close();
    m_logFile.reset();
  }
}

void
Logger::onStartUp() {
  // Initialize the logger
}

void
Logger::onShutDown() {
  if (m_fileOutput && m_logFile) {
    m_logFile->close();
    m_logFile.reset();
  }

  m_categories.clear();
}

void
Logger::registerCategory(LogCategory& category) {
  RecursiveLock lock(m_mutex);

  // Check if category is already registered
  for (auto* existingCategory : m_categories) {
    if (existingCategory == &category) {
      return;
    }
  }

  m_categories.push_back(&category);
}

LogCategory*
Logger::findCategory(const String& name) {
  RecursiveLock lock(m_mutex);

  for (auto* category : m_categories) {
    if (category->getName() == name) {
      return category;
    }
  }

  return nullptr;
}

void
Logger::setGlobalVerbosity(LogVerbosity verbosity)
{
  RecursiveLock lock(m_mutex);

  for (auto* category : m_categories) {
    category->setVerbosity(verbosity);
  }
}

void
Logger::setFileOutput(bool enabled, const String& filename) {
  RecursiveLock lock(m_mutex);

  if (m_fileOutput && m_logFile) {
    m_logFile->close();
    m_logFile.reset();
  }

  m_fileOutput = enabled;

  if (enabled) {
    m_logFilename = filename;

    try {
      Path path (m_logFilename);
      m_logFile = FileSystem::createAndOpenFile(path);

      if (!m_logFile) {
        m_fileOutput = false;
        if (m_consoleOutput) {
          std::cerr << "Failed to open log file: " << m_logFilename << std::endl;
        }
      }
    } 
    catch (const Exception& e) {
      m_fileOutput = false;
      if (m_consoleOutput) {
        std::cerr << "Failed to open log file: " << e.what() << std::endl;
      }
    }
  }
}

void
Logger::writeLogMessage(const LogCategory& category, LogVerbosity verbosity,
                        const String& message, const char* file, int line,
                        const char* function) {
  RecursiveLock lock(m_mutex);

  String timestamp = getCurrentTimeString();
  String verbosityStr = getVerbosityName(verbosity);
  String sourceLocation;

  if (file != nullptr && line > 0) {
    String shortFile = file;
    size_t lastSlash = shortFile.find_last_of("/\\");
    if (lastSlash != String::npos) {
      shortFile = shortFile.substr(lastSlash + 1);
    }

    sourceLocation = chString::format(" [{0}:{1}]", shortFile, line);

    if (function != nullptr) {
      sourceLocation += chString::format(" {0}", function);
    }
  }

  String formattedMessage =
      chString::format("[{0}] [{1}] [{2}]{3}: {4}", timestamp, verbosityStr,
                          category.getName(), sourceLocation, message);

  // Write to console if enabled
  if (m_consoleOutput) {
    String colorCode = getVerbosityColor(verbosity);
    std::cout << colorCode << formattedMessage << COLOR_RESET << std::endl;

    // For fatal errors, also write to stderr
    if (verbosity == LogVerbosity::Fatal) {
      std::cerr << colorCode << formattedMessage << COLOR_RESET << std::endl;
    }
  }

  // Write to file if enabled
  if (m_fileOutput && m_logFile && m_logFile->isWriteable()) {
    formattedMessage += "\n";
    m_logFile->write(formattedMessage.data(), formattedMessage.size());
    //m_logFile->close();
  }
}

/*
*/
void
Logger::writeLogMessage(const LogCategory& category, 
                        LogVerbosity verbosity, 
                        String&& message,
                        const char* file, 
                        int line, 
                        const char* function)
{
  RecursiveLock lock(m_mutex);

  String timestamp = getCurrentTimeString();
  String verbosityStr = getVerbosityName(verbosity);
  String sourceLocation;

  if (file != nullptr && line > 0) {
    String shortFile = file;
    size_t lastSlash = shortFile.find_last_of("/\\");
    if (lastSlash != String::npos) {
      shortFile = shortFile.substr(lastSlash + 1);
    }

    sourceLocation = chString::format(" [{0}:{1}]", shortFile, line);

    if (function != nullptr) {
      sourceLocation += chString::format(" {0}", function);
    }
  }

  String formattedMessage =
      chString::format("[{0}] [{1}] [{2}]{3}: {4}", timestamp, verbosityStr,
                          category.getName(), sourceLocation, std::move(message));

  // Write to console if enabled
  if (m_consoleOutput) {
    String colorCode = getVerbosityColor(verbosity);
    std::cout << colorCode << formattedMessage << COLOR_RESET << std::endl;

    // For fatal errors, also write to stderr
    if (verbosity == LogVerbosity::Fatal) {
      std::cerr << colorCode << formattedMessage << COLOR_RESET << std::endl;
    }
  }

  // Write to file if enabled
  if (m_fileOutput && m_logFile && m_logFile->isWriteable()) {
    formattedMessage += "\n";
    m_logFile->write(formattedMessage.data(), formattedMessage.size());
  }
}

} // namespace chEngineSDK