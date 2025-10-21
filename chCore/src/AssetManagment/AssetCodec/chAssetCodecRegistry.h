/************************************************************************/
/**
 * @file chAssetCodecRegistry.h
 * @author AccelMR
 * @date 2025/07/12
 * @details  Asset codec registry for Chimera Core.
 */
/***************************************************** *******************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetCodec.h"
#include "chTypeTraits.h"

#include "chUUID.h"

namespace chEngineSDK {
class IAssetCodec;

struct RegisterResponse {
  bool success;
  String message;
};

class AssetCodecRegistry
{
 public:

  template <typename AssetCodecType = IAssetCodec>
  void
  registerCodec() {
    SPtr<AssetCodecType> codec = chMakeShared<AssetCodecType>();

    for (const String& ext : codec->getSupportedExtensions()) {
      m_extensionToCodec[ext] = codec;
    }

    m_typeToCodec[AssetTypeTraits<AssetCodecType>::getTypeId()] = codec;
  }

  /*
  */
  template <typename AssetCodecType = IAssetCodec>
  SPtr<AssetCodecType>
  getCodec() const {
    auto it = m_typeToCodec.find(AssetTypeTraits<AssetCodecType>::getTypeId());
    if (it != m_typeToCodec.end()) {
      return std::static_pointer_cast<AssetCodecType>(it->second);
    }
    return nullptr;
  }

  SPtr<IAssetCodec>
  getCodecForExtension(const String& extension) const {
    auto it = m_extensionToCodec.find(extension);
    return (it != m_extensionToCodec.end()) ? it->second : nullptr;
  }

  SPtr<IAssetCodec>
  getCodecForAssetType(const UUID& assetType) const {
    auto it = m_typeToCodec.find(assetType);
    return (it != m_typeToCodec.end()) ? it->second : nullptr;
  }

  template <typename AssetType = IAsset>
  SPtr<IAssetCodec>
  getCodecForAssetType() const {
    return getCodecForAssetType(AssetTypeTraits<AssetType>::getTypeId());
  }

  const Vector<SPtr<IAssetCodec>>
  getAllCodecs() const {
    Vector<SPtr<IAssetCodec>> codecs;
    codecs.reserve(m_typeToCodec.size());

    auto codec_view = m_typeToCodec | std::views::transform([](const auto& pair) {
        return pair.second;
    });

    codecs.assign(codec_view.begin(), codec_view.end());
    return codecs;
  }

 private:
  UnorderedMap<String, SPtr<IAssetCodec>> m_extensionToCodec;
  UnorderedMap<UUID, SPtr<IAssetCodec>> m_typeToCodec;
};
} // namespace chEngineSDK
