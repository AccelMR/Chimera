/************************************************************************/
/**
 * @file chAssetCodecRegistry.cpp
 * @author AccelMR
 * @date 2025/07/12
 * @brief Keeps the registered asset codecs and finds them by extension or type.
 */
/************************************************************************/
#include "chPrerequisitesCore.h"

#if USING(CH_CODECS)

#include "chAssetCodecRegistry.h"

#include "chAssetCodecManager.h"
#include "chLogger.h"

namespace chEngineSDK {

/*
 */
SPtr<IAssetCodec>
AssetCodecRegistry::getCodecForExtension(StringView extension) const
{
  const auto it = m_codecsByExtension.find(IAssetCodec::normalizeExtension(extension));
  return m_codecsByExtension.end() != it ? it->second : nullptr;
}

/*
 */
SPtr<IAssetCodec>
AssetCodecRegistry::getCodecForAssetType(const UUID& assetType) const
{
  const auto it = m_codecsByAssetType.find(assetType);
  return m_codecsByAssetType.end() != it ? it->second : nullptr;
}

/*
 */
void
AssetCodecRegistry::addCodec(const SPtr<IAssetCodec>& codec, const UUID& codecType)
{
  m_codecs.push_back(codec);
  m_codecsByCodecType[codecType] = codec;

  // When two codecs claim the same extension or asset type, the one registered last
  // wins, so the warning tells which plugin replaced which.
  for (const UUID& assetType : codec->getSupportedAssetTypes()) {
    const auto [it, added] = m_codecsByAssetType.try_emplace(assetType, codec);
    if (!added) {
      CH_LOG_WARNING(AssetCodecSystem, "Codec {0} replaces codec {1} for asset type {2}",
                     codec->getCodecType(), it->second->getCodecType(), assetType);
      it->second = codec;
    }
  }

  for (const String& extension : codec->getSupportedExtensions()) {
    const auto [it, added] =
        m_codecsByExtension.try_emplace(IAssetCodec::normalizeExtension(extension), codec);
    if (!added) {
      CH_LOG_WARNING(AssetCodecSystem, "Codec {0} replaces codec {1} for extension {2}",
                     codec->getCodecType(), it->second->getCodecType(), extension);
      it->second = codec;
    }
  }
}

} // namespace chEngineSDK

#endif // USING(CH_CODECS)
