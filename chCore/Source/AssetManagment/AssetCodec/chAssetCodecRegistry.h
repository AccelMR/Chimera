/************************************************************************/
/**
 * @file chAssetCodecRegistry.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief Keeps the registered asset codecs and finds them by extension or type.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetCodec.h"
#include "chTypeTraits.h"
#include "chUUID.h"

namespace chEngineSDK {

/**
 * Indexes the codecs once, when they register, so finding the codec for an extension, a
 * codec type or an asset type is a single map lookup.
 */
class CH_CORE_EXPORT AssetCodecRegistry
{
 public:
  template<typename AssetCodecType>
  void
  registerCodec()
  {
    addCodec(chMakeShared<AssetCodecType>(), AssetTypeTraits<AssetCodecType>::getTypeId());
  }

  template<typename AssetCodecType>
  NODISCARD SPtr<AssetCodecType>
  getCodec() const
  {
    const auto it = m_codecsByCodecType.find(AssetTypeTraits<AssetCodecType>::getTypeId());
    if (m_codecsByCodecType.end() == it) {
      return nullptr;
    }
    return std::static_pointer_cast<AssetCodecType>(it->second);
  }

  /**
   * Accepts the extension with or without the dot, in any case (".PNG").
   */
  NODISCARD SPtr<IAssetCodec>
  getCodecForExtension(StringView extension) const;

  NODISCARD SPtr<IAssetCodec>
  getCodecForAssetType(const UUID& assetType) const;

  template<typename AssetType>
  NODISCARD SPtr<IAssetCodec>
  getCodecForAssetType() const
  {
    return getCodecForAssetType(AssetTypeTraits<AssetType>::getTypeId());
  }

  NODISCARD const Vector<SPtr<IAssetCodec>>&
  getAllCodecs() const noexcept
  {
    return m_codecs;
  }

 private:
  void
  addCodec(const SPtr<IAssetCodec>& codec, const UUID& codecType);

  Vector<SPtr<IAssetCodec>> m_codecs;
  UnorderedMap<UUID, SPtr<IAssetCodec>> m_codecsByCodecType;
  UnorderedMap<UUID, SPtr<IAssetCodec>> m_codecsByAssetType;
  UnorderedMap<String, SPtr<IAssetCodec>> m_codecsByExtension;
};

} // namespace chEngineSDK
