#include "chCommandParser.h"
#include "chException.h"
#include "chLogger.h"
#include "chStringUtils.h"
#include "chWindowedApplication.h"

using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(EditorMain, All);

int32 main(int32 argc, char* argv[])
{
  Logger::startUp();
  Logger& logger = Logger::instance();
  logger.setConsoleOutput(true);
  logger.setFileOutput(true, "resources/engine/logs/ChimeraEditor.log");
  logger.setGlobalVerbosity(LogVerbosity::Debug);

  CH_LOG_INFO(EditorMain, "Chimera Editor started.");

  CommandParser::startUp();
  CommandParser& commandParser = CommandParser::instance();
  commandParser.parse(argc, argv);

  try {
    BaseApplication::startUp<WindowedApplication>();
    //BaseApplication::startUp();
    BaseApplication& app = BaseApplication::instance();
    app.initialize();
    app.run();
    CH_LOG_INFO(EditorMain, "Chimera Editor finished successfully.");

    BaseApplication::shutDown();
  }
  catch(const Exception& e) {
    CH_LOG_ERROR(EditorMain, "Exception caught: {0}", e.what());
  }
  catch(...) {
    CH_LOG_ERROR(EditorMain, "Unknown exception caught.");
  }

  Logger::shutDown();

  return 0;
}
