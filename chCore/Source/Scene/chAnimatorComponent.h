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

#include "chAnimationPlayer.h"
#include "chComponent.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class AnimationClipAsset;
class ModelComponent;

/**
 * Exists so drawing a model and animating it are separate jobs, as Unity's Animator: the
 * ModelComponent of the same GameObject draws, and this component decides its pose with an
 * AnimationPlayer (which clip plays, its time, speed, loop and the crossfade to the next).
 * The clip saved with the scene is the one playing when it was saved; it starts on its own.
 * Works on a whole model only; a part stays at rest.
 */
class CH_CORE_EXPORT AnimatorComponent : public Component
{
 public:
  AnimatorComponent();

  explicit AnimatorComponent(const SPtr<AnimationClipAsset>& clip);

  /**
   * Plays clip from its start, fading from the pose shown now over fadeTime seconds; null
   * fades back to the rest pose. Playing the clip already playing only resumes it, so it
   * can be called every frame.
   */
  void
  play(const SPtr<AnimationClipAsset>& clip, float fadeTime = 0.0f);

  NODISCARD FORCEINLINE const SPtr<AnimationClipAsset>&
  getClipAsset() const noexcept { return m_clipAsset; }

  /**
   * The clip of the asset, kept while the component uses it, even if the asset unloads.
   */
  NODISCARD FORCEINLINE const SPtr<AnimationClip>&
  getClip() const noexcept { return m_clip; }

  /**
   * False when no node of the model has the name of a track of the clip, or there is no
   * model to animate.
   */
  NODISCARD FORCEINLINE bool
  hasBoundTracks() const noexcept { return m_player.hasBoundTracks(); }

  /**
   * Playing a clip that does not loop and already reached its end starts it again.
   */
  FORCEINLINE void
  setPlaying(bool bPlaying) { m_player.setPlaying(bPlaying); }

  NODISCARD FORCEINLINE bool
  isPlaying() const noexcept { return m_player.isPlaying(); }

  FORCEINLINE void
  setLooping(bool bLooping) noexcept { m_player.setLooping(bLooping); }

  NODISCARD FORCEINLINE bool
  isLooping() const noexcept { return m_player.isLooping(); }

  /**
   * Seconds of the clip per second of play; negative plays backwards.
   */
  FORCEINLINE void
  setSpeed(float speed) noexcept { m_player.setSpeed(speed); }

  NODISCARD FORCEINLINE float
  getSpeed() const noexcept { return m_player.getSpeed(); }

  /**
   * Jumps to a time of the clip (clamped to it); the pose follows on the next update.
   */
  FORCEINLINE void
  setTime(float time) { m_player.setTime(time); }

  NODISCARD FORCEINLINE float
  getTime() const noexcept { return m_player.getTime(); }

  NODISCARD FORCEINLINE float
  getDuration() const noexcept { return m_player.getDuration(); }

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
   * The ModelComponent of the owner, after starting the player again on its model when it
   * changed (keeping the clip, its time and whether it plays). Null when there is none.
   */
  ModelComponent*
  bindModel();

  SPtr<AnimationClipAsset> m_clipAsset;
  SPtr<AnimationClip> m_clip;
  // Kept when the asset is missing, so saving again keeps the reference.
  UUID m_clipAssetId = UUID::null();
  AnimationPlayer m_player;
  // What findClip answered, by the text it was asked.
  Vector<Pair<String, SPtr<AnimationClipAsset>>> m_foundClips;
};
DECLARE_TYPE_TRAITS(AnimatorComponent)

} // namespace chEngineSDK
