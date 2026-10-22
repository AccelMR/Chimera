#include "chEditorApplication.h"
#include "chEnginePaths.h"
#include "chProjectBrowserApplication.h"

using namespace chEngineSDK;

int32
main(int32 argc, ANSICHAR* argv[])
{
  return BaseApplication::launch(
      argc, argv, {.logFileName = "ChimeraEditor.log", .bProjectOptional = true}, [] {
        // Without -project the editor first asks for one.
        if (EnginePaths::hasProject()) {
          BaseApplication::startUp<EditorApplication>();
        }
        else {
          BaseApplication::startUp<ProjectBrowserApplication>();
        }
      });
}
