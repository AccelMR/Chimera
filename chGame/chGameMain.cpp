#include "chGameApplication.h"

using namespace chEngineSDK;

int32
main(int32 argc, ANSICHAR* argv[])
{
  return BaseApplication::launch<GameApplication>(
      argc, argv, {.logFileName = "ChimeraGame.log", .defaultProject = CH_GAME_DEV_PROJECT});
}
