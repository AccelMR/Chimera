/************************************************************************/
/**
 * @file chAssetManager.cpp
 * @author AccelMR
 * @date 2025/07/10
 * @brief
 * /
/************************************************************************/
#include "chAssetManagerImporter.h"

#include "chAssetManager.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chPath.h"
#include "chLogger.h"
#include "chSupportedImport.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(AssetImporterSystem, All);

/*
*/
bool
AssetManagerImporter::importAsset(const Path& absoluteImportFilePath,
                                  const Path& assetRelativePath) {
  CH_LOG_DEBUG(AssetImporterSystem, "Importing asset from {0} to {1}", absoluteImportFilePath, assetRelativePath);

  if (!FileSystem::exists(absoluteImportFilePath)) {
    CH_LOG_ERROR(AssetImporterSystem, "Import file path {0} does not exist", absoluteImportFilePath);
    return false;
  }

  const Path assetDir = EnginePaths::getAssetDirectory();
  if (!FileSystem::arePathsRelative(assetDir, assetRelativePath)) {
    CH_LOG_ERROR(AssetImporterSystem, "Asset path {0} is not relative to the asset directory {1}", assetRelativePath, assetDir);
    return false;
  }

  String baseName = absoluteImportFilePath.getFileName(false);
  Path absoluteAssetFilePath;
  int32 sameName = 0;
  do {
    String fileName = baseName + (sameName ? "_" + String::fromInt(sameName) : "") + ".chAsset";
    absoluteAssetFilePath = assetDir.join(fileName);
    ++sameName;
  } while (FileSystem::exists(absoluteAssetFilePath));

  CH_LOG_DEBUG(AssetImporterSystem, "Saving imported asset to {0}", absoluteAssetFilePath);

  // Create a chimera resource file
  SPtr<FileStream> fileStream = FileSystem::createAndOpenFile(absoluteAssetFilePath);
  if (!fileStream) {
    CH_LOG_ERROR(AssetImporterSystem, "Failed to create or open file {0}", absoluteAssetFilePath);
    return false;
  }

  AssetType assetType = AssetType::kUnknown;
  // Determine asset type based on file extension or content
  String extension = absoluteImportFilePath.getExtension();
  for (const auto& supportedImport : SupportedImport::getSupportedImports()) {
    if (supportedImport.extensions.contains(extension)) {
      assetType = supportedImport.type;
      break;
    }
  }

  switch (assetType) {
    case AssetType::kMesh:
    {
      CH_LOG_DEBUG(AssetImporterSystem, "Importing mesh asset");
      SPtr<Model> model = MeshManager::instance().loadModel(modelPath);
    }
    break;
    case AssetType::kTexture:
      CH_LOG_DEBUG(AssetImporterSystem, "Importing texture asset");
      // Call texture import logic here
      break;
    case AssetType::kAudio:
      CH_LOG_DEBUG(AssetImporterSystem, "Importing audio asset");
      // Call audio import logic here
      break;
    default:
      CH_LOG_ERROR(AssetImporterSystem, "Unsupported asset type for import: {0}", extension);
      return false;
  }

  AssetManager& assetManager = AssetManager::getInstance();
  SPtr<IAsset> asset = assetManager.createAsset(assetType, baseName, absoluteImportFilePath);

  if (!asset) {
    CH_LOG_ERROR(AssetImporterSystem, "Failed to create asset of type {0} for file {1}", assetType, absoluteImportFilePath);
    return false;
  }

}

} // namespace chEngineSDK
