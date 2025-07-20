/************************************************************************/
/**
 * @file chAssetRegistry.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset registry for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetTypeTraits.h"
#include "chUUID.h"

namespace chEngineSDK {
class IAsset;
struct AssetMetadata;

using AssetCreatorFunc = Function<SPtr<IAsset>(const AssetMetadata& metadata)>;
class AssetRegister {
 public:
  template<typename TAsset>
  void
  registerAssetCreator() {
    static_assert(std::is_base_of<IAsset, TAsset>::value, "TAsset must be derived from IAsset");
    const UUID uuid = AssetTypeTraits<TAsset>::getTypeId();
    m_assetCreators[uuid] = [](const AssetMetadata& metadata) -> SPtr<IAsset> {
      return chMakeShared<TAsset>(metadata);
    };
    m_assetTypeNames[uuid] = String(AssetTypeTraits<TAsset>::getTypeName());
  }

  AssetCreatorFunc
  getAssetCreator(const UUID& uuid) const {
    auto it = m_assetCreators.find(uuid);
    if (it != m_assetCreators.end()) {
      return it->second;
    }
    return nullptr;
  }

  const String&
  getAssetTypeName(const UUID& uuid) const {
    auto it = m_assetCreators.find(uuid);
    if (it != m_assetCreators.end()) {
      return m_assetTypeNames.at(uuid);
    }
    static const String unknownType = "Unknown";
    return unknownType;
  }

 private:
  UnorderedMap<UUID, AssetCreatorFunc> m_assetCreators; ///< Map of asset creators by UUID
  UnorderedMap<UUID, String> m_assetTypeNames; ///< Map of asset type names by UUID
}; // class AssetRegistry
} // namespace chEngineSDK
