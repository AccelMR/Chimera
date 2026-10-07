/************************************************************************/
/**
 * @file chMaterialAsset.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  A Material saved as an asset, with references to its textures.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chMaterial.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists to save a Material and share it between models. The material is kept by value and
 * never replaced (loading and unloading change its contents), so render items can point at
 * it for as long as the asset lives.
 */
class CH_CORE_EXPORT MaterialAsset : public IAsset
{
 public:
  MaterialAsset() = delete;
  MaterialAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  ~MaterialAsset() = default;

  NODISCARD FORCEINLINE Material&
  getMaterial() noexcept { return m_material; }

  NODISCARD FORCEINLINE const Material&
  getMaterial() const noexcept { return m_material; }

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
  Material m_material;
};
DECLARE_ASSET_TYPE(MaterialAsset);

} // namespace chEngineSDK
