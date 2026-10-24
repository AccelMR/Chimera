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

#if USING(CH_EDITOR)
#include "chVector2.h"
#endif

namespace chEngineSDK {
class AnimationGraph;

#if USING(CH_EDITOR)
/**
 * Where the graph editor draws the nodes of one layer, in its canvas units.
 */
struct AnimationGraphLayerLayout
{
  // One per state, in the order of the layer's states.
  Vector<Vector2> statePositions{};
  Vector2 anyStatePosition = Vector2::ZERO;
};

struct AnimationGraphLayout
{
  // One per layer, in the order of the graph's layers.
  Vector<AnimationGraphLayerLayout> layers{};
  // The model the graph editor previews the graph on; null picks one that fits.
  UUID previewModel = UUID::null();
};
#endif

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

#if USING(CH_EDITOR)
  /**
   * Replaces the graph; save writes it. Objects already using the old graph keep it until
   * they are given the asset again.
   */
  FORCEINLINE void
  setGraph(const SPtr<AnimationGraph>& graph)
  {
    m_graph = graph;
  }

  /**
   * Kept in the editor data, so it survives unloading and never reaches a game.
   */
  NODISCARD FORCEINLINE const AnimationGraphLayout&
  getLayout() const noexcept
  {
    return m_layout;
  }

  FORCEINLINE void
  setLayout(AnimationGraphLayout layout)
  {
    m_layout = std::move(layout);
  }
#endif

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

#if USING(CH_EDITOR)
  void
  writeEditorValues(ConfigFile& file) const override;

  void
  readEditorValues(const ConfigFile& file) override;
#endif

 private:
  SPtr<AnimationGraph> m_graph;
#if USING(CH_EDITOR)
  AnimationGraphLayout m_layout;
#endif
};
DECLARE_ASSET_TYPE(AnimationGraphAsset);

} // namespace chEngineSDK
