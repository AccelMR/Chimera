/************************************************************************/
/**
 * @file chSkeletonAsset.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Asset that holds a Skeleton.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class Skeleton;

/**
 * Exists to save a Skeleton and share it between the models and animations made for it.
 */
class CH_CORE_EXPORT SkeletonAsset : public IAsset
{
 public:
  SkeletonAsset() = delete;
  SkeletonAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  SkeletonAsset(const AssetMetadata& metadata, const SPtr<Skeleton>& skeleton)
    : IAsset(metadata), m_skeleton(skeleton)
  {}
  ~SkeletonAsset() = default;

  /**
   * Null while the asset is not loaded.
   */
  NODISCARD FORCEINLINE const SPtr<Skeleton>&
  getSkeleton() const noexcept { return m_skeleton; }

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

 private:
  SPtr<Skeleton> m_skeleton;
};
DECLARE_ASSET_TYPE(SkeletonAsset);

} // namespace chEngineSDK
