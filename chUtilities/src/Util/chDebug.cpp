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

#if USING(CH_COMPILER_MSVC)
# include "Win32/chWindows.h"
# include <iostream>

void
logIDEConsole(const chEngineSDK::String& msg) {
  OutputDebugString(msg.c_str());
  OutputDebugString("\n");

  std::cout << msg << std::endl;
}
#else
#include <iostream>

void logIDEConsole(const chEngineSDK::String& msg) 
{
  std::cout << msg << std::endl;
}
#endif

namespace chHTMLDebugConstants{
  static const char* style = R"(
  <style>
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
    .row.red {
      background-color:rgba(163, 19, 19, 0.81);
      color: #ff9999;
    }
    .row.red_dark {
      background-color: #800000;
      color: #ff9999;
    }    
    .row.yellow {
      background-color: #806600;
      color: #fff799;
    }
    .row.green {
      background-color: #004d00;
      color: #99ff99;
    }
    .row.light_gray {
      background-color: #333;
      color: #ccc;
    }
  </style>
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
const Array<String, static_cast<SIZE_T>(LOG_LEVEL::kCOUNT)> LOG_LEVEL_NAMES = {
  "FATAL",
  "ERROR",
  "WARN",
  "INFO",
  "DEBUG"
};

/*
*/
void
Debug::logMessage(const String& msg, LOG_LEVEL level) {
  m_log.logMsg(msg, level);
  logIDEConsole(StringUtils::format("[{0}] {1}", 
                                    LOG_LEVEL_NAMES[static_cast<int32>(level)], 
                                    msg));
}

/*
*/
void
Debug::saveLog(const Path& path) const {
  auto fileStream = FileSystem::createAndOpenFile(path);
  CH_ASSERT(fileStream);
  for (const LogEntry& logEntry : m_log.getAllEntries()) {
  String msg = StringUtils::format("[{0}] {1}",
                                   LOG_LEVEL_NAMES[static_cast<int32>(logEntry.getLevel())], 
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
  stream << style;
  stream << R"(</head><body>)";

  // Header section
  stream << R"(<div class="wrapper">)";
  stream << R"(<h1>Chimera Engine Log</h1>)";
  stream << R"(<p>Chimera Engine version: )";
  stream << CH_VERSION_MAJOR << "." << CH_VERSION_MINIOR << "." << CH_VERSION_PATCH;
  stream << R"(</p>)";

  // Log entries table
  stream << R"(<h2>Log Entries</h2>)";
  stream << R"(<div class="table">)";
  stream << R"(<div class="row header">)";
  stream << R"(<div class="cell">Type</div><div class="cell">Description</div></div>)";

  for (const LogEntry& entry : m_log.getAllEntries()) {
    LOG_LEVEL logChannel = entry.getLevel();

    // Start row with appropriate color based on log level
    if (logChannel == LOG_LEVEL::kERROR) {
      stream << R"(<div class="row red">)";
      stream << R"(<div class="cell">Error</div>)";
    } 
    else if (logChannel == LOG_LEVEL::kFATAL) {
      stream << R"(<div class="row red_dark">)";
      stream << R"(<div class="cell">Fatal</div>)";
    }
    else if (logChannel == LOG_LEVEL::kWARN) {
      stream << R"(<div class="row yellow">)";
      stream << R"(<div class="cell">Warning</div>)";
    } 
    else if (logChannel == LOG_LEVEL::kDEBUG) {
      stream << R"(<div class="row green">)";
      stream << R"(<div class="cell">Debug</div>)";
    }
    else {
      stream << R"(<div class="row light_gray">)";
      stream << R"(<div class="cell">Info</div>)";
    }

    // Escape and format the log message
    String parsedMessage = StringUtils::replaceAllSubStr(entry.getLogMessage(), "\n", "<br>");

    stream << R"(<div class="cell">)" << parsedMessage << R"(</div>)";
    stream << R"(</div>)";
  }

  stream << R"(</div></div></div>)";
  stream << htmlFooter;

  // Write to file
  SPtr<DataStream> fileStream = FileSystem::createAndOpenFile(path);
  fileStream << stream.str();
}

/*
*/
CH_UTILITY_EXPORT Debug&
g_Debug() {
  static Debug debug;
  return debug;
}

}


