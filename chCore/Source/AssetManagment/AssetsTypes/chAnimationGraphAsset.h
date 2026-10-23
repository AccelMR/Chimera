/************************************************************************/
/**
 * @file chAnimationGraphAsset.h
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Asset that holds an AnimationGraph.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class AnimationGraph;

/**
 * Exists to save an AnimationGraph with references to its clips, so every object that uses
 * it shares one copy and its clips are loaded before it.
 */
class CH_CORE_EXPORT AnimationGraphAsset : public IAsset
{
 public:
  AnimationGraphAsset() = delete;
  AnimationGraphAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  AnimationGraphAsset(const AssetMetadata& metadata, const SPtr<AnimationGraph>& graph)
   : IAsset(metadata), m_graph(graph)
  {
  }
  ~AnimationGraphAsset() = default;

  /**
   * Null while the asset is not loaded.
   */
  NODISCARD FORCEINLINE const SPtr<AnimationGraph>&
  getGraph() const noexcept
  {
    return m_graph;
  }

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
  SPtr<AnimationGraph> m_graph;
};
DECLARE_ASSET_TYPE(AnimationGraphAsset);

} // namespace chEngineSDK
