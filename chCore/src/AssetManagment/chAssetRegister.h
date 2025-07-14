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

#include "chUUID.h"

namespace chEngineSDK {
class IAsset;
struct AssetMetadata;

using AssetCreatorFunc = Function<SPtr<IAsset>(const AssetMetadata& metadata)>;
class AssetRegister {
 public:
  template<typename TAsset>
  void
  registerAssetCreator(const UUID& uuid) {
    static_assert(std::is_base_of<IAsset, TAsset>::value, "TAsset must be derived from IAsset");
    m_assetCreators[uuid] = [](const AssetMetadata& metadata) -> SPtr<IAsset> {
      return chMakeShared<TAsset>(metadata);
    };
  }

  AssetCreatorFunc
  getAssetCreator(const UUID& uuid) const {
    auto it = m_assetCreators.find(uuid);
    if (it != m_assetCreators.end()) {
      return it->second;
    }
    return nullptr;
  }

 private:
  UnorderedMap<UUID, AssetCreatorFunc> m_assetCreators; ///< Map of asset creators by UUID
}; // class AssetRegistry
} // namespace chEngineSDK
