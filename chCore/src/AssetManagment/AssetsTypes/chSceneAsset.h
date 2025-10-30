/************************************************************************/
/**
 * @file chSceneAsset.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class CH_CORE_EXPORT SceneAsset : public IAsset {
 public:
  SceneAsset() = default;

  SceneAsset(const AssetMetadata& metadata)
   : IAsset(metadata) {}

  WeakPtr<Scene>
  getScene() const {
    return m_scene;
  }


 protected:
  void
  clearAssetData() override {
    // Clear scene-specific data here
  }

  bool
  serialize(SPtr<DataStream> stream) override {
    // Implement scene serialization logic here
    return true;
  }

  bool
  deserialize(SPtr<DataStream> stream) override {
    // Implement scene deserialization logic here
    return true;
  }

 private:
  // Scene-specific data members
  SPtr<Scene> m_scene;
};
DECLARE_ASSET_TYPE(SceneAsset);

} // namespace chEngineSDK
