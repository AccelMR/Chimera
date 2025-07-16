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

#include "chAssetTypeTraits.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chPath.h"
#include "chSupportedImport.h"

// Base asset types
#include "chModelAsset.h"

namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(AssetSystem, All);

namespace AssetManagerUtils {
} // namespace AssetManagerUtils
using namespace AssetManagerUtils;

/*
 */
void
AssetManager::initialize() {
  CH_LOG_DEBUG(AssetSystem, "Initializing AssetManager");

  m_assetRegister = chMakeShared<AssetRegister>();

  // Register asset creators
  m_assetRegister->registerAssetCreator<ModelAsset>(AssetTypeTraits<ModelAsset>::getTypeId());

  m_onRegisterAsset(m_assetRegister);
}

/*
*/
void
AssetManager::lazyLoadAssetsFromDirectory(const Path& directory) {
  if (!FileSystem::isDirectory(directory)) {
    CH_LOG_ERROR(AssetSystem, "Directory does not exist: {0}", directory.toString());
    return;
  }

  Vector<Path> assetFiles;
  Vector<Path> assetDirectories;
  FileSystem::getChildren(directory, assetFiles, assetDirectories);
  for (const Path& file : assetFiles) {
    if (file.getExtension() != ".chAss") {
      CH_LOG_DEBUG(AssetSystem, "Skipping non-asset file: {0}", file.toString());
      continue;
    }
      SPtr<DataStream> stream = FileSystem::openFile(file, true);
      if (!stream) {
        CH_LOG_ERROR(AssetSystem, "Failed to open asset file: {0}", file.toString());
        continue;
      }
      SPtr<IAsset> asset = lazyDeserialize(stream);
      if (!asset) {
        CH_LOG_ERROR(AssetSystem, "Failed to lazy load asset from file: {0}", file.toString());
        continue;
      }

      m_assets[asset->getUUID()] = asset;
      CH_LOG_DEBUG(AssetSystem, "Lazy loaded asset: {0}", asset->getUUID().toString());
  }
}

/*
*/
SPtr<IAsset>
AssetManager::lazyDeserialize(const SPtr<DataStream>& stream) {
  if (!stream || !stream->isReadable()) {
    CH_LOG_ERROR(AssetSystem, "Invalid data stream for deserializing asset");
    return nullptr;
  }

  AssetMetadata metadata;
  if (!stream->read(reinterpret_cast<void*>(&metadata), sizeof(AssetMetadata))) {
    CH_LOG_ERROR(AssetSystem, "Failed to deserialize asset metadata from stream");
    return nullptr;
  }

  auto it = m_assets.find(metadata.uuid);
  if (it != m_assets.end()) {
    CH_LOG(AssetSystem, Debug, "Asset {0} already exists, reusing existing instance",
           metadata.name);
    return it->second;
  }

  CH_ASSERT(m_assetRegister);
  AssetCreatorFunc creator = m_assetRegister->getAssetCreator(metadata.assetType);
  if (!creator) {
    CH_LOG_ERROR(AssetSystem, "No asset creator found for type: {0}", metadata.assetType.toString());
    return nullptr;
  }

  SPtr<IAsset> asset = creator(metadata);
  if (!asset->validateMetadata(metadata)) {
    CH_LOG_ERROR(AssetSystem, "Invalid asset metadata: {0}", metadata.name);
    return nullptr;
  }

  return asset;
}
} // namespace chEngineSDK
