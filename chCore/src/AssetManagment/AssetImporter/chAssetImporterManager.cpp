
#include "chAssetImporterManager.h"

#include "chLogger.h"
CH_LOG_DEFINE_CATEGORY_SHARED(AssetImporterSystem, All);

#if USING(CH_IMPORTERS)

#include "chAssetImporter.h"
#include "chAssetImporterRegistry.h"
#include "chAssetManager.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chPath.h"
#include "chLogger.h"

namespace chEngineSDK {
/*
*/
void
AssetImporterManager::initialize() {
  CH_LOG_DEBUG(AssetImporterSystem, "Initializing AssetImporterRegistry");

  m_importerRegistry = chMakeShared<AssetImporterRegistry>();

  // In case someone wants to register importers before the registry is initialized
  m_onRegisterImporter(m_importerRegistry);
}

/*
*/
SPtr<IAsset>
AssetImporterManager::importAsset(const Path& absoluteImportFilePath,
                                  const Path& assetRelativePath) {
  CH_LOG_DEBUG(AssetImporterSystem, "Importing asset from {0} to {1}", absoluteImportFilePath, assetRelativePath);

  if (!FileSystem::exists(absoluteImportFilePath)) {
    CH_LOG_ERROR(AssetImporterSystem, "Import file path {0} does not exist", absoluteImportFilePath);
    return nullptr;
  }

  const Path assetDir = EnginePaths::getGameAssetDirectory();
  if (!FileSystem::arePathsRelative(assetDir, assetRelativePath)) {
    CH_LOG_ERROR(AssetImporterSystem, "Asset path {0} is not relative to the asset directory {1}", assetRelativePath, assetDir);
    return nullptr;
  }

  String baseName = absoluteImportFilePath.getFileName(false);
  Path absoluteAssetFilePath;
  int32 sameName = 0;
  do {
    String fileName = baseName + (sameName ? "_" + chString::fromInt32(sameName) : "") + ".chAsset";
    absoluteAssetFilePath = absoluteAssetFilePath / fileName;
    ++sameName;
  } while (FileSystem::exists(absoluteAssetFilePath));

  CH_LOG_DEBUG(AssetImporterSystem, "Saving imported asset to {0}", absoluteAssetFilePath);

  // Create a chimera resource file
  SPtr<DataStream> fileStream = FileSystem::createAndOpenFile(absoluteAssetFilePath);
  if (!fileStream) {
    CH_LOG_ERROR(AssetImporterSystem, "Failed to create or open file {0}", absoluteAssetFilePath);
    return nullptr;
  }

  // Determine asset type based on file extension or content
  String extension = absoluteImportFilePath.getExtension();
  SPtr<IAssetImporter> importer = m_importerRegistry->getImporterForExtension(extension);
  if (!importer) {
    CH_LOG_ERROR(AssetImporterSystem, "No importer found for file extension {0}", extension);
    return nullptr;
  }

  CH_LOG_DEBUG(AssetImporterSystem, "Using importer {0} for file {1}",
                                    importer->getImporterType().toString(),
                                    absoluteImportFilePath);

  return importer->importAsset(absoluteImportFilePath, baseName);
}

/*
*/
Vector<String>
AssetImporterManager::getSupportedAllExtensions() const {
  CH_ASSERT(m_importerRegistry && "AssetImporterRegistry must be initialized before accessing importers.");
  Vector<String> allExtensions;

  for (const auto& importer : m_importerRegistry->getAllImporters()) {
    Vector<String> extensions = importer->getSupportedExtensions();
    allExtensions.insert(allExtensions.end(), extensions.begin(), extensions.end());
  }

  return allExtensions;
}
} // namespace chEngineSDK
#endif //  USING(CH_IMPORTERS)
