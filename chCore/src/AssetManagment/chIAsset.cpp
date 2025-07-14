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

#include "chFileSystem.h"
#include "chFileStream.h"

#include "chLogger.h"

namespace chEngineSDK {

/*
*/
bool
IAsset::save(){
  const Path assetPath(String(m_metadata.assetPath));
  if (assetPath.empty()) {
    CH_LOG(AssetSystem, Error, "Asset path {0} is empty", assetPath.toString());
    return false;
  }

  SPtr<DataStream> stream = FileSystem::createAndOpenFile(assetPath);
  if (!stream) {
    CH_LOG(AssetSystem, Error, "Failed to create asset file {0}", assetPath.toString());
    return false;
  }

  stream << m_metadata.assetType;
  stream << m_metadata.uuid;

  const uint32 referencedAssetsCount = static_cast<uint32>(m_referencedAssets.size());
  stream << referencedAssetsCount;
  for (const UUID& refUUID : m_referencedAssets) {
    stream << refUUID;
  }

  const bool success = _internalSerialize(stream);
  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to serialize asset {0}", m_metadata.name);
    return false;
  }

  CH_LOG(AssetSystem, Debug, "Asset {0} saved successfully to {1}", m_metadata.name, assetPath.toString());
  return true;
}

/*
*/
bool
IAsset::load(){
  if (isLoaded()) {
    return true;
  }

  const Path assetPath(m_metadata.assetPath);
  if (assetPath.empty() || !FileSystem::exists(assetPath)) {
    CH_LOG(AssetSystem, Error, "Asset path {0} is empty or doesn't exist", assetPath.toString());
    return false;
  }

  m_state = AssetState::Loading;

  SPtr<DataStream> stream = FileSystem::openFile(assetPath);

  if (!stream) {
    CH_LOG(AssetSystem, Error, "Failed to open asset file {0}", assetPath.toString());
    m_state = AssetState::Failed;
    return false;
  }

  AssetMetadata metadata;
  stream >> metadata;

  if (!validateMetadata(metadata)) {
    CH_LOG(AssetSystem, Error, "Invalid asset metadata {0}", m_metadata.name);
    m_state = AssetState::Failed;
    return false;
  }
  m_metadata = metadata;

  uint32 referencedAssetCount = INVALID_UNSIGNED_INDEX;
  stream >> referencedAssetCount;
  if (referencedAssetCount == INVALID_UNSIGNED_INDEX) {
    CH_LOG(AssetSystem, Error, "Invalid asset reference count {0}", referencedAssetCount);
    m_state = AssetState::Failed;
    return false;
  }

  m_referencedAssets.resize(referencedAssetCount);
  for (uint32 i = 0; i < referencedAssetCount; ++i) {
    UUID refUUID = UUID::null();
    stream >> refUUID;
    if (refUUID.isNull()) {
      CH_LOG(AssetSystem, Error, "Invalid asset reference UUID {0}", refUUID.toString());
      m_state = AssetState::Failed;
      return false;
    }
    m_referencedAssets[i] = refUUID;
  }

  const bool success = _internalDeserialize(stream);

  if (!success) {
    CH_LOG(AssetSystem, Error, "Failed to deserialize asset {0}", m_metadata.name);
    m_state = AssetState::Failed;
    return false;
  }

  m_state = AssetState::Loaded;
  CH_LOG(AssetSystem, Debug, "Asset {0} loaded successfully", m_metadata.name);

  return true;
}

/*
*/
bool
IAsset::unload(){
  if (isUnloaded()) {
    return true;
  }

  m_state = AssetState::Unloading;

  // for (const UUID& refUUID : m_referencedAssets) {
  //   //AssetManager::getInstance().unloadAsset(refUUID);
  // }

  //clear the asset data
  clearAssetData();
  m_referencedAssets.clear();

  m_state = AssetState::Unloaded;
  CH_LOG(AssetSystem, Debug, "Asset {0} unloaded successfully", m_metadata.name);

  return true;
}

/*
*/
bool
IAsset::validateMetadata(const AssetMetadata& metadata) const {
  if (metadata.typeName[0] == '\0') { // Check if typeName is empty
    CH_LOG(AssetSystem, Error, "Asset name is empty or 'Unnamed'");
    return false;
  }

  // This probably does not need to be checked since is the original path and can be empty or
  // does not exist since it can be imported in a different machine.
  // if (metadata.originalPath.empty()) {
  //   CH_LOG(AssetSystem, Error, "Original path is empty");
  //   return false;
  // }

  // This is the actual path inside the assets folder, needs to be always in the assets folder.
  if (metadata.assetPath[0] == '\0' || !FileSystem::exists(Path(metadata.assetPath))) {
    CH_LOG(AssetSystem, Error, "Cached path is empty or doesn't exist");
    return false;
  }

  if (metadata.uuid.isNull()) {
    CH_LOG(AssetSystem, Error, "UUID is null");
    return false;
  }

  return true;
}

} // namespace chEngineSDK
