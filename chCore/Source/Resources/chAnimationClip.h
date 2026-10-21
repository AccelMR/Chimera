/************************************************************************/
/**
 * @file chAnimationClip.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Keys that move the nodes of a model over time.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chQuaternion.h"
#include "chUUID.h"
#include "chVector3.h"

namespace chEngineSDK {
class DataStream;

/**
 * Where a node stands relative to its parent: what an animation writes for each node.
 */
struct NodeTransform
{
  Vector3 position = Vector3::ZERO;
  Quaternion rotation = Quaternion::IDENTITY;
  Vector3 scale = Vector3::UNIT;
};

/**
 * The keys of one node. Each channel has one key when it never changes and one per frame
 * otherwise; the ranges index the clip's key arrays.
 */
struct AnimationTrack
{
  String nodeName;
  uint32 firstPosition = 0;
  uint32 positionCount = 0;
  uint32 firstRotation = 0;
  uint32 rotationCount = 0;
  uint32 firstScale = 0;
  uint32 scaleCount = 0;
};

/**
 * Exists to hold one animation of a source file, ready to sample every frame. Keys are
 * taken at a fixed rate when the file is imported, so finding the keys around a time is one
 * multiplication instead of a search, and every track shares it. Tracks name the nodes they
 * move, so a clip works on any model whose nodes have those names: the nodes of one model,
 * or the bones of every model of a skeleton. Read only once built.
 */
class CH_CORE_EXPORT AnimationClip
{
 public:
  AnimationClip() = default;

  /**
   * frameCount is at least 1; the clip lasts (frameCount - 1) / sampleRate seconds.
   */
  AnimationClip(float sampleRate, uint32 frameCount);

  /**
   * Adds the keys of one node, one per frame in each span. A channel whose keys are all the
   * same is kept as one key. Returns the index of the track.
   */
  uint32
  addTrack(StringView nodeName,
           Span<const Vector3> positions,
           Span<const Quaternion> rotations,
           Span<const Vector3> scales);

  /**
   * The skeleton the clip was made for, when its model had one; only informative, since
   * tracks find their nodes by name.
   */
  void
  setSkeletonId(const UUID& skeletonId) { m_skeletonId = skeletonId; }

  NODISCARD FORCEINLINE const UUID&
  getSkeletonId() const noexcept { return m_skeletonId; }

  NODISCARD FORCEINLINE float
  getSampleRate() const noexcept { return m_sampleRate; }

  NODISCARD FORCEINLINE uint32
  getFrameCount() const noexcept { return m_frameCount; }

  NODISCARD FORCEINLINE float
  getDuration() const noexcept
  {
    return static_cast<float>(m_frameCount - 1) / m_sampleRate;
  }

  NODISCARD FORCEINLINE const Vector<AnimationTrack>&
  getTracks() const noexcept { return m_tracks; }

  /**
   * Writes the pose at time (clamped to the clip) into outPose: track i goes to
   * outPose[trackTargets[i]], and tracks whose target is NO_TARGET are skipped. Nodes no
   * track moves are left as they were.
   */
  void
  sample(float time, Span<const uint32> trackTargets, Span<NodeTransform> outPose) const;

  void
  serialize(DataStream& stream) const;

  /**
   * Null when the data is broken.
   */
  NODISCARD static SPtr<AnimationClip>
  deserialize(DataStream& stream);

  static constexpr uint32 NO_TARGET = 0xFFFFFFFFu;
  static constexpr uint32 SERIALIZATION_VERSION = 1;

 private:
  float m_sampleRate = 30.0f;
  uint32 m_frameCount = 1;
  UUID m_skeletonId = UUID::null();
  Vector<AnimationTrack> m_tracks;
  Vector<Vector3> m_positions;
  Vector<Quaternion> m_rotations;
  Vector<Vector3> m_scales;
};

} // namespace chEngineSDK
