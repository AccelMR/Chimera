/************************************************************************/
/**
 * @file chGameObjectAsset.h
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {

class GameObjectAsset : public IAsset {
 public:
  GameObjectAsset() = delete;
  GameObjectAsset(const AssetMetadata& metadata) : IAsset(metadata) {}

   ~GameObjectAsset() = default;

 protected:
  bool
  serialize(SPtr<DataStream>) override;

  bool
  deserialize(SPtr<DataStream>) override;

  void
  clearAssetData() override;

 private:
  SPtr<GameObject> m_gameObject;
  Vector<UUID> m_referencedAssets;
};
DECLARE_ASSET_TYPE(GameObjectAsset)

} // namespace chEngineSDK
