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
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
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
  float speed;
  uint32 flags;
};
static_assert(sizeof(AnimatorFileData) == 24, "AnimatorFileData must have no padding");

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
  m_clipAsset = clip;
  m_clip = clip ? clip->getClip() : nullptr;
  m_clipAssetId = clip ? clip->getUUID() : UUID::null();
  m_player.play(m_clip, fadeTime);
  // The first pose of the clip shows at once, not one frame later.
  if (isRegistered()) {
    update(0.0f);
  }
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
  // The model forgets the pose when its own model is set again (undo, a new asset).
  const bool bChanged = m_player.advance(deltaTime);
  if (bChanged || model->getPose().data() != m_player.getPoseMatrices().data()) {
    model->setPose(m_player.getPoseMatrices());
  }
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
  if (model && !model->getPose().empty() &&
      model->getPose().data() == m_player.getPoseMatrices().data()) {
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
  const AnimatorFileData data{
      .clip = m_clipAssetId,
      .speed = m_player.getSpeed(),
      .flags = (m_player.isPlaying() ? kPlaying : 0u) | (m_player.isLooping() ? kLooping : 0u)};
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
  m_player.setPlaying((data.flags & kPlaying) != 0);
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
}

} // namespace chEngineSDK
