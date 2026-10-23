/************************************************************************/
/**
 * @file chAnimationGraphInstance.cpp
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Runs an AnimationGraph on the nodes of one model.
 */
/************************************************************************/
#include "chAnimationGraphInstance.h"

#include "chAlgorithm.h"
#include "chAnimationClipAsset.h"
#include "chAnimationPlayer.h"
#include "chBlendSpace.h"
#include "chMath.h"
#include "chModel.h"

namespace chEngineSDK {
namespace {
// Clips of one frame (a still pose) last zero seconds; their state counts as this long, so
// its normalized time still moves and exit times are reached.
constexpr float kMinStateDuration = 1e-3f;

NODISCARD FORCEINLINE float
getBlendValue(const AnimationParameter& parameter,
              const AnimationParameterValue& value) noexcept
{
  return parameter.type == AnimationParameterType::Float ? value.floatValue
                                                         : static_cast<float>(value.intValue);
}

/*
 * A looping state reaches an exit time below 1 in every loop, from that point to the end of
 * the loop; otherwise the time must have passed it.
 */
NODISCARD FORCEINLINE bool
isExitTimeReached(bool bLooping, float time, float exitTime) noexcept
{
  if (bLooping && exitTime < 1.0f) {
    return time - Math::floor(time) >= exitTime;
  }
  return time >= exitTime;
}

NODISCARD bool
conditionHolds(const AnimationParameter& parameter, const AnimationParameterValue& value,
               const AnimationCondition& condition) noexcept
{
  const bool bFloat = parameter.type == AnimationParameterType::Float;
  const float valueFloat = value.floatValue;
  const float thresholdFloat = condition.threshold.floatValue;
  const int32 valueInt = value.intValue;
  const int32 thresholdInt = condition.threshold.intValue;
  switch (condition.op) {
  case AnimationConditionOp::IsSet:
    return valueInt != 0;
  case AnimationConditionOp::IsNotSet:
    return valueInt == 0;
  case AnimationConditionOp::Greater:
    return bFloat ? valueFloat > thresholdFloat : valueInt > thresholdInt;
  case AnimationConditionOp::Less:
    return bFloat ? valueFloat < thresholdFloat : valueInt < thresholdInt;
  case AnimationConditionOp::GreaterOrEqual:
    return bFloat ? valueFloat >= thresholdFloat : valueInt >= thresholdInt;
  case AnimationConditionOp::LessOrEqual:
    return bFloat ? valueFloat <= thresholdFloat : valueInt <= thresholdInt;
  case AnimationConditionOp::Equal:
    return valueInt == thresholdInt;
  case AnimationConditionOp::NotEqual:
    return valueInt != thresholdInt;
  }
  return false;
}
} // namespace

/*
 */
AnimationGraphInstance::AnimationGraphInstance() = default;

/*
 */
AnimationGraphInstance::~AnimationGraphInstance() = default;

/*
 */
void
AnimationGraphInstance::setModel(const SPtr<Model>& model)
{
  m_model = model;
  bind();
}

/*
 */
void
AnimationGraphInstance::setGraph(const SPtr<AnimationGraph>& graph)
{
  m_graph = graph;
  bind();
}

/*
 */
void
AnimationGraphInstance::bind()
{
  m_bBound = m_model && m_graph && !m_graph->getLayers().empty();
  m_values.clear();
  m_clips.clear();
  m_clipTrackNodes.clear();
  m_blendSpaces.clear();
  m_layers.clear();
  if (!m_bBound) {
    m_restPose.clear();
    m_scratchPose.clear();
    m_localPose.clear();
    m_poseMatrices.clear();
    return;
  }

  const Vector<ModelNode>& nodes = m_model->getNodes();
  const SIZE_T nodeCount = nodes.size();
  m_restPose.resize(nodeCount);
  for (SIZE_T i = 0; i < nodeCount; ++i) {
    m_restPose[i] = {
        .position = nodes[i].position, .rotation = nodes[i].rotation, .scale = nodes[i].scale};
  }
  m_scratchPose = m_restPose;
  m_localPose = m_restPose;
  m_poseMatrices.resize(nodeCount);

  m_values.resize(m_graph->getParameters().size());

  // Once per model or graph change, so a walk over the nodes per track is fine.
  const Vector<AnimationGraphClip>& graphClips = m_graph->getClips();
  m_clips.resize(graphClips.size());
  m_clipTrackNodes.resize(graphClips.size());
  for (SIZE_T c = 0; c < graphClips.size(); ++c) {
    m_clips[c] = graphClips[c].clip ? graphClips[c].clip->getClip() : nullptr;
    if (!m_clips[c]) {
      continue;
    }
    const Vector<AnimationTrack>& tracks = m_clips[c]->getTracks();
    Vector<uint32>& trackNodes = m_clipTrackNodes[c];
    trackNodes.assign(tracks.size(), AnimationClip::NO_TARGET);
    for (SIZE_T t = 0; t < tracks.size(); ++t) {
      for (uint32 n = 0; n < nodeCount; ++n) {
        if (nodes[n].name == tracks[t].nodeName) {
          trackNodes[t] = n;
          break;
        }
      }
    }
  }

  const Vector<AnimationLayer>& graphLayers = m_graph->getLayers();
  m_layers.resize(graphLayers.size());
  for (SIZE_T l = 0; l < graphLayers.size(); ++l) {
    const AnimationLayer& graphLayer = graphLayers[l];
    LayerRuntime& layer = m_layers[l];

    layer.blendSpaces.assign(graphLayer.states.size(), NOT_FOUND);
    for (SIZE_T s = 0; s < graphLayer.states.size(); ++s) {
      if (graphLayer.states[s].kind != AnimationStateKind::Clip) {
        layer.blendSpaces[s] = static_cast<uint32>(m_blendSpaces.size());
        m_blendSpaces.emplace_back().build(graphLayer.states[s]);
      }
    }

    const uint32 transitionCount = static_cast<uint32>(graphLayer.transitions.size());
    layer.transitionOrder.resize(transitionCount);
    for (uint32 t = 0; t < transitionCount; ++t) {
      layer.transitionOrder[t] = t;
    }
    Algorithm::sort(layer.transitionOrder, [&graphLayer](uint32 a, uint32 b) {
      const int32 priorityA = graphLayer.transitions[a].priority;
      const int32 priorityB = graphLayer.transitions[b].priority;
      return priorityA != priorityB ? priorityA > priorityB : a < b;
    });

    layer.nodeWeights.clear();
    if (l > 0 && !graphLayer.maskBones.empty()) {
      layer.nodeWeights.assign(nodeCount, 0.0f);
      // Parents come first, so a node under a mask bone finds its parent already marked.
      for (SIZE_T n = 0; n < nodeCount; ++n) {
        const uint32 parent = nodes[n].parent;
        bool bMasked = parent != ModelNode::NO_PARENT && layer.nodeWeights[parent] > 0.0f;
        for (SIZE_T b = 0; !bMasked && b < graphLayer.maskBones.size(); ++b) {
          bMasked = nodes[n].name == graphLayer.maskBones[b];
        }
        layer.nodeWeights[n] = bMasked ? 1.0f : 0.0f;
      }
    }

    layer.statePose = m_restPose;
    layer.fromPose = m_restPose;
    layer.pose = m_restPose;
  }

  reset();
}

/*
 */
void
AnimationGraphInstance::reset()
{
  if (!m_bBound) {
    return;
  }
  const Vector<AnimationParameter>& parameters = m_graph->getParameters();
  for (SIZE_T i = 0; i < parameters.size(); ++i) {
    m_values[i] = parameters[i].defaultValue;
  }

  const Vector<AnimationLayer>& graphLayers = m_graph->getLayers();
  for (uint32 l = 0; l < m_layers.size(); ++l) {
    LayerRuntime& layer = m_layers[l];
    layer.state = graphLayers[l].defaultState;
    layer.stateWeights = computeStateWeights(l, layer.state);
    layer.time = getStateSpeed(l, layer.state) < 0.0f ? 1.0f : 0.0f;
    layer.weight = graphLayers[l].weight;
    layer.bInTransition = false;
    layer.bFromFrozen = false;
  }
  computePose();
}

/*
 */
bool
AnimationGraphInstance::setFloat(uint32 parameter, float value) noexcept
{
  if (!m_bBound || parameter >= m_values.size()
      || m_graph->getParameters()[parameter].type != AnimationParameterType::Float) {
    return false;
  }
  m_values[parameter].floatValue = value;
  return true;
}

/*
 */
bool
AnimationGraphInstance::setInt(uint32 parameter, int32 value) noexcept
{
  if (!m_bBound || parameter >= m_values.size()
      || m_graph->getParameters()[parameter].type != AnimationParameterType::Int) {
    return false;
  }
  m_values[parameter].intValue = value;
  return true;
}

/*
 */
bool
AnimationGraphInstance::setBool(uint32 parameter, bool bValue) noexcept
{
  if (!m_bBound || parameter >= m_values.size()
      || m_graph->getParameters()[parameter].type != AnimationParameterType::Bool) {
    return false;
  }
  m_values[parameter].intValue = bValue ? 1 : 0;
  return true;
}

/*
 */
bool
AnimationGraphInstance::setTrigger(uint32 parameter) noexcept
{
  if (!m_bBound || parameter >= m_values.size()
      || m_graph->getParameters()[parameter].type != AnimationParameterType::Trigger) {
    return false;
  }
  m_values[parameter].intValue = 1;
  return true;
}

/*
 */
bool
AnimationGraphInstance::resetTrigger(uint32 parameter) noexcept
{
  if (!m_bBound || parameter >= m_values.size()
      || m_graph->getParameters()[parameter].type != AnimationParameterType::Trigger) {
    return false;
  }
  m_values[parameter].intValue = 0;
  return true;
}

/*
 */
float
AnimationGraphInstance::getFloat(uint32 parameter) const noexcept
{
  if (!m_bBound || parameter >= m_values.size()) {
    return 0.0f;
  }
  return getBlendValue(m_graph->getParameters()[parameter], m_values[parameter]);
}

/*
 */
int32
AnimationGraphInstance::getInt(uint32 parameter) const noexcept
{
  return m_bBound && parameter < m_values.size() ? m_values[parameter].intValue : 0;
}

/*
 */
bool
AnimationGraphInstance::getBool(uint32 parameter) const noexcept
{
  return m_bBound && parameter < m_values.size() && m_values[parameter].intValue != 0;
}

/*
 */
void
AnimationGraphInstance::setLayerWeight(uint32 layer, float weight) noexcept
{
  if (layer < m_layers.size()) {
    m_layers[layer].weight = Math::clamp(weight, 0.0f, 1.0f);
  }
}

/*
 */
float
AnimationGraphInstance::getLayerWeight(uint32 layer) const noexcept
{
  return layer < m_layers.size() ? m_layers[layer].weight : 0.0f;
}

/*
 */
uint32
AnimationGraphInstance::getCurrentState(uint32 layer) const noexcept
{
  return layer < m_layers.size() ? m_layers[layer].state : NOT_FOUND;
}

/*
 */
float
AnimationGraphInstance::getStateTime(uint32 layer) const noexcept
{
  return layer < m_layers.size() ? m_layers[layer].time : 0.0f;
}

/*
 */
bool
AnimationGraphInstance::isInTransition(uint32 layer) const noexcept
{
  return layer < m_layers.size() && m_layers[layer].bInTransition;
}

/*
 */
AnimationGraphInstance::StateWeights
AnimationGraphInstance::computeStateWeights(uint32 layer, uint32 state) const noexcept
{
  StateWeights result;
  const uint32 blendSpace = m_layers[layer].blendSpaces[state];
  if (blendSpace == NOT_FOUND) {
    return result;
  }
  const AnimationState& graphState = m_graph->getLayers()[layer].states[state];
  const Vector<AnimationParameter>& parameters = m_graph->getParameters();
  const float x =
      getBlendValue(parameters[graphState.parameterX], m_values[graphState.parameterX]);
  const float y =
      graphState.kind == AnimationStateKind::BlendSpace2D
          ? getBlendValue(parameters[graphState.parameterY], m_values[graphState.parameterY])
          : 0.0f;
  const BlendSpaceWeights weights = m_blendSpaces[blendSpace].computeWeights(x, y);
  for (uint32 i = 0; i < weights.count; ++i) {
    result.samples[i] = graphState.samples[weights.items[i].sample].clip;
    result.weights[i] = weights.items[i].weight;
  }
  result.count = weights.count;
  return result;
}

/*
 */
float
AnimationGraphInstance::getStateDuration(uint32 layer, uint32 state,
                                         const StateWeights& weights) const noexcept
{
  const AnimationState& graphState = m_graph->getLayers()[layer].states[state];
  if (graphState.kind == AnimationStateKind::Clip) {
    const SPtr<AnimationClip>& clip = m_clips[graphState.clip];
    return clip ? clip->getDuration() : 0.0f;
  }
  float duration = 0.0f;
  for (uint32 i = 0; i < weights.count; ++i) {
    const SPtr<AnimationClip>& clip = m_clips[weights.samples[i]];
    duration += clip ? clip->getDuration() * weights.weights[i] : 0.0f;
  }
  return duration;
}

/*
 */
float
AnimationGraphInstance::getStateSpeed(uint32 layer, uint32 state) const noexcept
{
  const AnimationState& graphState = m_graph->getLayers()[layer].states[state];
  if (graphState.speedParameter == AnimationGraph::NO_PARAMETER) {
    return graphState.speed;
  }
  return graphState.speed * m_values[graphState.speedParameter].floatValue;
}

/*
 */
float
AnimationGraphInstance::advanceStateTime(uint32 layer, uint32 state,
                                         const StateWeights& weights, float time,
                                         float deltaTime) const noexcept
{
  const float duration = Math::max(getStateDuration(layer, state, weights), kMinStateDuration);
  const float next = time + deltaTime * getStateSpeed(layer, state) / duration;
  if (m_graph->getLayers()[layer].states[state].bLooping) {
    return next;
  }
  return Math::clamp(next, 0.0f, 1.0f);
}

/*
 */
bool
AnimationGraphInstance::advance(float deltaTime)
{
  if (!m_bBound) {
    return false;
  }

  for (uint32 l = 0; l < m_layers.size(); ++l) {
    LayerRuntime& layer = m_layers[l];
    layer.stateWeights = computeStateWeights(l, layer.state);
    layer.time = advanceStateTime(l, layer.state, layer.stateWeights, layer.time, deltaTime);

    if (layer.bInTransition) {
      if (!layer.bFromFrozen) {
        layer.fromWeights = computeStateWeights(l, layer.fromState);
        layer.fromTime =
            advanceStateTime(l, layer.fromState, layer.fromWeights, layer.fromTime, deltaTime);
      }
      layer.fadeElapsed += deltaTime;
      if (layer.fadeElapsed >= layer.fadeDuration) {
        layer.bInTransition = false;
      }
    }

    evaluateTransitions(l);
  }

  computePose();
  return true;
}

/*
 */
void
AnimationGraphInstance::evaluateTransitions(uint32 layer) noexcept
{
  LayerRuntime& runtime = m_layers[layer];
  const AnimationLayer& graphLayer = m_graph->getLayers()[layer];
  if (runtime.bInTransition && !graphLayer.transitions[runtime.transition].bInterruptible) {
    return;
  }

  const AnimationState& current = graphLayer.states[runtime.state];
  for (const uint32 index : runtime.transitionOrder) {
    const AnimationTransition& transition = graphLayer.transitions[index];
    if (transition.source == AnimationGraph::ANY_STATE) {
      if (transition.target == runtime.state && !transition.bCanTransitionToSelf) {
        continue;
      }
    }
    else if (transition.source != runtime.state) {
      continue;
    }
    if (transition.bHasExitTime
        && !isExitTimeReached(current.bLooping, runtime.time, transition.exitTime)) {
      continue;
    }
    if (!conditionsHold(transition)) {
      continue;
    }
    startTransition(layer, index);
    return;
  }
}

/*
 */
bool
AnimationGraphInstance::conditionsHold(const AnimationTransition& transition) const noexcept
{
  const Vector<AnimationParameter>& parameters = m_graph->getParameters();
  for (const AnimationCondition& condition : transition.conditions) {
    if (!conditionHolds(parameters[condition.parameter], m_values[condition.parameter],
                        condition)) {
      return false;
    }
  }
  return true;
}

/*
 */
void
AnimationGraphInstance::startTransition(uint32 layer, uint32 transition) noexcept
{
  LayerRuntime& runtime = m_layers[layer];
  const AnimationTransition& graphTransition =
      m_graph->getLayers()[layer].transitions[transition];

  // A trigger fires one transition only.
  const Vector<AnimationParameter>& parameters = m_graph->getParameters();
  for (const AnimationCondition& condition : graphTransition.conditions) {
    if (parameters[condition.parameter].type == AnimationParameterType::Trigger) {
      m_values[condition.parameter].intValue = 0;
    }
  }

  if (graphTransition.duration > 0.0f) {
    if (runtime.bInTransition) {
      // Interrupted: the new fade starts from what was shown, so it never jumps.
      runtime.fromPose = runtime.pose;
      runtime.bFromFrozen = true;
    }
    else {
      runtime.fromState = runtime.state;
      runtime.fromTime = runtime.time;
      runtime.fromWeights = runtime.stateWeights;
      runtime.bFromFrozen = false;
    }
    runtime.bInTransition = true;
    runtime.transition = transition;
    runtime.fadeElapsed = 0.0f;
    runtime.fadeDuration = graphTransition.duration;
  }
  else {
    runtime.bInTransition = false;
  }

  runtime.state = graphTransition.target;
  runtime.stateWeights = computeStateWeights(layer, runtime.state);
  runtime.time = getStateSpeed(layer, runtime.state) < 0.0f ? 1.0f : 0.0f;
}

/*
 */
void
AnimationGraphInstance::sampleClip(uint32 clip, float phase,
                                   Span<NodeTransform> outPose) const noexcept
{
  const SPtr<AnimationClip>& animation = m_clips[clip];
  if (animation) {
    animation->sample(phase * animation->getDuration(), m_clipTrackNodes[clip], outPose);
  }
}

/*
 */
void
AnimationGraphInstance::sampleState(uint32 layer, uint32 state, float time,
                                    const StateWeights& weights,
                                    Vector<NodeTransform>& outPose) noexcept
{
  const AnimationState& graphState = m_graph->getLayers()[layer].states[state];
  const float phase = graphState.bLooping ? time - Math::floor(time) : time;

  // From the rest pose each time, so nodes the clips do not move stay at rest.
  outPose = m_restPose;
  if (graphState.kind == AnimationStateKind::Clip) {
    sampleClip(graphState.clip, phase, outPose);
    return;
  }
  if (weights.count == 0) {
    return;
  }

  // Every clip at the same phase keeps their steps together; each one is blended in by its
  // share of the weight added so far, which gives every clip its own weight at the end.
  sampleClip(weights.samples[0], phase, outPose);
  float accumulated = weights.weights[0];
  for (uint32 i = 1; i < weights.count; ++i) {
    accumulated += weights.weights[i];
    if (weights.weights[i] <= 0.0f || accumulated <= 0.0f) {
      continue;
    }
    m_scratchPose = m_restPose;
    sampleClip(weights.samples[i], phase, m_scratchPose);
    AnimationPlayer::blendPoses(outPose, m_scratchPose, weights.weights[i] / accumulated, {},
                                outPose);
  }
}

/*
 */
void
AnimationGraphInstance::computePose() noexcept
{
  for (uint32 l = 0; l < m_layers.size(); ++l) {
    LayerRuntime& layer = m_layers[l];
    const float weight = l == 0 ? 1.0f : layer.weight;
    if (weight <= 0.0f) {
      continue;
    }

    sampleState(l, layer.state, layer.time, layer.stateWeights, layer.statePose);
    if (layer.bInTransition) {
      if (!layer.bFromFrozen) {
        sampleState(l, layer.fromState, layer.fromTime, layer.fromWeights, layer.fromPose);
      }
      AnimationPlayer::blendPoses(
          layer.fromPose, layer.statePose,
          AnimationPlayer::getFadeWeight(layer.fadeElapsed, layer.fadeDuration), {},
          layer.pose);
    }
    else {
      layer.pose = layer.statePose;
    }

    if (l == 0) {
      m_localPose = layer.pose;
    }
    else {
      AnimationPlayer::blendPoses(m_localPose, layer.pose, weight, layer.nodeWeights,
                                  m_localPose);
    }
  }
  AnimationPlayer::buildPoseMatrices(*m_model, m_localPose, m_poseMatrices);
}

} // namespace chEngineSDK
