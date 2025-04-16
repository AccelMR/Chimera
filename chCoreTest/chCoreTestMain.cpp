#include "chBaseApplication.h"
#include "chLogger.h"
#include "chPath.h"

#include <format>


using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(CoreTestMain, All);

int main(int argc, char** argv) {
  try {
    Logger::startUp();
    BaseApplication::startUp();
    Logger::instance().setConsoleOutput(true);
    Logger::instance().setFileOutput(true, "resources/engine/logs/chimeraTest.log");

    auto& app = BaseApplication::instance();
    app.initialize(argc, argv);
    app.run();
    // g_Debug().saveLogAsHtml("resources/engine/logs/chCoreTestMain.html");

    BaseApplication::shutDown();
    Logger::shutDown();
  }
  catch(const std::exception& e)
  {
    CH_LOG_ERROR(CoreTestMain, e.what());
    
    // g_Debug().logBacktrace();
    // g_Debug().saveLog("resources/engine/logs/CRASHED_chCoreTestMain.txt");
    // g_Debug().saveLogAsHtml("resources/engine/logs/CRASHED_chCoreTestMain.html");

    Logger::shutDown();
    return 0;
  }
  catch(...) {
    std::cerr << "Unknown exception caught" << std::endl;
  }
  return 1;
}
