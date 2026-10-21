/************************************************************************/
/**
 * @file chModelAsset.h
 * @author AccelMR
 * @date 2025/07/14
 * @brief
 *  Asset that holds a Model.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {
class Model;

/**
 * Exists to save and load a Model as an asset; the bytes of the model come from
 * ModelSerializer, and the materials of its slots are loaded as references.
 */
class CH_CORE_EXPORT ModelAsset : public IAsset
{
 public:
  ModelAsset() = delete;
  ModelAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  ModelAsset(const AssetMetadata& metadata, const SPtr<Model>& model)
    : IAsset(metadata), m_model(model)
  {
    CH_ASSERT(m_model && "Model cannot be null");
  }

  ~ModelAsset() = default;

  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const { return m_model; }

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

 private:
  SPtr<Model> m_model;
};
DECLARE_ASSET_TYPE(ModelAsset);

} // namespace chEngineSDK
