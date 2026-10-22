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

#include "chConfigFile.h"
#include "chEnginePaths.h"
#include "chFileStream.h"
#include "chFileSystem.h"

#include "chLogger.h"

namespace chEngineSDK {
namespace {
Path
assetFilePath(const Path& folder, StringView assetName, StringView extension)
{
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
  return assetFilePath(Path(m_metadata.assetPath), m_metadata.name, getFileExtension());
}

/*
 */
StringView
IAsset::getFileExtension() const
{
  return EnginePaths::getEngineAssetExtension();
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

  const Path fullFilePath = assetFilePath(assetPath, m_metadata.name, getFileExtension());
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(fullFilePath);
  if (!stream) {
    CH_LOG(AssetSystem, Error, "Failed to create asset file {0}", assetPath.toString());
    return false;
  }

  m_referencedAssets.clear();
  if (!isPlainFile()) {
    collectReferences(m_referencedAssets);
    String editorData;
#if USING(CH_EDITOR)
    editorData = makeEditorDataText();
#endif // USING(CH_EDITOR)
    AssetFile::writeStart(*stream, m_metadata.uuid, m_metadata.assetType, m_referencedAssets,
                          editorData);
  }

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

#if USING(CH_EDITOR)
/*
 */
String
IAsset::makeEditorDataText() const
{
  ConfigFile editorFile;
  AssetMeta::write(m_editorData, EnginePaths::getProjectDirectory(), editorFile);
  writeEditorValues(editorFile);
  return editorFile.toText();
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
  const Path oldFile = assetFilePath(assetPath, m_metadata.name, getFileExtension());
  const Path newFile = assetFilePath(assetPath, newName, getFileExtension());
  // renameFile would replace the other asset's file. A change of case only finds the asset's
  // own file on Windows, which keeps no two names that differ by case.
  if (!StringUtils::equalsIgnoreCase(m_metadata.name, newName) &&
      FileSystem::exists(newFile)) {
    CH_LOG(AssetSystem, Error, "{0} already has an asset named {1}", assetPath, newName);
    return false;
  }
  if (!FileSystem::renameFile(oldFile, newFile)) {
    CH_LOG(AssetSystem, Error, "Failed to rename asset {0} to {1}", m_metadata.name, newName);
    return false;
  }

  StringUtils::copyToBuffer(m_metadata.name, newName);
  return true;
}

/*
 */
bool
IAsset::moveTo(const Path& folder) {
  if (folder.toString() == m_metadata.assetPath) {
    return true;
  }

  if (folder.toString().size() >= sizeof(m_metadata.assetPath)) {
    CH_LOG(AssetSystem, Error, "Folder {0} is longer than {1} characters", folder,
           sizeof(m_metadata.assetPath) - 1);
    return false;
  }

  const Path oldFile = getFilePath();
  const Path newFile = assetFilePath(folder, m_metadata.name, getFileExtension());
  // renameFile would replace the other asset's file.
  if (FileSystem::exists(newFile)) {
    CH_LOG(AssetSystem, Error, "{0} already has an asset named {1}", folder, m_metadata.name);
    return false;
  }
  if (!FileSystem::renameFile(oldFile, newFile)) {
    CH_LOG(AssetSystem, Error, "Failed to move asset {0} to {1}", m_metadata.name, folder);
    return false;
  }

  StringUtils::copyToBuffer(m_metadata.assetPath, folder.toString());
  return true;
}
#endif // USING(CH_EDITOR)

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

  const Path fullFilePath = assetFilePath(assetPath, m_metadata.name, getFileExtension());
  SPtr<DataStream> stream = FileSystem::openFile(fullFilePath, true);

  if (!stream || !stream->isReadable()) {
    CH_LOG(AssetSystem, Error, "Failed to open asset file {0}", assetPath.toString());
    m_state = AssetState::Failed;
    return false;
  }
  AssetFileInfo info;
  if (!isPlainFile() &&
      (!AssetFile::readInfo(*stream, info) || info.uuid != m_metadata.uuid ||
       info.assetType != m_metadata.assetType ||
       !AssetFile::readReferences(*stream, m_referencedAssets) ||
       !AssetFile::readEditorData(*stream, nullptr))) {
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
