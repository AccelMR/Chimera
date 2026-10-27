/************************************************************************/
/**
 * @file chAnimatorComponent.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Component that animates the model of its GameObject.
 */
/************************************************************************/
#include "chAnimatorComponent.h"

#include "chAnimationClipAsset.h"
#include "chAnimationGraphAsset.h"
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chMath.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
struct AnimatorFileData
{
  UUID clip;
  UUID graph;
  float speed;
  uint32 flags;
};
static_assert(sizeof(AnimatorFileData) == 40, "AnimatorFileData must have no padding");

constexpr uint32 kPlaying = 1u << 0;
constexpr uint32 kLooping = 1u << 1;

const SPtr<Model> kNoModel;

/*
 * True when name is the text, or ends with '_' and the text.
 */
NODISCARD bool
isNameEnding(StringView name, StringView text)
{
  return name.size() > text.size() && name.ends_with(text) &&
         name[name.size() - text.size() - 1] == '_';
}
} // namespace

/*
 * The pose is seen in the editor without pressing Play, as in a preview.
 */
AnimatorComponent::AnimatorComponent()
{
  setUpdateFlags(ComponentUpdate::Frame);
  setUpdatesWhileEditing(true);
}

/*
 */
AnimatorComponent::AnimatorComponent(const SPtr<AnimationClipAsset>& clip)
  : AnimatorComponent()
{
  play(clip);
}

/*
 */
void
AnimatorComponent::play(const SPtr<AnimationClipAsset>& clip, float fadeTime)
{
  if (m_graph && !clip) {
    stop(fadeTime);
    return;
  }

  m_clipAsset = clip;
  m_clip = clip ? clip->getClip() : nullptr;
  m_clipAssetId = clip ? clip->getUUID() : UUID::null();
  if (m_graph) {
    // The player only crossfades between clips; the fade from the graph is the weight's.
    m_player.play(m_clip, m_clipWeight > 0.0f ? fadeTime : 0.0f);
    m_bClipOverGraph = true;
    m_clipFadeTime = fadeTime;
    if (fadeTime <= 0.0f) {
      m_clipWeight = 1.0f;
    }
  }
  else {
    m_player.play(m_clip, fadeTime);
  }
  // The first pose of the clip shows at once, not one frame later.
  if (isRegistered()) {
    update(0.0f);
  }
}

/*
 */
void
AnimatorComponent::stop(float fadeTime)
{
  if (!m_graph) {
    play(nullptr, fadeTime);
    return;
  }
  m_bClipOverGraph = false;
  m_clipFadeTime = fadeTime;
  if (fadeTime <= 0.0f) {
    m_clipWeight = 0.0f;
  }
  if (isRegistered()) {
    update(0.0f);
  }
}

/*
 */
void
AnimatorComponent::setPlaying(bool bPlaying)
{
  if (m_graph) {
    m_bGraphPaused = !bPlaying;
  }
  else {
    m_player.setPlaying(bPlaying);
  }
}

/*
 */
bool
AnimatorComponent::isPlaying() const noexcept
{
  return m_graph ? !m_bGraphPaused : m_player.isPlaying();
}

/*
 */
void
AnimatorComponent::setGraph(const SPtr<AnimationGraphAsset>& graph)
{
  m_graphAsset = graph;
  m_graph = graph ? graph->getGraph() : nullptr;
  m_graphAssetId = graph ? graph->getUUID() : UUID::null();
  m_graphInstance.setModel(m_player.getModel());
  m_graphInstance.setGraph(m_graph);
  m_bGraphPaused = false;
  m_bClipOverGraph = false;
  m_clipWeight = 0.0f;
  clearClip();
  if (isRegistered()) {
    update(0.0f);
  }
}

/*
 */
void
AnimatorComponent::clearClip()
{
  m_clipAsset = nullptr;
  m_clip = nullptr;
  m_clipAssetId = UUID::null();
  m_player.play(nullptr, 0.0f);
}

/*
 */
ModelComponent*
AnimatorComponent::bindModel()
{
  ModelComponent* model = getOwner()->getComponent<ModelComponent>();
  const SPtr<Model>& target = model && model->isWholeModel() ? model->getModel() : kNoModel;
  if (target != m_player.getModel()) {
    const float time = m_player.getTime();
    const bool bPlaying = m_player.isPlaying();
    m_player.setModel(target);
    m_player.play(m_clip, 0.0f);
    m_player.setTime(time);
    m_player.setPlaying(bPlaying);
    m_graphInstance.setModel(target);
    const SIZE_T nodeCount = target ? target->getNodeCount() : 0;
    m_blendedPose.resize(nodeCount);
    m_blendedMatrices.resize(nodeCount);
  }
  return model;
}

/*
 */
void
AnimatorComponent::update(float deltaTime)
{
  ModelComponent* model = bindModel();
  if (!model) {
    return;
  }
  if (m_graph) {
    updateGraph(*model, deltaTime);
    return;
  }
  // The model forgets the pose when its own model is set again (undo, a new asset).
  const bool bChanged = m_player.advance(deltaTime);
  if (bChanged || model->getPose().data() != m_player.getPoseMatrices().data()) {
    model->setPose(m_player.getPoseMatrices());
  }
}

/*
 */
void
AnimatorComponent::updateGraph(ModelComponent& model, float deltaTime)
{
  if (!m_graphInstance.isBound()) {
    return;
  }
  const float step = m_bGraphPaused ? 0.0f : deltaTime;
  bool bChanged = !m_bGraphPaused && m_graphInstance.advance(step);

  if (m_bClipOverGraph || m_clipWeight > 0.0f) {
    bChanged = m_player.advance(step) || bChanged;
    // A clip that does not loop gives the pose back to the graph once it ends.
    if (m_bClipOverGraph && m_clip && !m_player.isPlaying()) {
      m_bClipOverGraph = false;
    }
    const float target = m_bClipOverGraph ? 1.0f : 0.0f;
    if (m_clipFadeTime <= 0.0f) {
      m_clipWeight = target;
    }
    else if (m_clipWeight != target) {
      const float move = step / m_clipFadeTime;
      m_clipWeight = m_bClipOverGraph ? Math::min(m_clipWeight + move, 1.0f)
                                      : Math::max(m_clipWeight - move, 0.0f);
    }
    bChanged = true;

    if (m_clipWeight > 0.0f) {
      AnimationPlayer::blendPoses(m_graphInstance.getLocalPose(), m_player.getLocalPose(),
                                  AnimationPlayer::getFadeWeight(m_clipWeight, 1.0f), {},
                                  m_blendedPose);
      AnimationPlayer::buildPoseMatrices(*m_player.getModel(), m_blendedPose,
                                         m_blendedMatrices);
    }
    else if (!m_bClipOverGraph) {
      clearClip();
    }
  }

  const Span<const Matrix4> pose = getShownPose();
  if (bChanged || model.getPose().data() != pose.data()) {
    model.setPose(pose);
  }
}

/*
 */
Span<const Matrix4>
AnimatorComponent::getShownPose() const noexcept
{
  if (!m_graph) {
    return m_player.getPoseMatrices();
  }
  if (m_clipWeight > 0.0f) {
    return m_blendedMatrices;
  }
  return m_graphInstance.getPoseMatrices();
}

/*
 */
void
AnimatorComponent::onRegister()
{
  update(0.0f);
}

/*
 * The model goes back to rest, since the matrices it reads belong to this component.
 */
void
AnimatorComponent::onUnregister()
{
  ModelComponent* model = getOwner()->getComponent<ModelComponent>();
  const Span<const Matrix4> pose = getShownPose();
  if (model && !model->getPose().empty() && model->getPose().data() == pose.data()) {
    model->setPose({});
  }
}

/*
 */
SPtr<AnimationClipAsset>
AnimatorComponent::findClip(StringView pathOrName, String& outError)
{
  for (const auto& [text, clip] : m_foundClips) {
    if (text == pathOrName) {
      return clip;
    }
  }

  if (!AssetManager::isStarted()) {
    outError = "there are no assets to search";
    return nullptr;
  }
  AssetManager& assetManager = AssetManager::instance();
  const UUID& clipType = AssetTypeTraits<AnimationClipAsset>::getTypeId();

  SPtr<IAsset> found;
  if (pathOrName.find('/') != StringView::npos) {
    found = assetManager.findAsset(clipType, pathOrName);
    if (!found) {
      outError = StringUtils::format("there is no animation clip {0}", pathOrName);
      return nullptr;
    }
  }
  else {
    ModelComponent* model = getOwner()->getComponent<ModelComponent>();
    if (!model || !model->getModel()) {
      outError = "the object has no model";
      return nullptr;
    }
    const UUID& skeletonId = model->getModel()->getSkeletonId();
    const SPtr<ModelAsset>& modelAsset = model->getModelAsset();
    const StringView modelFolder = modelAsset ? StringView(modelAsset->getAssetPath())
                                              : StringView();

    Vector<SPtr<IAsset>> clips;
    assetManager.getAssetsOfType(clipType, clips);
    // A full name first: "Run" must not also take "A_Hero_Run" when a clip is named "Run".
    for (const bool bFullName : {true, false}) {
      uint32 fitCount = 0;
      for (const SPtr<IAsset>& asset : clips) {
        const StringView name = asset->getName();
        if (bFullName ? name != pathOrName : !isNameEnding(name, pathOrName)) {
          continue;
        }
        if (!assetManager.syncLoadAsset(asset)) {
          continue;
        }
        const SPtr<AnimationClip>& clip = asset->as<AnimationClipAsset>()->getClip();
        const bool bSameSkeleton =
            clip && !skeletonId.isNull() && clip->getSkeletonId() == skeletonId;
        const bool bFits =
            clip && (bSameSkeleton || StringView(asset->getAssetPath()) == modelFolder);
        if (bFits) {
          found = asset;
          ++fitCount;
        }
      }
      if (fitCount > 1) {
        outError = StringUtils::format("several animation clips are named {0}; use the path",
                                       pathOrName);
        return nullptr;
      }
      if (found) {
        break;
      }
    }
    if (!found) {
      outError =
          StringUtils::format("no animation clip named {0} fits the model", pathOrName);
      return nullptr;
    }
  }

  if (!assetManager.syncLoadAsset(found)) {
    outError = StringUtils::format("the animation clip {0} could not be loaded", pathOrName);
    return nullptr;
  }
  SPtr<AnimationClipAsset> clip = found->as<AnimationClipAsset>();
  m_foundClips.emplace_back(String(pathOrName), clip);
  return clip;
}

/*
 */
void
AnimatorComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  // A clip over a graph only lasts until it ends, so it is not saved.
  const AnimatorFileData data{
      .clip = m_graphAssetId.isNull() ? m_clipAssetId : UUID::null(),
      .graph = m_graphAssetId,
      .speed = m_player.getSpeed(),
      .flags = (isPlaying() ? kPlaying : 0u) | (m_player.isLooping() ? kLooping : 0u)};
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
}

/*
 */
bool
AnimatorComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  AnimatorFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data)) {
    return false;
  }

  SPtr<AnimationGraphAsset> graph;
  if (!data.graph.isNull() && AssetManager::isStarted()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(data.graph);
    graph = asset ? asset->as<AnimationGraphAsset>() : nullptr;
  }
  setGraph(graph);
  m_graphAssetId = data.graph;
  if (!graph && !data.graph.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "An animator lost its animation graph {0}.", data.graph);
  }

  SPtr<AnimationClipAsset> clip;
  if (!data.clip.isNull() && AssetManager::isStarted()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(data.clip);
    clip = asset ? asset->as<AnimationClipAsset>() : nullptr;
  }
  play(clip);
  m_clipAssetId = data.clip;
  if (!clip && !data.clip.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "An animator lost its animation clip {0}.", data.clip);
  }
  // No model yet: bindModel keeps these when the model is bound.
  m_player.setSpeed(data.speed);
  m_player.setLooping((data.flags & kLooping) != 0);
  setPlaying((data.flags & kPlaying) != 0);
  return true;
}

/*
 */
void
AnimatorComponent::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_clipAssetId.isNull()) {
    outReferences.push_back(m_clipAssetId);
  }
  if (!m_graphAssetId.isNull()) {
    outReferences.push_back(m_graphAssetId);
  }
}

} // namespace chEngineSDK
