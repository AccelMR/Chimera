/************************************************************************/
/**
 * @file chAnimationPlayer.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Plays animation clips on the nodes of a model and blends between them.
 */
/************************************************************************/
#include "chAnimationPlayer.h"

#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chModel.h"

namespace chEngineSDK {
namespace {
/*
 * Eases in and out, so neither end of a fade shows as a sudden change of speed.
 */
NODISCARD FORCEINLINE float
smoothFade(float alpha) noexcept
{
  return alpha * alpha * (3.0f - 2.0f * alpha);
}
} // namespace

/*
 */
void
AnimationPlayer::setModel(const SPtr<Model>& model)
{
  m_model = model;
  m_clip = nullptr;
  m_time = 0.0f;
  m_fadeDuration = 0.0f;
  m_fadeElapsed = 0.0f;
  m_bBoundTracks = false;
  m_bAnimating = false;
  m_bPoseDirty = false;
  m_trackNodes.clear();

  const SIZE_T nodeCount = model ? model->getNodeCount() : 0;
  m_restPose.resize(nodeCount);
  for (SIZE_T i = 0; i < nodeCount; ++i) {
    const ModelNode& node = model->getNodes()[i];
    m_restPose[i] = {.position = node.position, .rotation = node.rotation, .scale = node.scale};
  }
  m_sampledPose = m_restPose;
  m_fadeFromPose = m_restPose;
  m_localPose = m_restPose;
  m_poseMatrices.resize(nodeCount);
}

/*
 */
void
AnimationPlayer::play(const SPtr<AnimationClip>& clip, float fadeTime)
{
  if (clip && clip == m_clip) {
    setPlaying(true);
    return;
  }

  // The fade starts from what is shown now, whatever it was blending.
  if (fadeTime > 0.0f && m_model) {
    m_fadeFromPose = m_bAnimating ? m_localPose : m_restPose;
    m_fadeDuration = fadeTime;
    m_fadeElapsed = 0.0f;
  }
  else {
    m_fadeDuration = 0.0f;
    m_fadeElapsed = 0.0f;
  }

  m_clip = clip;
  m_time = 0.0f;
  m_bPlaying = true;
  bindClip();
  m_bAnimating = m_bBoundTracks || m_fadeElapsed < m_fadeDuration;
  m_bPoseDirty = true;
}

/*
 */
void
AnimationPlayer::bindClip()
{
  m_trackNodes.clear();
  m_bBoundTracks = false;
  if (!m_model || !m_clip) {
    return;
  }

  // Once per clip change, so a walk over the nodes per track is fine.
  const Vector<ModelNode>& nodes = m_model->getNodes();
  m_trackNodes.reserve(m_clip->getTracks().size());
  for (const AnimationTrack& track : m_clip->getTracks()) {
    uint32 target = AnimationClip::NO_TARGET;
    for (uint32 i = 0; i < nodes.size(); ++i) {
      if (nodes[i].name == track.nodeName) {
        target = i;
        break;
      }
    }
    m_trackNodes.push_back(target);
    m_bBoundTracks = m_bBoundTracks || target != AnimationClip::NO_TARGET;
  }
}

/*
 */
void
AnimationPlayer::setPlaying(bool bPlaying)
{
  if (bPlaying && !m_bPlaying && !m_bLooping && m_clip) {
    const float duration = m_clip->getDuration();
    if (m_speed >= 0.0f && m_time >= duration) {
      setTime(0.0f);
    }
    else if (m_speed < 0.0f && m_time <= 0.0f) {
      setTime(duration);
    }
  }
  m_bPlaying = bPlaying;
}

/*
 */
void
AnimationPlayer::setTime(float time)
{
  m_time = Math::clamp(time, 0.0f, getDuration());
  m_bPoseDirty = true;
}

/*
 */
float
AnimationPlayer::getDuration() const noexcept
{
  return m_clip ? m_clip->getDuration() : 0.0f;
}

/*
 */
bool
AnimationPlayer::advance(float deltaTime)
{
  if (!m_bAnimating) {
    // A fade back to rest that just ended still has to show the rest pose once.
    const bool bChanged = m_bPoseDirty;
    m_bPoseDirty = false;
    return bChanged;
  }

  if (m_fadeElapsed < m_fadeDuration) {
    m_fadeElapsed += deltaTime;
    m_bPoseDirty = true;
  }

  if (m_bBoundTracks && m_bPlaying && m_speed != 0.0f) {
    const float duration = m_clip->getDuration();
    float time = m_time + deltaTime * m_speed;
    if (m_bLooping && duration > 0.0f) {
      time -= Math::floor(time / duration) * duration;
    }
    else if (time >= duration || time <= 0.0f) {
      time = Math::clamp(time, 0.0f, duration);
      m_bPlaying = false;
    }
    m_time = time;
    m_bPoseDirty = true;
  }

  if (!m_bPoseDirty) {
    return false;
  }
  m_bPoseDirty = false;
  computePose();
  m_bAnimating = m_bBoundTracks || m_fadeElapsed < m_fadeDuration;
  return true;
}

/*
 */
void
AnimationPlayer::computePose()
{
  // From the rest pose each time, so nodes the clip does not move go back to rest when a
  // clip that moved them fades out.
  m_sampledPose = m_restPose;
  if (m_bBoundTracks) {
    m_clip->sample(m_time, m_trackNodes, m_sampledPose);
  }

  if (m_fadeElapsed < m_fadeDuration) {
    const float weight = smoothFade(Math::clamp(m_fadeElapsed / m_fadeDuration, 0.0f, 1.0f));
    blendPoses(m_fadeFromPose, m_sampledPose, weight, {}, m_localPose);
  }
  else {
    // The sampled pose is rebuilt from rest next time, so swapping costs no copy.
    m_localPose.swap(m_sampledPose);
  }

  const Vector<ModelNode>& nodes = m_model->getNodes();
  // Parents come first, so each parent's matrix is ready when its children need it.
  for (SIZE_T i = 0; i < nodes.size(); ++i) {
    const NodeTransform& local = m_localPose[i];
    const Matrix4 localMatrix =
        ScaleRotationTranslationMatrix(local.scale, local.rotation, local.position);
    m_poseMatrices[i] = nodes[i].parent == ModelNode::NO_PARENT
                            ? localMatrix
                            : localMatrix * m_poseMatrices[nodes[i].parent];
  }
}

/*
 */
void
AnimationPlayer::blendPoses(Span<const NodeTransform> from,
                            Span<const NodeTransform> to,
                            float weight,
                            Span<const float> nodeWeights,
                            Span<NodeTransform> out) noexcept
{
  CH_ASSERT(from.size() == to.size() && to.size() == out.size());
  CH_ASSERT(nodeWeights.empty() || nodeWeights.size() == out.size());
  for (SIZE_T i = 0; i < out.size(); ++i) {
    const float alpha = nodeWeights.empty() ? weight : weight * nodeWeights[i];
    out[i].position = from[i].position + (to[i].position - from[i].position) * alpha;
    out[i].rotation = Quaternion::nlerp(from[i].rotation, to[i].rotation, alpha);
    out[i].scale = from[i].scale + (to[i].scale - from[i].scale) * alpha;
  }
}

} // namespace chEngineSDK
