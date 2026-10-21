/************************************************************************/
/**
 * @file chAnimationClip.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Keys that move the nodes of a model over time.
 */
/************************************************************************/
#include "chAnimationClip.h"

#include "chAssetFile.h"
#include "chFileStream.h"
#include "chMath.h"

namespace chEngineSDK {
namespace {
// Above any real clip; only a broken file asks for more.
constexpr uint32 kMaxTracks = 65536;
constexpr uint32 kMaxFrames = 1u << 20;
constexpr uint32 kMaxNameLength = 1024;
// Keys closer than this to the first one count as the same, so a channel that only moves by
// rounding error is stored once.
constexpr float kConstantTolerance = 1e-5f;

struct ClipFileData
{
  float sampleRate;
  uint32 frameCount;
  UUID skeletonId;
  uint32 trackCount;
  uint32 positionCount;
  uint32 rotationCount;
  uint32 scaleCount;
};
static_assert(sizeof(ClipFileData) == 40, "ClipFileData must have no padding");

struct TrackFileData
{
  uint32 firstPosition;
  uint32 positionCount;
  uint32 firstRotation;
  uint32 rotationCount;
  uint32 firstScale;
  uint32 scaleCount;
};
static_assert(sizeof(TrackFileData) == 24, "TrackFileData must have no padding");

/*
 * Appends one key when every key equals the first, else all of them. Returns the count.
 */
template<typename T, typename Equal>
uint32
appendKeys(Vector<T>& outKeys, Span<const T> keys, Equal&& isEqual)
{
  bool bConstant = true;
  for (const T& key : keys.subspan(1)) {
    if (!isEqual(keys[0], key)) {
      bConstant = false;
      break;
    }
  }
  const SIZE_T count = bConstant ? 1 : keys.size();
  outKeys.insert(outKeys.end(), keys.begin(), keys.begin() + count);
  return static_cast<uint32>(count);
}

NODISCARD FORCEINLINE Vector3
lerpVector(const Vector3& from, const Vector3& to, float alpha)
{
  return from + (to - from) * alpha;
}

/*
 * A channel's range is valid when it has one key or one per frame inside the array.
 */
NODISCARD bool
isValidRange(uint32 first, uint32 count, uint32 frameCount, uint32 keyCount)
{
  return (count == 1 || count == frameCount) && first <= keyCount && count <= keyCount - first;
}
} // namespace

/*
 */
AnimationClip::AnimationClip(float sampleRate, uint32 frameCount)
  : m_sampleRate(Math::max(sampleRate, 1.0f)),
    m_frameCount(Math::max(frameCount, 1u))
{}

/*
 */
uint32
AnimationClip::addTrack(StringView nodeName,
                        Span<const Vector3> positions,
                        Span<const Quaternion> rotations,
                        Span<const Vector3> scales)
{
  CH_ASSERT(positions.size() == m_frameCount && rotations.size() == m_frameCount &&
            scales.size() == m_frameCount);

  // q and -q are the same rotation; keeping every key on the side of the one before makes
  // the blend between two keys always take the short way.
  Vector<Quaternion> continuous(rotations.begin(), rotations.end());
  for (SIZE_T i = 1; i < continuous.size(); ++i) {
    if (continuous[i].dot(continuous[i - 1]) < 0.0f) {
      continuous[i] *= -1.0f;
    }
  }

  const auto sameVector = [](const Vector3& a, const Vector3& b) {
    return a.nearEqual(b, kConstantTolerance);
  };
  const auto sameRotation = [](const Quaternion& a, const Quaternion& b) {
    return a.nearEqual(b, kConstantTolerance);
  };

  AnimationTrack track;
  track.nodeName = nodeName;
  track.firstPosition = static_cast<uint32>(m_positions.size());
  track.positionCount = appendKeys(m_positions, positions, sameVector);
  track.firstRotation = static_cast<uint32>(m_rotations.size());
  track.rotationCount =
      appendKeys(m_rotations, Span<const Quaternion>(continuous), sameRotation);
  track.firstScale = static_cast<uint32>(m_scales.size());
  track.scaleCount = appendKeys(m_scales, scales, sameVector);
  m_tracks.push_back(std::move(track));
  return static_cast<uint32>(m_tracks.size() - 1);
}

/*
 */
void
AnimationClip::sample(float time,
                      Span<const uint32> trackTargets,
                      Span<NodeTransform> outPose) const
{
  // Every track has its keys at the same times, so the frames around time are found once.
  const float lastFrame = static_cast<float>(m_frameCount - 1);
  const float frame = Math::clamp(time * m_sampleRate, 0.0f, lastFrame);
  const uint32 frameIndex = Math::min(static_cast<uint32>(frame), m_frameCount - 1);
  const uint32 nextIndex = Math::min(frameIndex + 1, m_frameCount - 1);
  const float alpha = frame - static_cast<float>(frameIndex);

  const SIZE_T trackCount = Math::min(m_tracks.size(), trackTargets.size());
  for (SIZE_T i = 0; i < trackCount; ++i) {
    const uint32 target = trackTargets[i];
    if (target == NO_TARGET || target >= outPose.size()) {
      continue;
    }
    const AnimationTrack& track = m_tracks[i];
    NodeTransform& pose = outPose[target];

    const Vector3* positions = m_positions.data() + track.firstPosition;
    pose.position = track.positionCount == 1
                        ? positions[0]
                        : lerpVector(positions[frameIndex], positions[nextIndex], alpha);
    const Quaternion* rotations = m_rotations.data() + track.firstRotation;
    pose.rotation =
        track.rotationCount == 1
            ? rotations[0]
            : Quaternion::nlerp(rotations[frameIndex], rotations[nextIndex], alpha);
    const Vector3* scales = m_scales.data() + track.firstScale;
    pose.scale = track.scaleCount == 1
                     ? scales[0]
                     : lerpVector(scales[frameIndex], scales[nextIndex], alpha);
  }
}

/*
 */
void
AnimationClip::serialize(DataStream& stream) const
{
  const ClipFileData data{.sampleRate = m_sampleRate,
                          .frameCount = m_frameCount,
                          .skeletonId = m_skeletonId,
                          .trackCount = static_cast<uint32>(m_tracks.size()),
                          .positionCount = static_cast<uint32>(m_positions.size()),
                          .rotationCount = static_cast<uint32>(m_rotations.size()),
                          .scaleCount = static_cast<uint32>(m_scales.size())};
  stream << SERIALIZATION_VERSION;
  stream << data;
  for (const AnimationTrack& track : m_tracks) {
    const TrackFileData trackData{.firstPosition = track.firstPosition,
                                  .positionCount = track.positionCount,
                                  .firstRotation = track.firstRotation,
                                  .rotationCount = track.rotationCount,
                                  .firstScale = track.firstScale,
                                  .scaleCount = track.scaleCount};
    AssetFile::writeString(stream, track.nodeName);
    stream << trackData;
  }
  stream.write(m_positions.data(), m_positions.size() * sizeof(Vector3));
  stream.write(m_rotations.data(), m_rotations.size() * sizeof(Quaternion));
  stream.write(m_scales.data(), m_scales.size() * sizeof(Vector3));
}

/*
 */
SPtr<AnimationClip>
AnimationClip::deserialize(DataStream& stream)
{
  uint32 version = 0;
  ClipFileData data;
  if (!AssetFile::readValue(stream, version) || version != SERIALIZATION_VERSION ||
      !AssetFile::readValue(stream, data) || !(data.sampleRate >= 1.0f) ||
      data.frameCount == 0 || data.frameCount > kMaxFrames || data.trackCount > kMaxTracks) {
    return nullptr;
  }
  // Checked against the bytes left before anything is allocated for them.
  const uint64 keyBytes = static_cast<uint64>(data.positionCount) * sizeof(Vector3) +
                          static_cast<uint64>(data.rotationCount) * sizeof(Quaternion) +
                          static_cast<uint64>(data.scaleCount) * sizeof(Vector3);
  if (keyBytes > AssetFile::getBytesLeft(stream)) {
    return nullptr;
  }

  SPtr<AnimationClip> clip = chMakeShared<AnimationClip>(data.sampleRate, data.frameCount);
  clip->m_skeletonId = data.skeletonId;
  clip->m_tracks.resize(data.trackCount);
  for (AnimationTrack& track : clip->m_tracks) {
    TrackFileData trackData;
    if (!AssetFile::readString(stream, track.nodeName, kMaxNameLength) ||
        !AssetFile::readValue(stream, trackData) ||
        !isValidRange(trackData.firstPosition, trackData.positionCount, data.frameCount,
                      data.positionCount) ||
        !isValidRange(trackData.firstRotation, trackData.rotationCount, data.frameCount,
                      data.rotationCount) ||
        !isValidRange(trackData.firstScale, trackData.scaleCount, data.frameCount,
                      data.scaleCount)) {
      return nullptr;
    }
    track.firstPosition = trackData.firstPosition;
    track.positionCount = trackData.positionCount;
    track.firstRotation = trackData.firstRotation;
    track.rotationCount = trackData.rotationCount;
    track.firstScale = trackData.firstScale;
    track.scaleCount = trackData.scaleCount;
  }

  clip->m_positions.resize(data.positionCount);
  clip->m_rotations.resize(data.rotationCount);
  clip->m_scales.resize(data.scaleCount);
  if (!AssetFile::readBytes(stream, clip->m_positions.data(),
                            clip->m_positions.size() * sizeof(Vector3)) ||
      !AssetFile::readBytes(stream, clip->m_rotations.data(),
                            clip->m_rotations.size() * sizeof(Quaternion)) ||
      !AssetFile::readBytes(stream, clip->m_scales.data(),
                            clip->m_scales.size() * sizeof(Vector3))) {
    return nullptr;
  }
  return clip;
}

} // namespace chEngineSDK
