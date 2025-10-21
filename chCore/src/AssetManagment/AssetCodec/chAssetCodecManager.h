#pragma once

#include "chLogDeclaration.h"
#include "chPrerequisitesCore.h"
CH_LOG_DECLARE_EXTERN(AssetCodecSystem);

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
   * @brief Imports an asset from the specified import path to the asset path.
   * @param importPath The path to the asset to be imported.
   * @param assetPath The destination path for the imported asset.
   * @return True if the import was successful, false otherwise.
   */
  SPtr<IAsset>
  importAsset(const Path& importPath, const Path& assetPath);

  template <typename AssetType = IAsset>
  FORCEINLINE SPtr<AssetType>
  importAsset(const Path& importPath, const String& assetName) {
    return std::static_pointer_cast<AssetType>(importAsset(importPath, assetName));
  }

  template <typename AssetCodecType = IAssetCodec>
  FORCEINLINE SPtr<AssetCodecType>
  getCodec() const {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodec<AssetCodecType>();
  }

  FORCEINLINE Vector<SPtr<IAssetCodec>>
  getAllCodecs() const {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getAllCodecs();
  }

  Vector<String>
  getSupportedAllExtensions() const;

  SPtr<IAssetCodec>
  FORCEINLINE getCodecForExtension(const String& extension) const {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForExtension(extension);
  }

  SPtr<IAssetCodec>
  FORCEINLINE getCodecForAssetType(const UUID& assetType) const {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForAssetType(assetType);
  }

  template <typename AssetType = IAsset>
  SPtr<IAssetCodec>
  FORCEINLINE getCodecForAssetType() const {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before accessing codecs.");
    return m_codecRegistry->getCodecForAssetType<AssetType>();
  }

  template <typename AssetCodecType = IAssetCodec>
  FORCEINLINE void
  registerCodec() {
    CH_ASSERT(m_codecRegistry &&
              "AssetCodecRegistry must be initialized before registering codecs.");
    m_codecRegistry->registerCodec<AssetCodecType>();
  }

 private:
  UniquePtr<AssetCodecRegistry> m_codecRegistry;
}; // class AssetCodecManager
} // namespace chEngineSDK
#endif  // USING(CH_EDITOR)
