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

#include <chrono>
#include <ctime>
#include <iostream>

#include "chAlgorithm.h"
#include "chFileSystem.h"
#include "chMath.h"
#include "chPath.h"
#include "chSTDThreading.h"

namespace chEngineSDK {

namespace {

/**
 * Writes the local time as "YYYY-MM-DD HH:MM:SS.mmm".
 */
void
writeTimestamp(ANSICHAR (&buffer)[24]) noexcept
{
  const auto now = std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() %
      1000;

  // std::localtime shares one result between threads, and any thread can log.
  std::tm localTime{};
#if USING(CH_PLATFORM_WIN32)
  localtime_s(&localTime, &time);
#else
  localtime_r(&time, &localTime);
#endif

  const SIZE_T length = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
  if (length + 5 > sizeof(buffer)) {
    return;
  }

  buffer[length] = '.';
  buffer[length + 1] = static_cast<ANSICHAR>('0' + milliseconds / 100);
  buffer[length + 2] = static_cast<ANSICHAR>('0' + milliseconds / 10 % 10);
  buffer[length + 3] = static_cast<ANSICHAR>('0' + milliseconds % 10);
  buffer[length + 4] = '\0';
}

NODISCARD const ANSICHAR*
getFileName(const ANSICHAR* path) noexcept
{
  const ANSICHAR* fileName = path;
  for (const ANSICHAR* p = path; *p != '\0'; ++p) {
    if (*p == '/' || *p == '\\') {
      fileName = p + 1;
    }
  }
  return fileName;
}

/**
 * ANSI escape code that colors the console text for the given verbosity.
 */
NODISCARD const ANSICHAR*
getVerbosityColor(LogVerbosity verbosity) noexcept
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

constexpr StringView kColorResetAndNewLine = "\033[0m\n";

// A program that never starts the Logger would otherwise keep every early entry.
constexpr SIZE_T kMaxEarlyEntries = 1000;

/**
 * Every LogCategory that exists. Created with new and never deleted: categories are
 * statics of other files, of the executable and of plugins, and some are destroyed after
 * the statics of this file, so the list must still be there when they remove themselves.
 */
struct CategoryRegistry
{
  Mutex mutex;
  Vector<LogCategory*> categories;
};

CategoryRegistry&
getCategoryRegistry()
{
  static CategoryRegistry* registry = new CategoryRegistry();
  return *registry;
}

/**
 * What CH_LOG uses while the Logger is not running. Never deleted, for the same reason as
 * CategoryRegistry: a static destructor may log after the statics of this file are gone.
 */
struct StoppedLoggerState
{
  Mutex mutex;
  // Entries written before the Logger started; it takes them when it starts.
  Vector<SPtr<const LogBufferEntry>> earlyEntries;
  bool loggerShutDown = false;
  String line;
};

StoppedLoggerState&
getStoppedLoggerState()
{
  static StoppedLoggerState* state = new StoppedLoggerState();
  return *state;
}

NODISCARD SPtr<const LogBufferEntry>
createEntry(const LogCategory& category,
            LogVerbosity verbosity,
            String message,
            const ANSICHAR* file,
            int32 line,
            const ANSICHAR* function)
{
  SPtr<LogBufferEntry> entry = chMakeShared<LogBufferEntry>();
  writeTimestamp(entry->timestamp);
  entry->verbosity = verbosity;
  entry->category = category.getName();
  entry->message = std::move(message);
  if (nullptr != file) {
    entry->sourceFile = getFileName(file);
  }
  entry->sourceLine = line;
  if (nullptr != function) {
    entry->sourceFunctionName = function;
  }
  return entry;
}

/**
 * Writes the console line of an entry into text and returns the same line without its
 * color codes, which is what the log file gets.
 */
StringView
buildLine(const LogBufferEntry& entry, String& text)
{
  const StringView colorCode = getVerbosityColor(entry.verbosity);
  text.clear();
  text.append(colorCode);
  text.append("[").append(entry.timestamp);
  text.append("] [").append(getVerbosityName(entry.verbosity));
  text.append("] [").append(entry.category).append("]");
  if (!entry.sourceFile.empty() && entry.sourceLine > 0) {
    ANSICHAR lineNumber[StringUtils::MAX_INTEGER_CHARS];
    text.append(" [").append(entry.sourceFile).append(":");
    text.append(StringUtils::toChars(lineNumber, entry.sourceLine)).append("]");
    if (!entry.sourceFunctionName.empty()) {
      text.append(" ").append(entry.sourceFunctionName);
    }
  }
  text.append(":\n\t").append(entry.message);
  const SIZE_T plainLength = text.size() - colorCode.size();
  text.append(kColorResetAndNewLine);

  return StringView(text).substr(colorCode.size(), plainLength);
}

// Only important messages are flushed right away, because flushing on every log is the
// slowest part of logging.
NODISCARD bool
mustFlush(LogVerbosity verbosity) noexcept
{
  return verbosity <= LogVerbosity::Warning;
}

void
writeToConsole(const String& text, LogVerbosity verbosity)
{
  std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (mustFlush(verbosity)) {
    std::cout.flush();
  }

  if (verbosity == LogVerbosity::Fatal) {
    std::cerr.write(text.data(), static_cast<std::streamsize>(text.size()));
  }
}

} // namespace

CH_LOG_DECLARE_STATIC(LoggerLog, All);

/*
 */
const ANSICHAR*
getVerbosityName(LogVerbosity verbosity) noexcept
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

//--------------------------------------------------------------------------
// LogCategory Implementation
//--------------------------------------------------------------------------

LogCategory::LogCategory(const String& name, const LogCategoryConfig& config)
 : m_name(name),
   m_config(config)
{
  CategoryRegistry& registry = getCategoryRegistry();
  LockGuard<Mutex> lock(registry.mutex);
  registry.categories.push_back(this);
}

/*
 */
LogCategory::~LogCategory()
{
  CategoryRegistry& registry = getCategoryRegistry();
  LockGuard<Mutex> lock(registry.mutex);
  Algorithm::removeFirst(registry.categories, this);
}

/*
 */
void
LogCategory::log(LogVerbosity verbosity,
                 String message,
                 const ANSICHAR* file,
                 int32 line,
                 const ANSICHAR* function) const
{
  if (!isEnabled(verbosity)) {
    return;
  }

  if (Logger::isStarted()) {
    Logger::instance().writeLogMessage(*this, verbosity, std::move(message), file, line,
                                       function);
    return;
  }

  SPtr<const LogBufferEntry> entry =
      createEntry(*this, verbosity, std::move(message), file, line, function);

  StoppedLoggerState& state = getStoppedLoggerState();
  LockGuard<Mutex> lock(state.mutex);
  buildLine(*entry, state.line);
  writeToConsole(state.line, verbosity);
  if (state.loggerShutDown) {
    return;
  }

  if (state.earlyEntries.size() >= kMaxEarlyEntries) {
    state.earlyEntries.erase(state.earlyEntries.begin());
  }
  state.earlyEntries.push_back(std::move(entry));
}

//--------------------------------------------------------------------------
// Logger Implementation
//--------------------------------------------------------------------------

// Recursive because a callback of logWrittenEvent runs with the lock held and may log.
struct Logger::Impl
{
  bool consoleOutput = true;
  bool fileOutput = false;
  String logFilename;
  SPtr<DataStream> logFile;
  RecursiveMutex mutex;
  Event<void(const SPtr<const LogBufferEntry>&)> logWrittenEvent;

  // Ring buffer: once full, logBufferStart is the index of the oldest entry.
  Vector<SPtr<const LogBufferEntry>> logBuffer;
  uint32 logBufferStart = 0;
  uint32 maxBufferSize = 500;
  bool bufferingEnabled = true;

  // Reused for every line, so writing a log does not allocate once it has grown.
  String line;

  template<typename Callback>
  void
  forEachBuffered(Callback&& callback) const
  {
    const SIZE_T count = logBuffer.size();
    for (SIZE_T i = 0; i < count; ++i) {
      SIZE_T index = logBufferStart + i;
      if (index >= count) {
        index -= count;
      }
      callback(logBuffer[index]);
    }
  }

  void
  pushBuffered(SPtr<const LogBufferEntry> entry)
  {
    if (logBuffer.size() < maxBufferSize) {
      logBuffer.push_back(std::move(entry));
      return;
    }

    logBuffer[logBufferStart] = std::move(entry);
    ++logBufferStart;
    if (logBufferStart == logBuffer.size()) {
      logBufferStart = 0;
    }
  }

  void
  writeToFile(StringView plainLine, bool flush)
  {
    if (!fileOutput || !logFile || !logFile->isWriteable()) {
      return;
    }

    logFile->write(plainLine.data(), plainLine.size());
    static constexpr ANSICHAR newLine = '\n';
    logFile->write(&newLine, 1);
    if (flush) {
      logFile->flush();
    }
  }
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
{
  StoppedLoggerState& state = getStoppedLoggerState();
  LockGuard<Mutex> stateLock(state.mutex);
  RecursiveLock lock(m_impl->mutex);

  // They were printed to the console when they were logged, so they only go to the buffer.
  for (SPtr<const LogBufferEntry>& entry : state.earlyEntries) {
    m_impl->pushBuffered(std::move(entry));
  }
  state.earlyEntries.clear();
  state.earlyEntries.shrink_to_fit();
}

/*
 */
void
Logger::onShutDown()
{
  {
    StoppedLoggerState& state = getStoppedLoggerState();
    LockGuard<Mutex> stateLock(state.mutex);
    state.loggerShutDown = true;
  }

  RecursiveLock lock(m_impl->mutex);
  if (m_impl->fileOutput && m_impl->logFile) {
    m_impl->logFile->close();
    m_impl->logFile.reset();
  }
}

/*
 */
LogCategory*
Logger::findCategory(const String& name)
{
  CategoryRegistry& registry = getCategoryRegistry();
  LockGuard<Mutex> lock(registry.mutex);

  for (LogCategory* category : registry.categories) {
    if (category->getName() == name) {
      return category;
    }
  }

  return nullptr;
}

/*
 */
Vector<LogCategory*>
Logger::getCategories()
{
  CategoryRegistry& registry = getCategoryRegistry();
  LockGuard<Mutex> lock(registry.mutex);
  return registry.categories;
}

/*
 */
void
Logger::setGlobalVerbosity(LogVerbosity verbosity)
{
  CategoryRegistry& registry = getCategoryRegistry();
  LockGuard<Mutex> lock(registry.mutex);

  for (LogCategory* category : registry.categories) {
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
  Vector<SPtr<const LogBufferEntry>>& buffer = m_impl->logBuffer;

  m_impl->bufferingEnabled = enabled;
  m_impl->maxBufferSize = Math::max<uint32>(maxSize, 1);

  if (!enabled) {
    buffer.clear();
    m_impl->logBufferStart = 0;
    return;
  }

  // Puts the entries back in order, so the ring starts at index 0 again.
  Algorithm::rotateToFront(buffer, m_impl->logBufferStart);
  m_impl->logBufferStart = 0;

  if (buffer.size() > m_impl->maxBufferSize) {
    buffer.erase(buffer.begin(), buffer.end() - m_impl->maxBufferSize);
  }

  buffer.reserve(m_impl->maxBufferSize);
}

/*
 */
Vector<SPtr<const LogBufferEntry>>
Logger::getBufferedLogs() const
{
  RecursiveLock lock(m_impl->mutex);

  Vector<SPtr<const LogBufferEntry>> entries;
  entries.reserve(m_impl->logBuffer.size());
  m_impl->forEachBuffered(
      [&entries](const SPtr<const LogBufferEntry>& entry) { entries.push_back(entry); });
  return entries;
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
    CH_LOG_ERROR(LoggerLog, "Failed to open log file: {0}", m_impl->logFilename);
    return;
  }

  // The file is opened once the project folder is known, so what was logged before (start
  // up, finding the project) is written first and the file has the whole session.
  m_impl->forEachBuffered([this](const SPtr<const LogBufferEntry>& entry) {
    m_impl->writeToFile(buildLine(*entry, m_impl->line), false);
  });
  m_impl->logFile->flush();
}

/*
 */
HEvent
Logger::onLogWritten(Function<void(const SPtr<const LogBufferEntry>&)> callback,
                     bool replayBuffered)
{
  RecursiveLock lock(m_impl->mutex);

  if (replayBuffered) {
    m_impl->forEachBuffered(callback);
  }

  return m_impl->logWrittenEvent.connect(std::move(callback));
}

/*
 */
void
Logger::disconnectLogListener(HEvent& handle)
{
  // Without a Logger nothing calls the listeners, and the handle keeps what it needs to
  // disconnect alive on its own.
  if (!isStarted()) {
    handle.disconnect();
    return;
  }

  // writeLogMessage calls the listeners with this lock held, so once it is taken no
  // listener is still running.
  RecursiveLock lock(instance().m_impl->mutex);
  handle.disconnect();
}

/*
 */
void
Logger::writeLogMessage(const LogCategory& category,
                        LogVerbosity verbosity,
                        String message,
                        const ANSICHAR* file,
                        int32 line,
                        const ANSICHAR* function)
{
  RecursiveLock lock(m_impl->mutex);

  const SPtr<const LogBufferEntry> entry =
      createEntry(category, verbosity, std::move(message), file, line, function);

  const StringView plainLine = buildLine(*entry, m_impl->line);

  if (m_impl->consoleOutput) {
    writeToConsole(m_impl->line, verbosity);
  }

  m_impl->writeToFile(plainLine, mustFlush(verbosity));

  if (m_impl->bufferingEnabled) {
    m_impl->pushBuffered(entry);
  }

  m_impl->logWrittenEvent(entry);
}

} // namespace chEngineSDK
