/************************************************************************/
/**
 * @file chGameObjectAsset.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chGameObjectAsset.h"

#include "chGameObject.h"

namespace chEngineSDK {

bool
GameObjectAsset::serialize(SPtr<DataStream> stream) {
  CH_PARAMETER_UNUSED(stream);
  return true;
}

bool
GameObjectAsset::deserialize(SPtr<DataStream> stream) {
  CH_PARAMETER_UNUSED(stream);
  m_gameObject = chMakeShared<GameObject>("Test");
  return true;
}

void
GameObjectAsset::clearAssetData() {
  m_referencedAssets.clear();
}

} // namespace chEngineSDK
