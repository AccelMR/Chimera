#include "chEditorApplication.h"

using namespace chEngineSDK;

int32
main(int32 argc, ANSICHAR* argv[])
{
  return BaseApplication::launch<EditorApplication>(
      argc, argv, {.logFileName = "ChimeraEditor.log", .logBufferSize = 500});
}
