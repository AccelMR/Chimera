
#include "chAssetCodecManager.h"

#include "chLogger.h"
CH_LOG_DEFINE_CATEGORY_SHARED(AssetCodecSystem, All);

#if USING(CH_CODECS)

#include "chAssetCodec.h"
#include "chAssetCodecRegistry.h"
#include "chAssetManager.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chPath.h"
#include "chLogger.h"

namespace chEngineSDK {
/*
*/
void
AssetCodecManager::initialize() {
  CH_LOG_DEBUG(AssetCodecSystem, "Initializing AssetCodecRegistry");

  m_codecRegistry = chMakeUnique<AssetCodecRegistry>();
}

/*
*/
SPtr<IAsset>
AssetCodecManager::importAsset(const Path& absoluteImportFilePath,
                                  const Path& assetRelativePath) {
  CH_LOG_DEBUG(AssetCodecSystem, "Importing asset from {0} to {1}", absoluteImportFilePath, assetRelativePath);

  if (!FileSystem::exists(absoluteImportFilePath)) {
    CH_LOG_ERROR(AssetCodecSystem, "Import file path {0} does not exist", absoluteImportFilePath);
    return nullptr;
  }

  const Path assetDir = EnginePaths::getGameAssetDirectory();
  if (!FileSystem::arePathsRelative(assetDir, assetRelativePath)) {
    CH_LOG_ERROR(AssetCodecSystem, "Asset path {0} is not relative to the asset directory {1}", assetRelativePath, assetDir);
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

  CH_LOG_DEBUG(AssetCodecSystem, "Saving imported asset to {0}", absoluteAssetFilePath);

  // Create a chimera resource file
  SPtr<DataStream> fileStream = FileSystem::createAndOpenFile(absoluteAssetFilePath);
  if (!fileStream) {
    CH_LOG_ERROR(AssetCodecSystem, "Failed to create or open file {0}", absoluteAssetFilePath);
    return nullptr;
  }

  // Determine asset type based on file extension or content
  String extension = absoluteImportFilePath.getExtension();
  SPtr<IAssetCodec> codec = m_codecRegistry->getCodecForExtension(extension);
  if (!codec) {
    CH_LOG_ERROR(AssetCodecSystem, "No codec found for file extension {0}", extension);
    return nullptr;
  }

  CH_LOG_DEBUG(AssetCodecSystem, "Using codec {0} for file {1}",
                                    codec->getCodecType().toString(),
                                    absoluteImportFilePath);
  return codec->importAsset(absoluteImportFilePath, baseName);
}

/*
*/
Vector<String>
AssetCodecManager::getSupportedAllExtensions() const {
  CH_ASSERT(m_codecRegistry && "AssetCodecRegistry must be initialized before accessing codecs.");
  Vector<String> allExtensions;

  for (const auto& codec : m_codecRegistry->getAllCodecs()) {
    Vector<String> extensions = codec->getSupportedExtensions();
    allExtensions.insert(allExtensions.end(), extensions.begin(), extensions.end());
  }

  return allExtensions;
}
} // namespace chEngineSDK
#endif //  USING(CH_CODECS)
