/************************************************************************/
/**
 * @file chAssetManager.cpp
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 * AssetManager class for managing assets in the engine.
 * It handles loading, unloading, and saving assets.
 * It also provides functionality for managing asset references and states.
 */
/************************************************************************/

#include "chAssetManager.h"

#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chSupportedImport.h"
#include "chLogger.h"
#include "chPath.h"


#if USING(CH_DEBUG_MODE)
#define CH_ASSETSYSTEM_LOG_LEVEL All
#else
#define CH_ASSETSYSTEM_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)


namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(AssetSystem, CH_ASSETSYSTEM_LOG_LEVEL);

/*
*/
bool 
AssetManager::importAsset(const Path& importPath, const Path& assetPath) {

  if (importPath.empty() || assetPath.empty()) {
    CH_LOG(AssetSystem, Error, "Invalid import or asset path");
    return false;
  }

  if (!FileSystem::isFile(importPath)) {
    CH_LOG(AssetSystem, Error, "Import path is not a file: {0}", importPath);
    return false;
  }

  if (!importPath.exists()) {
    CH_LOG(AssetSystem, Error, "Import path does not exist: {0}", importPath);
    return false;
  }

  //Check if assetPath is inside asset directory
  if (assetPath.isRelative() ) {
    CH_LOG(AssetSystem, Error, "Asset path must be absolute: {0}", assetPath);
    return false;
  }

  const Path fullAssetPath = chEnginePaths::ASSETS_PATH / assetPath;
  const Path parentDir = fullAssetPath.getDirectory();
  if (!parentDir.exists()) {
    if (!FileSystem::createDirectories(parentDir)) {
      CH_LOG(AssetSystem, Error, "Failed to create directory: {0}", parentDir);
      return false;
    }
  }

  CH_LOG(AssetSystem, Info, "Importing asset from {0} to {1}", importPath, assetPath);
  const String extension = assetPath.getExtension();
  for (const auto& [type, extensions] : SUPPORTED_IMPORTS) {
    if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
      switch (type) {
        case AssetType::Model:
          return importModel(importPath, fullAssetPath);
        // Add other asset types here
        default:
          CH_LOG(AssetSystem, Error, "Unsupported asset type: {0}", type);
          return false;
      }
    }
  }

  //Should never reach here
  CH_LOG(AssetSystem, Error, "Unsupported file extension: {0}", extension);
  return false;
}

/******************************************************************************************* */
/*Custom Importers*/
/******************************************************************************************* */
//TODO: this probably can be compiled only in Editor mode, since we don expect to import assets in runtime
/*
*/
bool
AssetManager::importModel(const Path& importPath, const Path& fullAssetPath) {
  return true;
}
} // namespace chEngineSDK

