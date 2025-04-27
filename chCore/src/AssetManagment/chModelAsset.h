/************************************************************************/
/**
 * @file chModelAsset.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief 
 *  This file contains the ModelAsset class for managing model assets in the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {
class CH_CORE_EXPORT ModelAsset : public IAsset
{
 public:
  ModelAsset() = default;

  ~ModelAsset() override = default;

  bool
  load() override;

  bool
  unload() override;

  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  NODISCAR FORCEINLINE static AssetType
  getStaticType() { return AssetType::Model; }

 private:
  void
  clearAssetData() override;
}; // class ModelAsset
} // namespace chEngineSDK