/************************************************************************/
/**
 * @file chAnimationPlayer.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Plays animation clips on the nodes of a model and blends between them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAnimationClip.h"
#include "chMatrix4.h"

namespace chEngineSDK {
class Model;

/**
 * Exists to turn clips into the pose of one model: which clip plays, its time, speed and
 * loop, and the crossfade when another clip starts. A new clip fades in from a copy of the
 * pose shown when it started, so starting a third clip during a fade still blends two poses
 * only and the cost per frame stays the same. Buffers are sized once per model, so playing
 * and blending never allocate. The animator and the graphs to come drive it; it knows no
 * scene.
 */
class CH_CORE_EXPORT AnimationPlayer
{
 public:
  /**
   * Starts at the rest pose of model (null plays nothing) and forgets the clip.
   */
  void
  setModel(const SPtr<Model>& model);

  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const noexcept { return m_model; }

  /**
   * Plays clip from its start, fading from the pose shown now over fadeTime seconds; null
   * fades back to the rest pose. Playing the clip already playing only resumes it (from the
   * start when it reached its end without looping), so it can be called every frame.
   */
  void
  play(const SPtr<AnimationClip>& clip, float fadeTime);

  NODISCARD FORCEINLINE const SPtr<AnimationClip>&
  getClip() const noexcept { return m_clip; }

  /**
   * True while a clip moves at least one node of the model or a fade runs: the pose may
   * differ from the rest pose.
   */
  NODISCARD FORCEINLINE bool
  isAnimating() const noexcept { return m_bAnimating; }

  /**
   * True when the clip moves at least one node of the model.
   */
  NODISCARD FORCEINLINE bool
  hasBoundTracks() const noexcept { return m_bBoundTracks; }

  /**
   * Playing a clip that does not loop and already reached its end starts it again.
   */
  void
  setPlaying(bool bPlaying);

  NODISCARD FORCEINLINE bool
  isPlaying() const noexcept { return m_bPlaying; }

  FORCEINLINE void
  setLooping(bool bLooping) noexcept { m_bLooping = bLooping; }

  NODISCARD FORCEINLINE bool
  isLooping() const noexcept { return m_bLooping; }

  /**
   * Seconds of the clip per second of play; negative plays backwards.
   */
  FORCEINLINE void
  setSpeed(float speed) noexcept { m_speed = speed; }

  NODISCARD FORCEINLINE float
  getSpeed() const noexcept { return m_speed; }

  /**
   * Jumps to a time of the clip (clamped to it); the pose follows on the next advance.
   */
  void
  setTime(float time);

  NODISCARD FORCEINLINE float
  getTime() const noexcept { return m_time; }

  /**
   * Zero without a clip.
   */
  NODISCARD float
  getDuration() const noexcept;

  /**
   * Moves the clip and the fade by deltaTime seconds. True when the pose changed, so whoever
   * shows it must read it again.
   */
  NODISCARD bool
  advance(float deltaTime);

  /**
   * One matrix per node of the model, in the model's space; empty while nothing animates,
   * which means the rest pose. Valid until the next setModel.
   */
  NODISCARD FORCEINLINE Span<const Matrix4>
  getPoseMatrices() const noexcept
  {
    return m_bAnimating ? Span<const Matrix4>(m_poseMatrices) : Span<const Matrix4>();
  }

  /**
   * Writes the blend of from and to into out (all the same size): weight 0 is from, 1 is
   * to. nodeWeights, when not empty, scales weight per node, so a layer can move only some
   * bones. Positions and scales are lerped and rotations nlerped on the shorter way.
   */
  static void
  blendPoses(Span<const NodeTransform> from,
             Span<const NodeTransform> to,
             float weight,
             Span<const float> nodeWeights,
             Span<NodeTransform> out) noexcept;

 private:
  /**
   * Finds the node of each track of the clip by name.
   */
  void
  bindClip();

  /**
   * Samples the clip, blends it with the fade's start pose and rebuilds the matrices.
   */
  void
  computePose();

  SPtr<Model> m_model;
  SPtr<AnimationClip> m_clip;
  float m_time = 0.0f;
  float m_speed = 1.0f;
  bool m_bPlaying = true;
  bool m_bLooping = true;
  bool m_bBoundTracks = false;
  bool m_bAnimating = false;
  // The time changed without the pose following yet.
  bool m_bPoseDirty = false;

  float m_fadeDuration = 0.0f;
  // Seconds since the fade started; at m_fadeDuration or more there is no fade.
  float m_fadeElapsed = 0.0f;

  // Per track of the clip, the model node it moves, or AnimationClip::NO_TARGET.
  Vector<uint32> m_trackNodes;
  // Per model node: the rest pose, the clip sampled over it, the pose the fade starts from
  // and the pose shown, relative to the parent; then the shown pose in the model's space.
  Vector<NodeTransform> m_restPose;
  Vector<NodeTransform> m_sampledPose;
  Vector<NodeTransform> m_fadeFromPose;
  Vector<NodeTransform> m_localPose;
  Vector<Matrix4> m_poseMatrices;
};

} // namespace chEngineSDK
