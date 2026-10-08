
#include "chAssetCodecManager.h"

#include "chLogger.h"
CH_LOG_DEFINE_CATEGORY_SHARED(AssetCodecSystem, All);

#if USING(CH_CODECS)

#include "chAssetCodec.h"
#include "chAssetCodecRegistry.h"
#include "chFileSystem.h"
#include "chPath.h"

namespace chEngineSDK {

// The editor starts this module and the codec plugins use it, so its storage must live
// here: without this, Linux gives a plugin opened with dlopen its own copy, which was never
// started (an executable does not export its symbols).
template class Module<AssetCodecManager>;

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
AssetCodecManager::importAsset(const Path& importPath,
                               const String& assetName,
                               const Path& assetFolder,
                               const ImportSettings& settings)
{
  CH_ASSERT(m_codecRegistry && "AssetCodecRegistry must be initialized before importing.");

  if (!FileSystem::isFile(importPath)) {
    CH_LOG_ERROR(AssetCodecSystem, "Import file {0} does not exist", importPath);
    return nullptr;
  }

  const String extension = importPath.getExtension();
  SPtr<IAssetCodec> codec = m_codecRegistry->getCodecForExtension(extension);
  if (!codec) {
    CH_LOG_ERROR(AssetCodecSystem, "No codec found for file extension {0}", extension);
    return nullptr;
  }

  CH_LOG_DEBUG(AssetCodecSystem, "Importing {0} as {1} with codec {2}", importPath, assetName,
               codec->getCodecType());
  return codec->importAsset(importPath, assetName, assetFolder, settings);
}

/*
*/
SPtr<IAsset>
AssetCodecManager::importAssetFromMemory(Span<const uint8> data,
                                         StringView extension,
                                         const String& assetName,
                                         const Path& assetFolder,
                                         StringView importedPath,
                                         const ImportSettings& settings)
{
  CH_ASSERT(m_codecRegistry && "AssetCodecRegistry must be initialized before importing.");

  SPtr<IAssetCodec> codec = m_codecRegistry->getCodecForExtension(extension);
  if (!codec) {
    CH_LOG_ERROR(AssetCodecSystem, "No codec found for file extension {0}", extension);
    return nullptr;
  }
  return codec->importAssetFromMemory(data, assetName, assetFolder, importedPath, settings);
}

/*
*/
Vector<String>
AssetCodecManager::getSupportedAllExtensions() const {
  CH_ASSERT(m_codecRegistry && "AssetCodecRegistry must be initialized before accessing codecs.");
  Vector<String> allExtensions;

  for (const auto& codec : m_codecRegistry->getAllCodecs()) {
    const Vector<String>& extensions = codec->getSupportedExtensions();
    allExtensions.insert(allExtensions.end(), extensions.begin(), extensions.end());
  }

  return allExtensions;
}
} // namespace chEngineSDK
#endif //  USING(CH_CODECS)
