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

#include "chAssetManager.h"

#include "chEnginePaths.h"
#include "chFileStream.h"
#include "chFileSystem.h"

#include "chLogger.h"

namespace chEngineSDK {

/*
*/
void
IAsset::setAssetPath(const ANSICHAR* assetPath) {
  if (!assetPath || !*assetPath) {
    CH_LOG(AssetSystem, Error, "Invalid asset path for asset {0}", m_metadata.name);
    return;
  }
  SIZE_T pathLength = chString::length(assetPath);
  chString::copyANSI(m_metadata.assetPath, assetPath, pathLength + 1);
  CH_LOG(AssetSystem, Debug, "Set asset path for asset {0} to {1}", m_metadata.name,
         m_metadata.assetPath);
  //updateMetadata(m_metadata);
}


/*
 */
bool
IAsset::save() {
  const Path assetPath(String(m_metadata.assetPath));
  if (assetPath.empty() || !assetPath.isRelative()) {
    CH_LOG(AssetSystem, Error, "Asset path {0} is empty", assetPath.toString());
    return false;
  }

  String assetName(m_metadata.name);
  assetName += EnginePaths::getEngineAssetExtension();
  const Path fullFilePath = assetPath.join(Path(assetName));
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(fullFilePath);
  if (!stream) {
    CH_LOG(AssetSystem, Error, "Failed to create asset file {0}", assetPath.toString());
    return false;
  }

  stream->write(static_cast<const void*>(&m_metadata), sizeof(AssetMetadata));

  //TODO: Write referenced assets count

  const bool success = serialize(stream);
  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to serialize asset {0}", m_metadata.name);
    return false;
  }

  stream->close();

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

  if (chString::equals(m_metadata.name, newName)) {
    CH_LOG(AssetSystem, Warning, "Asset {0} already has the name {1}", m_metadata.name,
           newName);
    return true;
  }

  String assetFullStr = String(m_metadata.assetPath) + "/" + String(m_metadata.name) + EnginePaths::getEngineAssetExtension();
  const Path assetAbsPath(FileSystem::absolutePath(Path(assetFullStr)));

  String newFullStr = String(m_metadata.assetPath) + "/" + String(newName) + EnginePaths::getEngineAssetExtension();
  const Path newAssetPath(FileSystem::absolutePath(Path(newFullStr)));

  const bool success = FileSystem::renameFile(assetAbsPath,
                                              newAssetPath);
  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to rename asset {0} to {1}", m_metadata.name, newName);
    return false;
  }

  // Update the metadata
  chString::copyANSI(m_metadata.name, newName);

  return updateMetadata(m_metadata);
}

/*
 */
bool
IAsset::updateMetadata(const AssetMetadata& newMetadata) {
  const Path assetPath(String(m_metadata.assetPath));
  if (assetPath.empty() || !assetPath.isRelative()) {
    return false;
  }

  String assetName(m_metadata.name);
  assetName += EnginePaths::getEngineAssetExtension();
  const Path fullFilePath = assetPath.join(Path(assetName));

  // Open file in read/write mode instead of create mode
  SPtr<DataStream> stream = FileSystem::openFile(fullFilePath, false); // read/write
  if (!stream) {
    return false;
  }

  // Write only metadata at the beginning
  stream->seek(0); // Go to start
  stream->write(&newMetadata, sizeof(AssetMetadata));
  stream->close();

  m_metadata = newMetadata;
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

  const Path fullFilePath = assetPath.join(Path(String(m_metadata.name) + EnginePaths::getEngineAssetExtension()));
  SPtr<DataStream> stream = FileSystem::openFile(fullFilePath, true);

  if (!stream || !stream->isReadable()) {
    CH_LOG(AssetSystem, Error, "Failed to open asset file {0}", assetPath.toString());
    m_state = AssetState::Failed;
    return false;
  }
  AssetMetadata metadata;
  stream->read(static_cast<void*>(&metadata), sizeof(metadata));

  if (!validateMetadata(metadata)) {
    CH_LOG(AssetSystem, Error, "Invalid asset metadata {0}", m_metadata.name);
    m_state = AssetState::Failed;
    return false;
  }
  m_metadata = metadata;

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
IAsset::validateMetadata(const AssetMetadata& metadata) const {
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
