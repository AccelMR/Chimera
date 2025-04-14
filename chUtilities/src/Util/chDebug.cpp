/************************************************************************/
/**
 * @file chDebug.cpp
 * @author AccelMR
 * @date 2022/06/15
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDebug.h"

#include "chStringUtils.h"
#include "chFileSystem.h"
# include <iostream>

#if USING(CH_COMPILER_MSVC)
#include "Win32/chWindows.h"
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void
logIDEConsole(const chEngineSDK::String& msg) {
  OutputDebugString(msg.c_str());
  OutputDebugString("\n");

  std::cout << msg << std::endl;
}
#elif USING(CH_PLATFORM_LINUX)
#include <execinfo.h>

void logIDEConsole(const chEngineSDK::String& msg) 
{
  std::cout << msg << std::endl;
}
#endif

namespace chHTMLDebugConstants{
  static const char* style = R"(
    body {
        font-family: Arial, sans-serif;
        background-color: #000;
        color: #fff;
        margin: 0;
        padding: 0;
    }
    .wrapper {
        padding: 20px;
    }
    h1 {
        text-align: center;
        color: #ccc;
    }
    .table {
        width: 100%;
        border-collapse: collapse;
        margin: 20px 0;
    }
    .table .header {
        background-color: #0073e6;
        color: #fff;
        border-radius: 8px 8px 0 0;
    }
    .table .row {
        display: flex;
        margin-bottom: 5px;
        border-radius: 8px;
        overflow: hidden;
    }
    .table .cell {
        flex: 1;
        padding: 10px;
        border: 1px solid #444;
        text-align: center;
    }
    )";

  static const char* htmlHeader = R"(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chimera Engine: Log</title>
  )";

  static const char* htmlFooter = R"(
    </body>
  </html>
  )";
}

namespace chEngineSDK{
const LogLevel* LogLevel::FATAL = nullptr;
const LogLevel* LogLevel::ERROR = nullptr;
const LogLevel* LogLevel::WARN = nullptr;
const LogLevel* LogLevel::INFO = nullptr;
const LogLevel* LogLevel::DEBUG = nullptr;

#define CH_DEFAULT_FATAL_STYLE   ".row.fatal { background-color: #800000; color: #ff9999; }"
#define CH_DEFAULT_ERROR_STYLE   ".row.error { background-color: #a31313; color: #ff9999; }"
#define CH_DEFAULT_WARN_STYLE    ".row.warning { background-color: #806600; color: #fff799; }"
#define CH_DEFAULT_INFO_STYLE    ".row.info { background-color: #333; color: #ccc; }"
#define CH_DEFAULT_DEBUG_STYLE   ".row.debug { background-color: #004d00; color: #99ff99; }"


/*
*/
Debug::Debug() {
  LogLevel::FATAL = registerLogLevel("FATAL", "Fatal", CH_DEFAULT_FATAL_STYLE, 0);
  LogLevel::ERROR = registerLogLevel("ERROR", "Error", CH_DEFAULT_ERROR_STYLE, 1);
  LogLevel::WARN = registerLogLevel("WARN", "Warning", CH_DEFAULT_WARN_STYLE, 2);
  LogLevel::INFO = registerLogLevel("INFO", "Info", CH_DEFAULT_INFO_STYLE, 3);
  LogLevel::DEBUG = registerLogLevel("DEBUG", "Debug", CH_DEFAULT_DEBUG_STYLE, 4);
}

/*
*/
const LogLevel*
Debug::registerLogLevel(const String& name, 
                        const String& displayName, 
                        const String& htmlStyle,
                        uint32 severity) {
  auto it = m_logLevels.find(name);
  if (it != m_logLevels.end()) {
    return it->second.get();
  }
  
  auto newLevel = std::make_unique<LogLevel>(name, displayName, htmlStyle, severity);
  auto* ptr = newLevel.get();
  m_logLevels[name] = std::move(newLevel);
  return ptr;
}

/*
*/
void 
Debug::logMessage(String&& msg, const LogLevel* level) {
#if USING(CH_DEBUG_MODE)
  logIDEConsole(StringUtils::format("[{0}] {1}", 
                                    level->getDisplayName(), 
                                    msg));
#endif // USING(CH_DEBUG_MODE)
  m_log.logMsg(std::move(msg), level);
}

/*
*/
void 
Debug::logMessage(const char* msg, const LogLevel* level) {
#if USING(CH_DEBUG_MODE)
  logIDEConsole(StringUtils::format("[{0}] {1}", 
                                    level->getDisplayName(), 
                                    msg));
#endif // USING(CH_DEBUG_MODE)
  m_log.logMsg(msg, level);
}

/*
*/
void
Debug::saveLog(const Path& path) const {
  auto fileStream = FileSystem::createAndOpenFile(path);
  CH_ASSERT(fileStream);
  for (const LogEntry& logEntry : m_log.getAllEntries()) {
  String msg = StringUtils::format("[{0}] {1}",
                                   logEntry.getLevel()->getDisplayName(), 
                                   logEntry.getLogMessage());
    fileStream << msg << "\n";
  }
  fileStream->close();
}

/*
*/
void 
Debug::saveLogAsHtml(const Path& path) const {
  using namespace chHTMLDebugConstants;
  StringStream stream;

  // Start HTML file
  stream << htmlHeader;
  stream << "<style>\n";
  stream << style; // Base styles

  // Collect all unique styles
  UnorderedSet<String> addedStyles;
  for (const LogEntry& entry : m_log.getAllEntries()) {
      const LogLevel* level = entry.getLevel();
      const String& cssStyle = level->getCssStyle();

      if (addedStyles.find(cssStyle) == addedStyles.end()) {
          stream << cssStyle << "\n";
          addedStyles.insert(cssStyle);
      }
  }

  stream << "</style>\n";
  stream << "</head>\n";
  stream << "<body>\n";

  // Header section
  stream << "<div class=\"wrapper\">\n";
  stream << "<h1>Chimera Engine Log</h1>\n";
  stream << "<p>Chimera Engine version: ";
  stream << CH_VERSION_MAJOR << "." << CH_VERSION_MINIOR << "." << CH_VERSION_PATCH;
  stream << "</p>\n";

  // Log entries table
  stream << "<h2>Log Entries</h2>\n";
  stream << "<div class=\"table\">\n";
  stream << "<div class=\"row header\">\n";
  stream << "<div class=\"cell\">Type</div>\n";
  stream << "<div class=\"cell\">Description</div>\n";
  stream << "</div>\n"; // Close header row

  for (const LogEntry& entry : m_log.getAllEntries()) {
    const LogLevel* level = entry.getLevel();

    // Extract class name from CSS
    String className = "row";
    size_t start = level->getCssStyle().find(".row.");
    if (start != String::npos) {
      start += 5; // Skip ".row."
      size_t end = level->getCssStyle().find_first_of(" {", start);
      if (end != String::npos) {
        className += " " + level->getCssStyle().substr(start, end - start);
      }
    }

    // Format message with HTML line breaks
    String formattedMsg = StringUtils::replaceAllSubStr(entry.getLogMessage(), "\n", "<br>");

    // Log entry row
    stream << "<div class=\"" << className << "\">\n";
    stream << "<div class=\"cell\">" << level->getDisplayName() << "</div>\n";
    stream << "<div class=\"cell\">" << formattedMsg << "</div>\n";
    stream << "</div>\n"; // Close row
  }

  // Close all containers
  stream << "</div>\n"; // Close table
  stream << "</div>\n"; // Close wrapper
  stream << htmlFooter;

  // Write to file
  SPtr<DataStream> fileStream = FileSystem::createAndOpenFile(path);
  fileStream << stream.str();
}

/*
*/
void 
Debug::logBacktrace(const LogLevel* level) {
#if USING(CH_ENABLE_BACKTRACE)
  constexpr int MAX_FRAMES = 128;
  if (!level) level = LogLevel::ERROR; // Default level if null
      
#if USING(CH_COMPILER_MSVC)
  void* callstack[MAX_FRAMES];
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  
  WORD frames = CaptureStackBackTrace(0, MAX_FRAMES, callstack, NULL);
  
  String backtraceStr = "Backtrace:\n";
  for (WORD i = 0; i < frames; ++i) {
    DWORD64 address = reinterpret_cast<DWORD64>(callstack[i]);
      
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(
      malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char)));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD64 displacement = 0;
    if (SymFromAddr(process, address, &displacement, symbol)) {
      backtraceStr += StringUtils::format("{0}: {1} - 0x{2:X}\n", 
                                          i, symbol->Name, symbol->Address);
    } 
    else {
      backtraceStr += StringUtils::format("{0}: [Symbol not found] - 0x{1:X}\n", 
                                          i, address);
    }
  
    free(symbol);
  }
  
  logMessage(std::move(backtraceStr), level);
  SymCleanup(process);
  
#elif USING(CH_COMPILER_CLANG)
  void* callstack[MAX_FRAMES];
  int frames = backtrace(callstack, MAX_FRAMES);
  char** strs = backtrace_symbols(callstack, frames);
  
  String backtraceStr = "Backtrace:\n";
  for (int i = 0; i < frames; ++i) {
    backtraceStr += StringUtils::format("{0}: {1}\n", i, strs[i]);
  }
  logMessage(std::move(backtraceStr), level);
  free(strs);
#endif // USING(CH_COMPILER_MSVC)
#endif // USING(CH_BACKTRACE)
}

/*
*/
Debug& 
Debug::getInstance() {
  static Debug* m_instance = nullptr;
  if (!m_instance) {
    m_instance = new Debug();
  }
  return *m_instance;
}

/*
*/
CH_UTILITY_EXPORT Debug&
g_Debug() {
  return Debug::getInstance();
}

} // namespace chEngineSDK


