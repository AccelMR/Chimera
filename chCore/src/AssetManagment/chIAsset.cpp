/************************************************************************/
/**
 * @file chIAsset.cpp
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This is a base class for all assets in the engine.
 *   It provides a common interface for loading, saving, and managing assets.
 */
/************************************************************************/

#include "chIAsset.h"

#include "chAssetFile.h"
#include "chAssetManager.h"

#include "chEnginePaths.h"
#include "chFileStream.h"
#include "chFileSystem.h"

#include "chLogger.h"

namespace chEngineSDK {
namespace {
Path
assetFilePath(const Path& folder, StringView assetName)
{
  const StringView extension = EnginePaths::getEngineAssetExtension();
  String fileName;
  fileName.reserve(assetName.size() + extension.size());
  fileName.append(assetName);
  fileName.append(extension);
  return folder.join(Path(std::move(fileName)));
}
} // namespace

/*
 */
const ANSICHAR*
IAsset::getTypeName() const
{
  return AssetManager::instance().getAssetTypeName(m_metadata.assetType).c_str();
}

/*
 */
Path
IAsset::getFilePath() const
{
  return assetFilePath(Path(m_metadata.assetPath), m_metadata.name);
}

/*
 */
bool
IAsset::save() {
  const Path assetPath(m_metadata.assetPath);
  if (!FileSystem::isVirtual(assetPath)) {
    CH_LOG(AssetSystem, Error, "Asset path '{0}' must be a virtual path such as /Game",
           assetPath);
    return false;
  }

  const Path fullFilePath = assetFilePath(assetPath, m_metadata.name);
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(fullFilePath);
  if (!stream) {
    CH_LOG(AssetSystem, Error, "Failed to create asset file {0}", assetPath.toString());
    return false;
  }

  m_referencedAssets.clear();
  collectReferences(m_referencedAssets);
  AssetFile::writeStart(*stream, m_metadata.uuid, m_metadata.assetType, m_referencedAssets);

  const bool success = serialize(stream);
  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to serialize asset {0}", m_metadata.name);
    return false;
  }

  stream->close();

#if USING(CH_EDITOR)
  if (!AssetMeta::save(fullFilePath, m_editorData)) {
    CH_LOG(AssetSystem, Warning, "Failed to write the .meta file of asset {0}",
           m_metadata.name);
  }
#endif // USING(CH_EDITOR)

  CH_LOG(AssetSystem, Debug, "Asset {0} saved successfully to {1}", m_metadata.name,
         assetPath.toString());
  return true;
}

/*
 */
bool
IAsset::rename(const ANSICHAR* newName) {
  if (!newName || !*newName) {
    CH_LOG(AssetSystem, Error, "Invalid new name for asset {0}", m_metadata.name);
    return false;
  }

  if (StringUtils::equals(m_metadata.name, newName)) {
    CH_LOG(AssetSystem, Warning, "Asset {0} already has the name {1}", m_metadata.name,
           newName);
    return true;
  }

  // Checked before touching the file, so the file name and the stored name stay equal.
  if (StringView(newName).size() >= sizeof(m_metadata.name)) {
    CH_LOG(AssetSystem, Error, "New name for asset {0} is longer than {1} characters",
           m_metadata.name, sizeof(m_metadata.name) - 1);
    return false;
  }

  const Path assetPath(m_metadata.assetPath);
  const Path oldFile = assetFilePath(assetPath, m_metadata.name);
  const Path newFile = assetFilePath(assetPath, newName);
  if (!FileSystem::renameFile(oldFile, newFile)) {
    CH_LOG(AssetSystem, Error, "Failed to rename asset {0} to {1}", m_metadata.name, newName);
    return false;
  }

#if USING(CH_EDITOR)
  const Path oldMeta = AssetMeta::getPath(oldFile);
  if (FileSystem::isFile(oldMeta) &&
      !FileSystem::renameFile(oldMeta, AssetMeta::getPath(newFile))) {
    CH_LOG(AssetSystem, Warning, "Failed to rename the .meta file of asset {0}",
           m_metadata.name);
  }
#endif // USING(CH_EDITOR)

  StringUtils::copyToBuffer(m_metadata.name, newName);
  return true;
}

/*
 */
bool
IAsset::load() {
  if (isLoaded()) {
    return true;
  }

  const Path assetPath(m_metadata.assetPath);
  if (assetPath.empty() || !FileSystem::exists(assetPath)) {
    CH_LOG(AssetSystem, Error, "Asset path {0} is empty or doesn't exist",
           assetPath.toString());
    return false;
  }

  m_state = AssetState::Loading;

  const Path fullFilePath = assetFilePath(assetPath, m_metadata.name);
  SPtr<DataStream> stream = FileSystem::openFile(fullFilePath, true);

  if (!stream || !stream->isReadable()) {
    CH_LOG(AssetSystem, Error, "Failed to open asset file {0}", assetPath.toString());
    m_state = AssetState::Failed;
    return false;
  }
  AssetFileInfo info;
  if (!AssetFile::readInfo(*stream, info) || info.uuid != m_metadata.uuid ||
      info.assetType != m_metadata.assetType ||
      !AssetFile::readReferences(*stream, m_referencedAssets)) {
    CH_LOG(AssetSystem, Error, "Invalid asset file {0}", m_metadata.name);
    m_state = AssetState::Failed;
    return false;
  }

  // Loaded first, so deserialize finds the assets it points to already loaded.
  AssetManager& assetManager = AssetManager::instance();
  for (const UUID& reference : m_referencedAssets) {
    const SPtr<IAsset> referenced = assetManager.getAsset(reference);
    if (!referenced) {
      CH_LOG(AssetSystem, Warning, "Asset {0} references {1}, which does not exist",
             m_metadata.name, reference);
      continue;
    }
    if (!assetManager.syncLoadAsset(referenced)) {
      CH_LOG(AssetSystem, Warning, "Asset {0} could not load its reference {1}",
             m_metadata.name, referenced->getName());
    }
  }

  const bool success = deserialize(stream);

  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to deserialize asset {0}", m_metadata.name);
    m_state = AssetState::Failed;
    return false;
  }

  m_state = AssetState::Loaded;
  return true;
}

/*
 */
bool
IAsset::unload() {
  if (isUnloaded()) {
    return true;
  }

  m_state = AssetState::Unloading;
  // clear the asset data
  clearAssetData();
  m_referencedAssets.clear();

  m_state = AssetState::Unloaded;
  return true;
}

/*
 */
bool
IAsset::validateMetadata(const AssetMetadata& metadata) {
  bool isValid = true;
  const auto validateAsset = [&isValid](bool condition, const String& errorMessage) {
    if (!condition) {
      CH_LOG(AssetSystem, Error, errorMessage);
      isValid = isValid && false;
    }
  };
  validateAsset(!metadata.uuid.isNull(), "Asset UUID cannot be null");
  validateAsset(!metadata.assetType.isNull(), "Asset type UUID cannot be null");

  return isValid;
}

} // namespace chEngineSDK
