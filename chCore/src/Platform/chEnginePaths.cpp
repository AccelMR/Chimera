/************************************************************************/
/**
 * @file chEnginePaths.cpp
 * @author AccelMR
 * @date 2025/07/15
 * @brief
 *          This file contains the implementation of the EnginePaths class,
 */
/************************************************************************/
#include "chEnginePaths.h"

#include "chFileSystem.h"
#include "chPath.h"

namespace chEngineSDK {

namespace chEnginePathsVars {
static const Path ASSETS_PATH = Path("Assets");
} // namespace chEnginePaths
using namespace chEnginePathsVars;

/*
*/
Path
EnginePaths::getGameAssetDirectory() {
  // Get the absolute path to the assets directory
  return ASSETS_PATH;
}

/*
 */
Path
EnginePaths::getAbsoluteGameAssetDirectory() {
  // Get the absolute path to the assets directory
  static Path assetDir = FileSystem::absolutePath(ASSETS_PATH);
  return assetDir;
}

/*
*/
Path
EnginePaths::getEngineAssetDirectory() {

  static Path engineAssetDir = FileSystem::absolutePath(Path(
    #if USING(CH_DEBUG_MODE)
    "chEditor/assets"
    #else
    "EngineAssets/Release"
    #endif
  ));
  return engineAssetDir;
}

} // namespace chEngineSDK
