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
EnginePaths::getAssetDirectory() {
  // Get the absolute path to the assets directory
  return ASSETS_PATH;
}

/*
 */
Path
EnginePaths::getAbsoluteAssetFolder() {
  // Get the absolute path to the assets directory
  static Path assetDir = FileSystem::absolutePath(ASSETS_PATH);
  return assetDir;
}

} // namespace chEngineSDK
