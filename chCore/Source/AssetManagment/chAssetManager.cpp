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
#include "chScriptAsset.h"


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
  m_assetRegister->registerAssetCreator<ScriptAsset>();
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

#if USING(CH_EDITOR)
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
#endif // USING(CH_EDITOR)

/*
 */
void
AssetManager::lazyLoadAssetsFromDirectory(const Path& directory) {
  if (!FileSystem::isDirectory(directory)) {
    CH_LOG_ERROR(AssetSystem, "Directory does not exist: {0}", directory.toString());
    return;
  }

  FileSystem::forEachFileChildRecursive(directory, [&](const Path& file) {
    const String extension = file.getExtension();
    SPtr<IAsset> asset;
    if (extension == EnginePaths::getEngineAssetExtension()) {
      asset = lazyLoadAssetFile(file);
    }
    else if (extension == ScriptAsset::EXTENSION) {
      asset = lazyLoadScriptFile(file);
    }
    else {
      return;
    }
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

/*
 */
bool
AssetManager::moveAsset(const SPtr<IAsset>& asset, const Path& folder)
{
  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Cannot move null asset");
    return false;
  }
  if (!asset->moveTo(folder)) {
    return false;
  }
  CH_LOG_DEBUG(AssetSystem, "Moved asset {0} to {1}", asset->getName(), folder);
  return true;
}

/*
 */
bool
AssetManager::moveFolder(const Path& folder, const Path& newFolder)
{
  const String& oldText = folder.toString();
  const String& newText = newFolder.toString();
  const auto isInside = [](StringView path, StringView base) {
    return path == base ||
           (path.size() > base.size() && path.starts_with(base) && path[base.size()] == '/');
  };

  if (oldText == EnginePaths::getGameAssetDirectory().toString()) {
    CH_LOG_ERROR(AssetSystem, "The asset root folder cannot be moved");
    return false;
  }
  if (isInside(newText, oldText)) {
    CH_LOG_ERROR(AssetSystem, "Cannot move {0} into itself", folder);
    return false;
  }

  // Checked before touching the disk, so files and metadata never disagree.
  for (const auto& [uuid, asset] : m_assets) {
    const StringView assetFolder = asset->getAssetPath();
    if (isInside(assetFolder, oldText) &&
        newText.size() + assetFolder.size() - oldText.size() >=
            sizeof(AssetMetadata::assetPath)) {
      CH_LOG_ERROR(AssetSystem, "Moving {0} to {1} makes the folder of {2} too long", folder,
                   newFolder, asset->getName());
      return false;
    }
  }

  if (!FileSystem::renameDirectory(folder, newFolder)) {
    CH_LOG_ERROR(AssetSystem, "Failed to move folder {0} to {1}", folder, newFolder);
    return false;
  }

  String movedFolder;
  for (const auto& [uuid, asset] : m_assets) {
    const StringView assetFolder = asset->getAssetPath();
    if (!isInside(assetFolder, oldText)) {
      continue;
    }
    movedFolder = newText;
    movedFolder += assetFolder.substr(oldText.size());
    StringUtils::copyToBuffer(asset->m_metadata.assetPath, movedFolder);
  }
  CH_LOG_DEBUG(AssetSystem, "Moved folder {0} to {1}", folder, newFolder);
  return true;
}

/*
 */
bool
AssetManager::removeFolder(const Path& folder)
{
  const String& folderText = folder.toString();
  const auto isInside = [&folderText](StringView path) {
    return path == folderText || (path.size() > folderText.size() &&
                                  path.starts_with(folderText) &&
                                  path[folderText.size()] == '/');
  };

  if (folderText == EnginePaths::getGameAssetDirectory().toString()) {
    CH_LOG_ERROR(AssetSystem, "The asset root folder cannot be deleted");
    return false;
  }
  Vector<UUID> inside;
  for (const auto& [uuid, asset] : m_assets) {
    if (!isInside(asset->getAssetPath())) {
      continue;
    }
    if (!asset->isUnloaded()) {
      CH_LOG_ERROR(AssetSystem, "Cannot delete {0}: {1} is in use", folder, asset->getName());
      return false;
    }
    inside.push_back(uuid);
  }

  const bool bRemoved = FileSystem::removeAll(folder);
  // A failure may have deleted part of the files, so each asset is checked on its own.
  for (const UUID& uuid : inside) {
    const auto it = m_assets.find(uuid);
    if (!FileSystem::exists(it->second->getFilePath())) {
      m_assets.erase(it);
    }
  }
  if (!bRemoved) {
    CH_LOG_ERROR(AssetSystem, "Failed to delete folder {0}", folder);
    return false;
  }
  CH_LOG_DEBUG(AssetSystem, "Deleted folder {0} with {1} assets", folder, inside.size());
  return true;
}

#endif // Editor-specific functionality

/*
 */
SPtr<IAsset>
AssetManager::lazyLoadScriptFile(const Path& file)
{
  SPtr<DataStream> stream = FileSystem::openFile(file, true);
  if (!stream || !stream->isReadable()) {
    CH_LOG_ERROR(AssetSystem, "Failed to open script file: {0}", file);
    return nullptr;
  }
  // As text, so a file saved as UTF-16 or with a byte order mark reads the same.
  String source = stream->getAsString();
  stream->close();

  UUID uuid = UUID::null();
  if (!ScriptAsset::readUUIDLine(source, uuid)) {
    uuid = UUID::null();
  }

  const Path virtualFile = FileSystem::toVirtualPath(file);
  bool bTaken = false;
  if (!uuid.isNull()) {
    const auto it = m_assets.find(uuid);
    if (it != m_assets.end()) {
      if (it->second->getFilePath() == virtualFile) {
        return it->second;
      }
      // A copied script brings the UUID line of the original along.
      bTaken = true;
    }
  }

#if USING(CH_EDITOR)
  const bool bNeedsUUID = uuid.isNull() || bTaken;
  if (bNeedsUUID) {
    if (bTaken) {
      CH_LOG_WARNING(AssetSystem, "Script {0} has the UUID of another script; it gets a new one.",
                     virtualFile);
    }
    uuid = UUID::createRandom();
  }
#else
  if (uuid.isNull() || bTaken) {
    CH_LOG_ERROR(AssetSystem,
                 "Script {0} has no UUID line of its own and is skipped. Open the project in "
                 "the editor, which writes one.",
                 virtualFile);
    return nullptr;
  }
#endif // USING(CH_EDITOR)

  AssetMetadata metadata;
  metadata.uuid = uuid;
  metadata.assetType = AssetTypeTraits<ScriptAsset>::getTypeId();
  if (!StringUtils::copyToBuffer(metadata.name, file.getFileName(false)) ||
      !StringUtils::copyToBuffer(metadata.assetPath, virtualFile.getDirectory().toString())) {
    CH_LOG_ERROR(AssetSystem, "Script file name or folder is too long: {0}", file);
    return nullptr;
  }

#if USING(CH_EDITOR)
  if (bNeedsUUID) {
    // Saving writes the UUID line into the file.
    SPtr<ScriptAsset> asset = chMakeShared<ScriptAsset>(metadata, std::move(source));
    asset->m_editorData = AssetMeta::makeNew(Path(), StringView(), ImportSettings{});
    if (!asset->save()) {
      CH_LOG_WARNING(AssetSystem, "Could not write the UUID line of script {0}", virtualFile);
    }
    return asset;
  }
#endif // USING(CH_EDITOR)

  SPtr<ScriptAsset> asset = chMakeShared<ScriptAsset>(metadata);
#if USING(CH_EDITOR)
  asset->m_editorData = AssetMeta::makeNew(Path(), StringView(), ImportSettings{});
#endif // USING(CH_EDITOR)
  return asset;
}
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
  Vector<UUID> references;
  // Only the editor reads its data; the game moves past it.
  String editorData;
  String* editorDataTarget = nullptr;
#if USING(CH_EDITOR)
  editorDataTarget = &editorData;
#endif // USING(CH_EDITOR)
  const bool bReadInfo = AssetFile::readInfo(*stream, info) &&
                         AssetFile::readReferences(*stream, references) &&
                         AssetFile::readEditorData(*stream, editorDataTarget);
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
  asset->m_referencedAssets = std::move(references);

#if USING(CH_EDITOR)
  ConfigFile editorFile;
  editorFile.parse(editorData, file.toString());
  AssetMeta::read(editorFile, EnginePaths::getProjectDirectory(), asset->m_editorData);
  asset->readEditorValues(editorFile);
#endif // USING(CH_EDITOR)

  return asset;
}

} // namespace chEngineSDK
