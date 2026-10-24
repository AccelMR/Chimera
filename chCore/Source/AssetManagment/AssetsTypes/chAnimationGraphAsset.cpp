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

#if USING(CH_EDITOR)
#include <charconv>

#include "chConfigFile.h"
#include "chStringUtils.h"
#endif // USING(CH_EDITOR)

namespace chEngineSDK {

#if USING(CH_EDITOR)
namespace {
constexpr StringView kLayoutSection = "GraphLayout";

/*
 * Every position of the list as "x y", separated by spaces.
 */
String
positionsText(Span<const Vector2> positions)
{
  String text;
  for (const Vector2& position : positions) {
    if (!text.empty()) {
      text += ' ';
    }
    text += StringUtils::format("{0} {1}", position.x, position.y);
  }
  return text;
}

/*
 * Reads numbers in pairs until the text ends or something is not a number.
 */
void
readPositions(StringView text, Vector<Vector2>& outPositions)
{
  const ANSICHAR* current = text.data();
  const ANSICHAR* const end = current + text.size();
  float values[2];
  uint32 count = 0;
  while (true) {
    while (current < end && *current == ' ') {
      ++current;
    }
    const std::from_chars_result result = std::from_chars(current, end, values[count]);
    if (result.ec != std::errc()) {
      return;
    }
    current = result.ptr;
    if (++count == 2) {
      outPositions.push_back(Vector2(values[0], values[1]));
      count = 0;
    }
  }
}
} // namespace
#endif // USING(CH_EDITOR)

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
  // The editor also opens a graph still being built; games only load finished ones.
  m_graph = stream ? AnimationGraph::deserialize(*stream, &error, !USING(CH_EDITOR)) : nullptr;
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

#if USING(CH_EDITOR)
/*
 * One line per layer, LayerN = the positions of its states, then the Any State node.
 */
void
AnimationGraphAsset::writeEditorValues(ConfigFile& file) const
{
  for (SIZE_T i = 0; i < m_layout.layers.size(); ++i) {
    const AnimationGraphLayerLayout& layer = m_layout.layers[i];
    file.setValue(kLayoutSection, StringUtils::format("Layer{0}", i),
                  positionsText(layer.statePositions));
    file.setValue(kLayoutSection, StringUtils::format("Layer{0}Any", i),
                  positionsText(Span<const Vector2>(&layer.anyStatePosition, 1)));
  }
}

/*
 */
void
AnimationGraphAsset::readEditorValues(const ConfigFile& file)
{
  m_layout.layers.clear();
  for (uint32 i = 0;; ++i) {
    const String* states = file.getValue(kLayoutSection, StringUtils::format("Layer{0}", i));
    if (!states) {
      break;
    }
    AnimationGraphLayerLayout& layer = m_layout.layers.emplace_back();
    readPositions(*states, layer.statePositions);
    Vector<Vector2> any;
    if (const String* anyText =
            file.getValue(kLayoutSection, StringUtils::format("Layer{0}Any", i))) {
      readPositions(*anyText, any);
    }
    if (!any.empty()) {
      layer.anyStatePosition = any[0];
    }
  }
}
#endif // USING(CH_EDITOR)

} // namespace chEngineSDK
