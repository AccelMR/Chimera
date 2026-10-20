/************************************************************************/
/**
 * @file chAnimationClipAsset.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Asset that holds an AnimationClip.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class AnimationClip;

/**
 * Exists to save an AnimationClip, so every object that plays it shares one copy.
 */
class CH_CORE_EXPORT AnimationClipAsset : public IAsset
{
 public:
  AnimationClipAsset() = delete;
  AnimationClipAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  AnimationClipAsset(const AssetMetadata& metadata, const SPtr<AnimationClip>& clip)
    : IAsset(metadata), m_clip(clip)
  {}
  ~AnimationClipAsset() = default;

  /**
   * Null while the asset is not loaded.
   */
  NODISCARD FORCEINLINE const SPtr<AnimationClip>&
  getClip() const noexcept { return m_clip; }

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

 private:
  SPtr<AnimationClip> m_clip;
};
DECLARE_ASSET_TYPE(AnimationClipAsset);

} // namespace chEngineSDK
