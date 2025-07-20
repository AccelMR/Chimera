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
#include "chTextureAsset.h"

// Base asset types
#include "chModelAsset.h"

namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(AssetSystem, All);

namespace AssetManagerUtils {} // namespace AssetManagerUtils
using namespace AssetManagerUtils;

/*
 */
void
AssetManager::initialize() {
  CH_LOG_DEBUG(AssetSystem, "Initializing AssetManager");

  m_assetRegister = chMakeShared<AssetRegister>();

  // Register asset creators
  m_assetRegister->registerAssetCreator<ModelAsset>();
  m_assetRegister->registerAssetCreator<TextureAsset>();
}

/*
 */
bool
AssetManager::loadAsset(const SPtr<IAsset>& asset) {
  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Cannot load null asset");
    return false;
  }
  if (asset->isLoaded()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is already loaded", asset->getName());
    return true;
  }
  if (asset->isLoading()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is already loading", asset->getName());
    return false;
  }
  if (asset->isUnloading()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is currently unloading", asset->getName());
    return false;
  }
  if (asset->isUnloaded()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is unloaded, loading now", asset->getName());
    asset->m_state = AssetState::Loading;
  }
  else if (asset->isFailed()) {
    CH_LOG_ERROR(AssetSystem, "Asset {0} has failed state, cannot load", asset->getName());
    return false;
  }

  CH_LOG_DEBUG(AssetSystem, "Loading asset {0}", asset->getName());
  if (!asset->load()) {
    CH_LOG_ERROR(AssetSystem, "Failed to load asset {0}", asset->getName());
    asset->m_state = AssetState::Failed;
    return false;
  }
  m_loadedAssets[asset->getUUID()] = asset;

  CH_LOG_DEBUG(AssetSystem, "Asset {0} loaded successfully", asset->getName());

  return true;
}

/*
*/
bool
AssetManager::unloadAsset(const SPtr<IAsset>& asset) {
  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Cannot unload null asset");
    return false;
  }

  if (asset->isUnloaded()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is already unloaded", asset->getName());
    return true;
  }

  if (asset->isLoading()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is currently loading, cannot unload",
                 asset->getName());
    return false;
  }
  if (asset->isUnloading()) {
    CH_LOG_DEBUG(AssetSystem, "Asset {0} is currently unloading", asset->getName());
    return false;
  }

  CH_LOG_DEBUG(AssetSystem, "Unloading asset {0}", asset->getName());
  if (!asset->unload()) {
    CH_LOG_ERROR(AssetSystem, "Failed to unload asset {0}", asset->getName());
    return false;
  }
  m_loadedAssets.erase(asset->getUUID());
  return true;
}

/*
*/
bool
AssetManager::loadAsset(const UUID& assetUUID) {
  auto it = m_assets.find(assetUUID);
  if (it == m_assets.end()) {
    CH_LOG_ERROR(AssetSystem, "Asset with UUID {0} not found", assetUUID.toString());
    return false;
  }
  return loadAsset(it->second);

}

/*
 */
bool
AssetManager::unloadAsset(const UUID& assetUUID) {
  auto it = m_loadedAssets.find(assetUUID);
  if (it == m_loadedAssets.end()) {
    CH_LOG_ERROR(AssetSystem, "Asset with UUID {0} not found in loaded assets",
                 assetUUID.toString());
    return false;
  }

  SPtr<IAsset> asset = it->second;
  return unloadAsset(asset);
}

/*
 */
bool
AssetManager::renameAsset(const UUID& assetUUID, const ANSICHAR* newName) {
  auto it = m_assets.find(assetUUID);
  if (it == m_assets.end()) {
    CH_LOG_ERROR(AssetSystem, "Asset with UUID {0} not found", assetUUID.toString());
    return false;
  }
  SPtr<IAsset> asset = it->second;
  return renameAsset(asset, newName);
}

/*
 */
bool
AssetManager::renameAsset(const SPtr<IAsset>& asset, const ANSICHAR* newName) {
  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Cannot rename null asset");
    return false;
  }

  if (!asset->rename(newName)) {
    CH_LOG_ERROR(AssetSystem, "Failed to rename asset {0} to {1}", asset->getName(), newName);
    return false;
  }

  CH_LOG_DEBUG(AssetSystem, "Renamed asset {0} to {1}", asset->getName(), newName);
  return true;
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
  // FileSystem::getChildren(directory, assetFiles, assetDirectories);

  FileSystem::forEachFileChildRecursive(directory, [&](const Path& file) {
    if (file.getExtension() != ".chAss") {
      CH_LOG_DEBUG(AssetSystem, "Skipping non-asset file: {0}", file.toString());
      return;
    }
    SPtr<DataStream> stream = FileSystem::openFile(file, true);
    if (!stream) {
      CH_LOG_ERROR(AssetSystem, "Failed to open asset file: {0}", file.toString());
      return;
    }
    SPtr<IAsset> asset = lazyDeserialize(stream);
    if (!asset) {
      CH_LOG_ERROR(AssetSystem, "Failed to lazy load asset from file: {0}", file.toString());
      return;
    }
    stream->close();

    // Check if asset metadata asset path and this path are the same
    const Path assetPath = FileSystem::absolutePath(Path(asset->getAssetPath()));

    String relativePath = file.toString();
    auto pos = relativePath.find("Assets/");
    if (pos != String::npos) {
      relativePath = relativePath.substr(pos);
    }
    //Remove Filename and extension
    pos = relativePath.find_last_of('/');
    if (pos != String::npos) {
      relativePath = relativePath.substr(0, pos);
    }

    if (!chString::compare(asset->getAssetPath(), relativePath)) {
      CH_LOG_WARNING(AssetSystem,
                       "Asset path mismatch for {0}: expected {1}, found {2}.\n"
                       "Will update asset path to match file location.",
                       asset->getName(),
                       assetPath,
                       file.toString());

      asset->setAssetPath(relativePath.c_str());
      //TODO: this is nasty fix it.
      // we can probably set a dirtyu flag and then save it later?
      // Main issue ius that we are soft loading here so model data does not exist yet.
      // This is a hacky fix, we should probably have a better way to handle this
      asset->updateMetadata(asset->m_metadata);
    }

    m_assets[asset->getUUID()] = asset;
    CH_LOG_DEBUG(AssetSystem, "Lazy loaded asset: {0}", asset->getUUID().toString());
  });
}

#if USING(CH_EDITOR)
/*
 */
bool
AssetManager::removeAsset(const UUID& assetUUID) {
  auto it = m_assets.find(assetUUID);
  if (it == m_assets.end()) {
    CH_LOG_ERROR(AssetSystem, "Asset with UUID {0} not found", assetUUID.toString());
    return false;
  }

  SPtr<IAsset> asset = it->second;
  if (asset->isLoaded() && !asset->unload()) {
    CH_LOG_ERROR(AssetSystem, "Failed to unload asset {0} before removal", asset->getName());
    return false;
  }

  m_assets.erase(it);
  CH_LOG_DEBUG(AssetSystem, "Removed asset: {0}", asset->getName());
  return true;
}

#endif // Editor-specific functionality

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
    CH_LOG_ERROR(AssetSystem, "No asset creator found for type: {0}",
                 metadata.assetType.toString());
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
