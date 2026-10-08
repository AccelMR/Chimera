#pragma once

#include "chLogDeclaration.h"
#include "chPrerequisitesCore.h"
CH_LOG_DECLARE_EXTERN(CH_CORE_EXPORT, AssetCodecSystem);

#if USING(CH_CODECS)

#include "chAssetCodec.h"
#include "chAssetCodecRegistry.h"
#include "chEventSystem.h"
#include "chModule.h"

namespace chEngineSDK {

class CH_CORE_EXPORT AssetCodecManager : public Module<AssetCodecManager>
{
 public:
  void
  initialize();

  /**
   * @brief Imports an external file with the codec registered for its extension.
   *        The codec creates and saves the asset.
   * @param importPath The external file to import.
   * @param assetName The name of the new asset.
   * @param assetFolder Virtual folder under /Game that receives the asset.
   * @return The new asset, or nullptr if the file does not exist, no codec handles
   *         its extension, or the codec fails.
   */
  SPtr<IAsset>
  importAsset(const Path& importPath,
              const String& assetName,
              const Path& assetFolder,
              const ImportSettings& settings = {});

  /**
   * Same as importAsset for a file already in memory; the codec is picked by extension
   * (with or without the dot). See IAssetCodec::importAssetFromMemory.
   */
  SPtr<IAsset>
  importAssetFromMemory(Span<const uint8> data,
                        StringView extension,
                        const String& assetName,
                        const Path& assetFolder,
                        StringView importedPath,
                        const ImportSettings& settings = {});

  template<typename AssetCodecType>
  NODISCARD FORCEINLINE SPtr<AssetCodecType>
  getCodec() const
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodec<AssetCodecType>();
  }

  NODISCARD FORCEINLINE const Vector<SPtr<IAssetCodec>>&
  getAllCodecs() const
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getAllCodecs();
  }

  NODISCARD Vector<String>
  getSupportedAllExtensions() const;

  /**
   * Accepts the extension with or without the dot, in any case (".PNG").
   */
  NODISCARD FORCEINLINE SPtr<IAssetCodec>
  getCodecForExtension(StringView extension) const
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForExtension(extension);
  }

  NODISCARD FORCEINLINE SPtr<IAssetCodec>
  getCodecForAssetType(const UUID& assetType) const
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForAssetType(assetType);
  }

  template<typename AssetType>
  NODISCARD FORCEINLINE SPtr<IAssetCodec>
  getCodecForAssetType() const
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForAssetType<AssetType>();
  }

  template<typename AssetCodecType>
  FORCEINLINE void
  registerCodec()
  {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before registering codecs.");
    m_codecRegistry->registerCodec<AssetCodecType>();
  }

 private:
  UniquePtr<AssetCodecRegistry> m_codecRegistry;
}; // class AssetCodecManager
} // namespace chEngineSDK
#endif  // USING(CH_EDITOR)
