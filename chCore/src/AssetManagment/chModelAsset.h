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
  ModelAsset() = delete;
  ModelAsset(const AssetMetadata& metadata)
    : IAsset(metadata) {}

  ~ModelAsset() = default;

  bool
  load() override { return true; }

  bool
  unload() override { return true; }

  bool
  _internalSerialize(SPtr<DataStream>) override { return true; }

  bool
  _internalDeserialize(SPtr<DataStream>) override { return true; }

 private:
  friend class MeshManager;
  void
  clearAssetData() override { }

  FORCEINLINE void
  setModel(const SPtr<Model>& model) {
    m_model = model;
  }

  SPtr<Model> m_model; ///< The model data associated with this asset
}; // class ModelAsset
DECLARE_ASSET_TYPE(ModelAsset);

} // namespace chEngineSDK
