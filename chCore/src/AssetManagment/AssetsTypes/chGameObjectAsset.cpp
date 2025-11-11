/************************************************************************/
/**
 * @file chGameObjectAsset.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chGameObjectAsset.h"

namespace chEngineSDK {

bool
GameObjectAsset::serialize(SPtr<DataStream> stream) {
  return true;
}

bool
GameObjectAsset::deserialize(SPtr<DataStream> stream) {
  return true;
}

void
GameObjectAsset::clearAssetData() {
  m_referencedAssets.clear();
}

} // namespace chEngineSDK
