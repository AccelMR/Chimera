#include "chEditorApplication.h"

#include "chCommandLine.h"
#include "chEnginePaths.h"
#include "chException.h"
#include "chLogger.h"
#include "chPath.h"
#include "chStringUtils.h"

using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(EditorMain, All);

int32
main(int32 argc, ANSICHAR* argv[]) {
  Logger::startUp();
  Logger& logger = Logger::instance();
  logger.setConsoleOutput(true);
  logger.setGlobalVerbosity(LogVerbosity::Debug);
  logger.setBufferingEnabled(true, 500);

  CommandLine::initialize(argc, argv);

  try {
    // The log file lives in the project's Saved folder, so the project (-project) must
    // be known before the file is opened.
    EnginePaths::initialize();
    const Path logFile = EnginePaths::getLogDirectory().join(Path("ChimeraEditor.log"));
    logger.setFileOutput(true, logFile.toString());

    CH_LOG_INFO(EditorMain, "Chimera Editor started.");

    BaseApplication::startUp<EditorApplication>();
    // BaseApplication::startUp();
    BaseApplication& app = BaseApplication::instance();
    app.initialize();
    app.run();
    CH_LOG_INFO(EditorMain, "Chimera Editor finished successfully.");

    BaseApplication::shutDown();
  } catch (const Exception& e) {
    CH_LOG_ERROR(EditorMain, "Exception caught: {0}", e.what());
  } catch (...) {
    CH_LOG_ERROR(EditorMain, "Unknown exception caught.");
  }

  Logger::shutDown();

  return 0;
}
