/************************************************************************/
/**
 * @file chAnimatorComponent.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Component that animates the model of its GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAnimationGraphInstance.h"
#include "chAnimationPlayer.h"
#include "chComponent.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class AnimationClipAsset;
class AnimationGraphAsset;
class ModelComponent;

/**
 * Exists so drawing a model and animating it are separate jobs, as Unity's Animator: the
 * ModelComponent of the same GameObject draws, and this component decides its pose. Without
 * a graph it plays one clip at a time with an AnimationPlayer (which clip, its time, speed,
 * loop and the crossfade to the next); the clip saved with the scene is the one playing
 * when it was saved, and it starts on its own. With a graph, the graph decides the pose
 * from its parameters, and a clip given to play shows over it until the clip ends (one
 * that loops, until stop), fading in and back out over the fade time; the clip settings
 * (time, speed, loop) are that clip's. Works on a whole model only; a part stays at rest.
 */
class CH_CORE_EXPORT AnimatorComponent : public Component
{
 public:
  AnimatorComponent();

  explicit AnimatorComponent(const SPtr<AnimationClipAsset>& clip);

  /**
   * Plays clip from its start, fading from the pose shown now over fadeTime seconds; null is
   * stop. Playing the clip already playing only resumes it, so it can be called every frame.
   */
  void
  play(const SPtr<AnimationClipAsset>& clip, float fadeTime = 0.0f);

  /**
   * Without a graph, fades back to the rest pose; with one, fades the clip out so the graph
   * shows again.
   */
  void
  stop(float fadeTime = 0.0f);

  NODISCARD FORCEINLINE const SPtr<AnimationClipAsset>&
  getClipAsset() const noexcept
  {
    return m_clipAsset;
  }

  /**
   * The clip of the asset, kept while the component uses it, even if the asset unloads.
   */
  NODISCARD FORCEINLINE const SPtr<AnimationClip>&
  getClip() const noexcept
  {
    return m_clip;
  }

  /**
   * False when no node of the model has the name of a track of the clip, or there is no
   * model to animate.
   */
  NODISCARD FORCEINLINE bool
  hasBoundTracks() const noexcept
  {
    return m_player.hasBoundTracks();
  }

  /**
   * Pauses or resumes everything, the graph and the clip. Without a graph, playing a clip
   * that does not loop and already reached its end starts it again.
   */
  void
  setPlaying(bool bPlaying);

  NODISCARD bool
  isPlaying() const noexcept;

  FORCEINLINE void
  setLooping(bool bLooping) noexcept
  {
    m_player.setLooping(bLooping);
  }

  NODISCARD FORCEINLINE bool
  isLooping() const noexcept
  {
    return m_player.isLooping();
  }

  /**
   * Seconds of the clip per second of play; negative plays backwards.
   */
  FORCEINLINE void
  setSpeed(float speed) noexcept
  {
    m_player.setSpeed(speed);
  }

  NODISCARD FORCEINLINE float
  getSpeed() const noexcept
  {
    return m_player.getSpeed();
  }

  /**
   * Jumps to a time of the clip (clamped to it); the pose follows on the next update.
   */
  FORCEINLINE void
  setTime(float time)
  {
    m_player.setTime(time);
  }

  NODISCARD FORCEINLINE float
  getTime() const noexcept
  {
    return m_player.getTime();
  }

  NODISCARD FORCEINLINE float
  getDuration() const noexcept
  {
    return m_player.getDuration();
  }

  /**
   * The graph that decides the pose, starting from its defaults; null goes back to playing
   * clips. Either way the clip stops at once.
   */
  void
  setGraph(const SPtr<AnimationGraphAsset>& graph);

  NODISCARD FORCEINLINE const SPtr<AnimationGraphAsset>&
  getGraphAsset() const noexcept
  {
    return m_graphAsset;
  }

  /**
   * True when a graph decides the pose (its asset is set and loaded).
   */
  NODISCARD FORCEINLINE bool
  hasGraph() const noexcept
  {
    return m_graph != nullptr;
  }

  /**
   * The graph's parameters, states and layer weights for this object. Not bound while
   * there is no graph or no model.
   */
  NODISCARD FORCEINLINE AnimationGraphInstance&
  getGraphInstance() noexcept
  {
    return m_graphInstance;
  }

  NODISCARD FORCEINLINE const AnimationGraphInstance&
  getGraphInstance() const noexcept
  {
    return m_graphInstance;
  }

  /**
   * True while a clip shows over the graph, also while it fades in or out.
   */
  NODISCARD FORCEINLINE bool
  isClipOverGraph() const noexcept
  {
    return m_graph && (m_bClipOverGraph || m_clipWeight > 0.0f);
  }

  /**
   * The clip a text names for this object's model, loaded: a virtual path
   * (/Game/Hero/A_Hero_Run), or the name of a clip asset, also without what comes before
   * its last '_' (Run for A_Hero_Run). A name only counts clips made for the model's
   * skeleton or kept in the model's folder, and a full name wins over an ending. Answers
   * are kept, so asking again for the same text is a short search without allocations.
   * Null with the reason in outError when there is none, or several fit.
   */
  NODISCARD SPtr<AnimationClipAsset>
  findClip(StringView pathOrName, String& outError);

  void
  update(float deltaTime) override;

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  void
  onRegister() override;

  void
  onUnregister() override;

 private:
  /**
   * The ModelComponent of the owner, after starting the player and the graph again on its
   * model when it changed (the player keeps the clip, its time and whether it plays). Null
   * when there is none.
   */
  ModelComponent*
  bindModel();

  /**
   * Moves the graph, the clip over it and its fade, and gives the model the result.
   */
  void
  updateGraph(ModelComponent& model, float deltaTime);

  /**
   * Forgets the clip, once nothing shows it.
   */
  void
  clearClip();

  /**
   * The matrices the model reads from this component, whichever pose they hold.
   */
  NODISCARD Span<const Matrix4>
  getShownPose() const noexcept;

  SPtr<AnimationClipAsset> m_clipAsset;
  SPtr<AnimationClip> m_clip;
  // Kept when the asset is missing, so saving again keeps the reference.
  UUID m_clipAssetId = UUID::null();
  AnimationPlayer m_player;
  // What findClip answered, by the text it was asked.
  Vector<Pair<String, SPtr<AnimationClipAsset>>> m_foundClips;

  SPtr<AnimationGraphAsset> m_graphAsset;
  SPtr<AnimationGraph> m_graph;
  UUID m_graphAssetId = UUID::null();
  AnimationGraphInstance m_graphInstance;
  bool m_bGraphPaused = false;
  // The clip over the graph: whether it is wanted, how much it shows (0 to 1, before
  // easing) and how long its fade lasts.
  bool m_bClipOverGraph = false;
  float m_clipWeight = 0.0f;
  float m_clipFadeTime = 0.0f;
  // The graph pose with the clip blended over it, while a clip shows.
  Vector<NodeTransform> m_blendedPose;
  Vector<Matrix4> m_blendedMatrices;
};
DECLARE_TYPE_TRAITS(AnimatorComponent)

} // namespace chEngineSDK
