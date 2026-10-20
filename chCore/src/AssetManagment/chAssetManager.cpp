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


#include "chAssetFile.h"
#include "chConfigFile.h"
#include "chTypeTraits.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chPath.h"

// Base asset types
#include "chPrefabAsset.h"
#include "chMaterialAsset.h"
#include "chModelAsset.h"
#include "chTextureAsset.h"
#include "chSceneAsset.h"
#include "chSkeletonAsset.h"
#include "chAnimationClipAsset.h"


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
  m_assetRegister->registerAssetCreator<MaterialAsset>();
  m_assetRegister->registerAssetCreator<PrefabAsset>();
  m_assetRegister->registerAssetCreator<SceneAsset>();
  m_assetRegister->registerAssetCreator<SkeletonAsset>();
  m_assetRegister->registerAssetCreator<AnimationClipAsset>();
}

/*
 */
bool
AssetManager::syncLoadAsset(const SPtr<IAsset>& asset) {
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
SPtr<IAsset>
AssetManager::getAsset(const UUID& assetUUID) const
{
  const auto it = m_assets.find(assetUUID);
  return it != m_assets.end() ? it->second : nullptr;
}

/*
*/
bool
AssetManager::syncLoadAsset(const UUID& assetUUID) {
  auto it = m_assets.find(assetUUID);
  if (it == m_assets.end()) {
    CH_LOG_ERROR(AssetSystem, "Asset with UUID {0} not found", assetUUID.toString());
    return false;
  }
  return syncLoadAsset(it->second);

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

  FileSystem::forEachFileChildRecursive(directory, [&](const Path& file) {
    if (file.getExtension() != EnginePaths::getEngineAssetExtension()) {
      return;
    }
    SPtr<IAsset> asset = lazyLoadAssetFile(file);
    if (!asset) {
      CH_LOG_ERROR(AssetSystem, "Failed to lazy load asset from file: {0}", file.toString());
      return;
    }

    m_assets[asset->getUUID()] = asset;
    CH_LOG_DEBUG(AssetSystem, "Lazy loaded asset: {0}", asset->getUUID().toString());
  });
}

/*
 */
SPtr<SceneAsset>
AssetManager::findScene(StringView pathOrName) const
{
  const SIZE_T nameStart = pathOrName.rfind('/');
  const bool bHasFolder = nameStart != StringView::npos;
  const StringView name = bHasFolder ? pathOrName.substr(nameStart + 1) : pathOrName;
  const StringView folder = bHasFolder ? pathOrName.substr(0, nameStart) : StringView();

  for (const auto& [uuid, asset] : m_assets) {
    if (asset->getAssetTypeId() != AssetTypeTraits<SceneAsset>::getTypeId() ||
        StringView(asset->getName()) != name) {
      continue;
    }
    if (!bHasFolder || StringView(asset->getAssetPath()) == folder) {
      return std::static_pointer_cast<SceneAsset>(asset);
    }
  }
  return nullptr;
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

/*
 */
SPtr<IAsset>
AssetManager::findAssetBySource(const Path& sourceFile, StringView sourcePart) const
{
  for (const auto& [uuid, asset] : m_assets) {
    const AssetEditorData& editorData = asset->getEditorData();
    if (editorData.sourceFile == sourceFile && editorData.sourcePart == sourcePart) {
      return asset;
    }
  }
  return nullptr;
}

/*
 */
String
AssetManager::makeUniqueAssetName(const Path& folder, StringView name) const
{
  // Cut first, so the name checked is the one the metadata will keep, with room for a
  // suffix of up to "_999".
  constexpr SIZE_T kMaxNameLength = sizeof(AssetMetadata::name) - 1;
  constexpr SIZE_T kSuffixRoom = 4;
  const String baseName(name.substr(0, kMaxNameLength - kSuffixRoom));

  const auto isTaken = [this, &folder](const String& candidate) {
    for (const auto& [uuid, asset] : m_assets) {
      if (StringUtils::equals(asset->getName(), candidate) &&
          StringUtils::equals(asset->getAssetPath(), folder.toString())) {
        return true;
      }
    }
    String fileName = candidate;
    fileName += EnginePaths::getEngineAssetExtension();
    return FileSystem::exists(folder.join(Path(fileName)));
  };

  if (!isTaken(baseName)) {
    return baseName;
  }
  ANSICHAR numberBuffer[StringUtils::MAX_INTEGER_CHARS];
  for (uint32 suffix = 1; suffix < 1000; ++suffix) {
    String candidate = baseName;
    candidate += '_';
    candidate += StringUtils::toChars(numberBuffer, suffix);
    if (!isTaken(candidate)) {
      return candidate;
    }
  }
  return baseName;
}

#endif // Editor-specific functionality

/*
 */
SPtr<IAsset>
AssetManager::lazyLoadAssetFile(const Path& file) {
  SPtr<DataStream> stream = FileSystem::openFile(file, true);
  if (!stream || !stream->isReadable()) {
    CH_LOG_ERROR(AssetSystem, "Failed to open asset file: {0}", file.toString());
    return nullptr;
  }

  AssetFileInfo info;
  const bool bReadInfo = AssetFile::readInfo(*stream, info);
  stream->close();
  if (!bReadInfo) {
    return nullptr;
  }

  auto it = m_assets.find(info.uuid);
  if (it != m_assets.end()) {
    CH_LOG(AssetSystem, Debug, "Asset {0} already exists, reusing existing instance",
           it->second->getName());
    return it->second;
  }

  AssetMetadata metadata;
  metadata.uuid = info.uuid;
  metadata.assetType = info.assetType;
  const String name = file.getFileName(false);
  const String folder = FileSystem::toVirtualPath(file).getDirectory().toString();
  if (!StringUtils::copyToBuffer(metadata.name, name) ||
      !StringUtils::copyToBuffer(metadata.assetPath, folder)) {
    CH_LOG_ERROR(AssetSystem, "Asset file name or folder is too long: {0}", file);
    return nullptr;
  }

  CH_ASSERT(m_assetRegister);
  AssetCreatorFunc creator = m_assetRegister->getAssetCreator(metadata.assetType);
  if (!creator) {
    CH_LOG_ERROR(AssetSystem, "No asset creator found for type: {0}",
                 metadata.assetType.toString());
    return nullptr;
  }

  if (!IAsset::validateMetadata(metadata)) {
    CH_LOG_ERROR(AssetSystem, "Invalid asset metadata: {0}", metadata.name);
    return nullptr;
  }
  SPtr<IAsset> asset = creator(metadata);

#if USING(CH_EDITOR)
  ConfigFile metaFile;
  if (metaFile.load(AssetMeta::getPath(file))) {
    AssetMeta::read(metaFile, EnginePaths::getProjectDirectory(), asset->m_editorData);
    asset->readEditorValues(metaFile);
  }
#endif // USING(CH_EDITOR)

  return asset;
}

} // namespace chEngineSDK
