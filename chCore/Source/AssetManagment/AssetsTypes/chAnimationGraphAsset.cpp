/************************************************************************/
/**
 * @file chAnimationGraphAsset.cpp
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Asset that holds an AnimationGraph.
 */
/************************************************************************/
#include "chAnimationGraphAsset.h"

#include "chAnimationClipAsset.h"
#include "chAnimationGraph.h"
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chLogger.h"

namespace chEngineSDK {

/*
 */
bool
AnimationGraphAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream || !m_graph) {
    CH_LOG_ERROR(AssetSystem, "AnimationGraphAsset {0} has no graph to save", getName());
    return false;
  }
  m_graph->serialize(*stream);
  return true;
}

/*
 */
bool
AnimationGraphAsset::deserialize(SPtr<DataStream> stream)
{
  String error;
  m_graph = stream ? AnimationGraph::deserialize(*stream, &error) : nullptr;
  if (!m_graph) {
    CH_LOG_ERROR(AssetSystem, "AnimationGraphAsset {0} could not be read. {1}", getName(),
                 error);
    return false;
  }

  // IAsset::load already loaded the references, so the clips are ready when they exist.
  const Vector<AnimationGraphClip>& clips = m_graph->getClips();
  for (uint32 i = 0; i < static_cast<uint32>(clips.size()); ++i) {
    const UUID& clipId = clips[i].clipId;
    if (clipId.isNull()) {
      continue;
    }
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(clipId);
    SPtr<AnimationClipAsset> clip = asset ? asset->as<AnimationClipAsset>() : nullptr;
    if (!clip) {
      CH_LOG_WARNING(AssetSystem, "AnimationGraphAsset {0} lost its clip {1}", getName(),
                     clipId);
    }
    m_graph->setClipAsset(i, clip);
  }
  return true;
}

/*
 */
void
AnimationGraphAsset::clearAssetData()
{
  m_graph.reset();
}

/*
 */
void
AnimationGraphAsset::collectReferences(Vector<UUID>& outReferences) const
{
  if (m_graph) {
    m_graph->collectReferences(outReferences);
  }
}

} // namespace chEngineSDK
