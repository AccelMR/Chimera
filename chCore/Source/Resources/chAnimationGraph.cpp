/************************************************************************/
/**
 * @file chAnimationGraph.cpp
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Parameters, layers, states and transitions that decide how a model animates.
 */
/************************************************************************/
#include "chAnimationGraph.h"

#include "chAssetFile.h"
#include "chFileStream.h"
#include "chMath.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
// Above any real graph; only a broken file asks for more.
constexpr uint32 kMaxParameters = 1024;
constexpr uint32 kMaxClips = 4096;
constexpr uint32 kMaxLayers = 64;
constexpr uint32 kMaxStates = 4096;
constexpr uint32 kMaxTransitions = 16384;
constexpr uint32 kMaxConditions = 64;
constexpr uint32 kMaxSamples = 256;
constexpr uint32 kMaxMaskBones = 1024;
constexpr uint32 kMaxNameLength = 1024;

struct GraphFileData
{
  uint32 parameterCount;
  uint32 clipCount;
  uint32 layerCount;
};
static_assert(sizeof(GraphFileData) == 12, "GraphFileData must have no padding");

struct ParameterFileData
{
  uint32 type;
  float floatValue;
  int32 intValue;
};
static_assert(sizeof(ParameterFileData) == 12, "ParameterFileData must have no padding");

struct LayerFileData
{
  float weight;
  uint32 defaultState;
  uint32 stateCount;
  uint32 transitionCount;
  uint32 maskBoneCount;
};
static_assert(sizeof(LayerFileData) == 20, "LayerFileData must have no padding");

struct StateFileData
{
  uint32 kind;
  uint32 clip;
  uint32 parameterX;
  uint32 parameterY;
  float speed;
  uint32 speedParameter;
  uint32 looping;
  uint32 sampleCount;
};
static_assert(sizeof(StateFileData) == 32, "StateFileData must have no padding");

struct SampleFileData
{
  uint32 clip;
  float x;
  float y;
};
static_assert(sizeof(SampleFileData) == 12, "SampleFileData must have no padding");

struct TransitionFileData
{
  uint32 source;
  uint32 target;
  float duration;
  uint32 hasExitTime;
  float exitTime;
  uint32 interruptible;
  uint32 canTransitionToSelf;
  int32 priority;
  uint32 conditionCount;
};
static_assert(sizeof(TransitionFileData) == 36, "TransitionFileData must have no padding");

struct ConditionFileData
{
  uint32 parameter;
  uint32 op;
  float floatValue;
  int32 intValue;
};
static_assert(sizeof(ConditionFileData) == 16, "ConditionFileData must have no padding");

/*
 * Writes the error when the caller wants it; always returns false so checks can return it.
 */
template<typename... Args>
bool
fail(String* outError, FormatString<std::type_identity_t<Args>...> format, Args&&... args)
{
  if (!outError) {
    return false;
  }
  if constexpr (sizeof...(Args) == 0) {
    *outError = format.get();
  }
  else {
    *outError = StringUtils::format(format, std::forward<Args>(args)...);
  }
  return false;
}

NODISCARD bool
isValidName(StringView name)
{
  return !name.empty() && name.size() <= kMaxNameLength;
}

/*
 * Index of the first name equal to an earlier one (ignoring case), or NOT_FOUND. Lists are
 * short and this runs once per load, so the quadratic walk is fine.
 */
template<typename T, typename GetName>
NODISCARD uint32
findDuplicateName(const Vector<T>& items, GetName&& getName)
{
  for (SIZE_T i = 1; i < items.size(); ++i) {
    for (SIZE_T j = 0; j < i; ++j) {
      if (StringUtils::equalsIgnoreCase(getName(items[i]), getName(items[j]))) {
        return static_cast<uint32>(i);
      }
    }
  }
  return AnimationGraph::NOT_FOUND;
}

NODISCARD bool
isBlendParameter(AnimationParameterType type)
{
  return type == AnimationParameterType::Float || type == AnimationParameterType::Int;
}

NODISCARD bool
isValidOp(AnimationParameterType type, AnimationConditionOp op)
{
  switch (type) {
  case AnimationParameterType::Trigger:
    return op == AnimationConditionOp::IsSet;
  case AnimationParameterType::Bool:
    return op == AnimationConditionOp::IsSet || op == AnimationConditionOp::IsNotSet;
  case AnimationParameterType::Float:
    return op >= AnimationConditionOp::Greater && op <= AnimationConditionOp::LessOrEqual;
  case AnimationParameterType::Int:
    return op >= AnimationConditionOp::Greater && op <= AnimationConditionOp::NotEqual;
  }
  return false;
}

NODISCARD bool
validateState(const AnimationGraph& graph, const AnimationLayer& layer,
              const AnimationState& state, String* outError)
{
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  const uint32 clipCount = static_cast<uint32>(graph.getClips().size());
  const uint32 parameterCount = static_cast<uint32>(parameters.size());

  if (!isValidName(state.name)) {
    return fail(outError, "Layer '{0}' has a state without a valid name", layer.name);
  }
  if (!Math::isFinite(state.speed)) {
    return fail(outError, "State '{0}' has a speed that is not a number", state.name);
  }
  if (state.speedParameter != AnimationGraph::NO_PARAMETER
      && (state.speedParameter >= parameterCount
          || parameters[state.speedParameter].type != AnimationParameterType::Float)) {
    return fail(outError, "State '{0}' has a speed parameter that is not a Float parameter",
                state.name);
  }

  switch (state.kind) {
  case AnimationStateKind::Clip:
    if (state.clip >= clipCount) {
      return fail(outError, "State '{0}' plays a clip that does not exist", state.name);
    }
    return true;
  case AnimationStateKind::BlendSpace1D:
  case AnimationStateKind::BlendSpace2D:
    break;
  default:
    return fail(outError, "State '{0}' has an unknown kind {1}", state.name,
                static_cast<uint32>(state.kind));
  }

  const bool b2D = state.kind == AnimationStateKind::BlendSpace2D;
  if (state.parameterX >= parameterCount
      || !isBlendParameter(parameters[state.parameterX].type)
      || (b2D
          && (state.parameterY >= parameterCount
              || !isBlendParameter(parameters[state.parameterY].type)
              || state.parameterY == state.parameterX))) {
    return fail(outError,
                "Blend space '{0}' needs Float or Int parameters (two different ones in 2D)",
                state.name);
  }
  if (state.samples.empty()) {
    return fail(outError, "Blend space '{0}' has no clips", state.name);
  }
  for (SIZE_T i = 0; i < state.samples.size(); ++i) {
    const BlendSpaceSample& sample = state.samples[i];
    if (sample.clip >= clipCount) {
      return fail(outError, "Blend space '{0}' uses a clip that does not exist", state.name);
    }
    if (!Math::isFinite(sample.x) || (b2D && !Math::isFinite(sample.y))) {
      return fail(outError, "Blend space '{0}' has a position that is not a number",
                  state.name);
    }
    for (SIZE_T j = 0; j < i; ++j) {
      if (state.samples[j].x == sample.x && (!b2D || state.samples[j].y == sample.y)) {
        return fail(outError, "Blend space '{0}' has two clips at the same position",
                    state.name);
      }
    }
  }
  return true;
}

NODISCARD bool
validateTransition(const AnimationGraph& graph, const AnimationLayer& layer,
                   const AnimationTransition& transition, String* outError)
{
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  const uint32 stateCount = static_cast<uint32>(layer.states.size());
  const bool bFromAny = transition.source == AnimationGraph::ANY_STATE;

  if ((!bFromAny && transition.source >= stateCount) || transition.target >= stateCount) {
    return fail(outError, "Layer '{0}' has a transition between states that do not exist",
                layer.name);
  }
  const String& targetName = layer.states[transition.target].name;
  if (transition.source == transition.target) {
    return fail(outError, "Layer '{0}' has a transition from '{1}' to itself", layer.name,
                targetName);
  }
  if (!Math::isFinite(transition.duration) || transition.duration < 0.0f
      || !Math::isFinite(transition.exitTime) || transition.exitTime < 0.0f) {
    return fail(outError, "A transition to '{0}' has a negative or invalid time", targetName);
  }
  if (bFromAny && transition.bHasExitTime) {
    return fail(outError, "The Any State transition to '{0}' cannot have an exit time",
                targetName);
  }
  // Without conditions it would start at once, every frame.
  if (transition.conditions.empty() && (bFromAny || !transition.bHasExitTime)) {
    return fail(outError, "A transition to '{0}' needs a condition or an exit time",
                targetName);
  }
  for (const AnimationCondition& condition : transition.conditions) {
    if (condition.parameter >= parameters.size()) {
      return fail(outError, "A transition to '{0}' tests a parameter that does not exist",
                  targetName);
    }
    const AnimationParameter& parameter = parameters[condition.parameter];
    if (!isValidOp(parameter.type, condition.op)) {
      return fail(outError,
                  "A transition to '{0}' tests {1} parameter '{2}' in a way it "
                  "does not allow",
                  targetName, AnimationGraph::getParameterTypeName(parameter.type),
                  parameter.name);
    }
    if (!Math::isFinite(condition.threshold.floatValue)) {
      return fail(outError,
                  "A transition to '{0}' compares with a value that is not a "
                  "number",
                  targetName);
    }
  }
  return true;
}

NODISCARD bool
validateLayer(const AnimationGraph& graph, const AnimationLayer& layer, bool bFirst,
              String* outError)
{
  if (!isValidName(layer.name)) {
    return fail(outError, "A layer has no valid name");
  }
  if (!Math::isFinite(layer.weight) || layer.weight < 0.0f || layer.weight > 1.0f) {
    return fail(outError, "Layer '{0}' has a weight outside 0 to 1", layer.name);
  }
  if (layer.states.empty() || layer.defaultState >= layer.states.size()) {
    return fail(outError, "Layer '{0}' needs states and a default state that exists",
                layer.name);
  }
  if (bFirst && !layer.maskBones.empty()) {
    return fail(outError,
                "The first layer '{0}' moves every node, so it cannot have a "
                "mask",
                layer.name);
  }
  for (const String& bone : layer.maskBones) {
    if (!isValidName(bone)) {
      return fail(outError, "Layer '{0}' has a mask bone without a valid name", layer.name);
    }
  }
  const uint32 duplicateState = findDuplicateName(
      layer.states, [](const AnimationState& s) -> StringView { return s.name; });
  if (duplicateState != AnimationGraph::NOT_FOUND) {
    return fail(outError, "Layer '{0}' has two states named '{1}'", layer.name,
                layer.states[duplicateState].name);
  }
  for (const AnimationState& state : layer.states) {
    if (!validateState(graph, layer, state, outError)) {
      return false;
    }
  }
  for (const AnimationTransition& transition : layer.transitions) {
    if (!validateTransition(graph, layer, transition, outError)) {
      return false;
    }
  }
  return true;
}

/*
 * True when count fits its limit and count items of at least itemSize bytes are still in
 * the stream, so nothing big is allocated for a broken count.
 */
NODISCARD bool
canRead(const DataStream& stream, uint32 count, uint32 maxCount, SIZE_T itemSize)
{
  return count <= maxCount
         && static_cast<uint64>(count) * itemSize <= AssetFile::getBytesLeft(stream);
}

NODISCARD bool
readName(DataStream& stream, String& outName)
{
  return AssetFile::readString(stream, outName, kMaxNameLength);
}

NODISCARD bool
readState(DataStream& stream, AnimationState& outState)
{
  StateFileData data;
  if (!readName(stream, outState.name) || !AssetFile::readValue(stream, data)
      || !canRead(stream, data.sampleCount, kMaxSamples, sizeof(SampleFileData))) {
    return false;
  }
  outState.kind = static_cast<AnimationStateKind>(data.kind);
  outState.clip = data.clip;
  outState.parameterX = data.parameterX;
  outState.parameterY = data.parameterY;
  outState.speed = data.speed;
  outState.speedParameter = data.speedParameter;
  outState.bLooping = data.looping != 0;
  outState.samples.resize(data.sampleCount);
  for (BlendSpaceSample& sample : outState.samples) {
    SampleFileData sampleData;
    if (!AssetFile::readValue(stream, sampleData)) {
      return false;
    }
    sample = {.clip = sampleData.clip, .x = sampleData.x, .y = sampleData.y};
  }
  return true;
}

NODISCARD bool
readTransition(DataStream& stream, AnimationTransition& outTransition)
{
  TransitionFileData data;
  if (!AssetFile::readValue(stream, data)
      || !canRead(stream, data.conditionCount, kMaxConditions, sizeof(ConditionFileData))) {
    return false;
  }
  outTransition.source = data.source;
  outTransition.target = data.target;
  outTransition.duration = data.duration;
  outTransition.bHasExitTime = data.hasExitTime != 0;
  outTransition.exitTime = data.exitTime;
  outTransition.bInterruptible = data.interruptible != 0;
  outTransition.bCanTransitionToSelf = data.canTransitionToSelf != 0;
  outTransition.priority = data.priority;
  outTransition.conditions.resize(data.conditionCount);
  for (AnimationCondition& condition : outTransition.conditions) {
    ConditionFileData conditionData;
    if (!AssetFile::readValue(stream, conditionData)) {
      return false;
    }
    condition.parameter = conditionData.parameter;
    condition.op = static_cast<AnimationConditionOp>(conditionData.op);
    condition.threshold = {.floatValue = conditionData.floatValue,
                           .intValue = conditionData.intValue};
  }
  return true;
}

NODISCARD bool
readLayer(DataStream& stream, AnimationLayer& outLayer)
{
  LayerFileData data;
  if (!readName(stream, outLayer.name) || !AssetFile::readValue(stream, data)
      || !canRead(stream, data.maskBoneCount, kMaxMaskBones, sizeof(uint32))
      || !canRead(stream, data.stateCount, kMaxStates, sizeof(StateFileData))
      || !canRead(stream, data.transitionCount, kMaxTransitions, sizeof(TransitionFileData))) {
    return false;
  }
  outLayer.weight = data.weight;
  outLayer.defaultState = data.defaultState;
  outLayer.maskBones.resize(data.maskBoneCount);
  for (String& bone : outLayer.maskBones) {
    if (!readName(stream, bone)) {
      return false;
    }
  }
  outLayer.states.resize(data.stateCount);
  for (AnimationState& state : outLayer.states) {
    if (!readState(stream, state)) {
      return false;
    }
  }
  outLayer.transitions.resize(data.transitionCount);
  for (AnimationTransition& transition : outLayer.transitions) {
    if (!readTransition(stream, transition)) {
      return false;
    }
  }
  return true;
}
} // namespace

/*
 */
uint32
AnimationGraph::addParameter(AnimationParameter parameter)
{
  m_parameters.push_back(std::move(parameter));
  return static_cast<uint32>(m_parameters.size() - 1);
}

/*
 */
uint32
AnimationGraph::addClip(const UUID& clipId, const SPtr<AnimationClipAsset>& clip)
{
  m_clips.push_back({.clipId = clipId, .clip = clip});
  return static_cast<uint32>(m_clips.size() - 1);
}

/*
 */
uint32
AnimationGraph::addLayer(AnimationLayer layer)
{
  m_layers.push_back(std::move(layer));
  return static_cast<uint32>(m_layers.size() - 1);
}

/*
 */
void
AnimationGraph::setClipAsset(uint32 index, const SPtr<AnimationClipAsset>& clip)
{
  if (index < m_clips.size()) {
    m_clips[index].clip = clip;
  }
}

/*
 */
uint32
AnimationGraph::findParameter(StringView name) const
{
  for (SIZE_T i = 0; i < m_parameters.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(m_parameters[i].name, name)) {
      return static_cast<uint32>(i);
    }
  }
  return NOT_FOUND;
}

/*
 */
uint32
AnimationGraph::findLayer(StringView name) const
{
  for (SIZE_T i = 0; i < m_layers.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(m_layers[i].name, name)) {
      return static_cast<uint32>(i);
    }
  }
  return NOT_FOUND;
}

/*
 */
uint32
AnimationGraph::findState(uint32 layer, StringView name) const
{
  if (layer >= m_layers.size()) {
    return NOT_FOUND;
  }
  const Vector<AnimationState>& states = m_layers[layer].states;
  for (SIZE_T i = 0; i < states.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(states[i].name, name)) {
      return static_cast<uint32>(i);
    }
  }
  return NOT_FOUND;
}

/*
 */
bool
AnimationGraph::validate(String* outError) const
{
  for (const AnimationParameter& parameter : m_parameters) {
    if (!isValidName(parameter.name)) {
      return fail(outError, "A parameter has no valid name");
    }
    switch (parameter.type) {
    case AnimationParameterType::Float:
      if (!Math::isFinite(parameter.defaultValue.floatValue)) {
        return fail(outError, "Parameter '{0}' has a default that is not a number",
                    parameter.name);
      }
      break;
    case AnimationParameterType::Bool:
      if (parameter.defaultValue.intValue != 0 && parameter.defaultValue.intValue != 1) {
        return fail(outError, "Bool parameter '{0}' has a default that is not 0 or 1",
                    parameter.name);
      }
      break;
    case AnimationParameterType::Int:
      break;
    case AnimationParameterType::Trigger:
      // A trigger set from the start would fire before any script asked for it.
      if (parameter.defaultValue.intValue != 0) {
        return fail(outError, "Trigger parameter '{0}' must start off", parameter.name);
      }
      break;
    default:
      return fail(outError, "Parameter '{0}' has an unknown type {1}", parameter.name,
                  static_cast<uint32>(parameter.type));
    }
  }
  const uint32 duplicateParameter = findDuplicateName(
      m_parameters, [](const AnimationParameter& p) -> StringView { return p.name; });
  if (duplicateParameter != NOT_FOUND) {
    return fail(outError, "Two parameters are named '{0}'",
                m_parameters[duplicateParameter].name);
  }

  if (m_layers.empty()) {
    return fail(outError, "The graph has no layers");
  }
  const uint32 duplicateLayer = findDuplicateName(
      m_layers, [](const AnimationLayer& l) -> StringView { return l.name; });
  if (duplicateLayer != NOT_FOUND) {
    return fail(outError, "Two layers are named '{0}'", m_layers[duplicateLayer].name);
  }
  for (SIZE_T i = 0; i < m_layers.size(); ++i) {
    if (!validateLayer(*this, m_layers[i], i == 0, outError)) {
      return false;
    }
  }
  return true;
}

/*
 */
void
AnimationGraph::collectReferences(Vector<UUID>& outReferences) const
{
  for (const AnimationGraphClip& clip : m_clips) {
    if (!clip.clipId.isNull()) {
      outReferences.push_back(clip.clipId);
    }
  }
}

/*
 */
void
AnimationGraph::serialize(DataStream& stream) const
{
  stream << SERIALIZATION_VERSION;
  stream << GraphFileData{.parameterCount = static_cast<uint32>(m_parameters.size()),
                          .clipCount = static_cast<uint32>(m_clips.size()),
                          .layerCount = static_cast<uint32>(m_layers.size())};

  for (const AnimationParameter& parameter : m_parameters) {
    AssetFile::writeString(stream, parameter.name);
    stream << ParameterFileData{.type = static_cast<uint32>(parameter.type),
                                .floatValue = parameter.defaultValue.floatValue,
                                .intValue = parameter.defaultValue.intValue};
  }
  for (const AnimationGraphClip& clip : m_clips) {
    stream << clip.clipId;
  }

  for (const AnimationLayer& layer : m_layers) {
    AssetFile::writeString(stream, layer.name);
    stream << LayerFileData{.weight = layer.weight,
                            .defaultState = layer.defaultState,
                            .stateCount = static_cast<uint32>(layer.states.size()),
                            .transitionCount = static_cast<uint32>(layer.transitions.size()),
                            .maskBoneCount = static_cast<uint32>(layer.maskBones.size())};
    for (const String& bone : layer.maskBones) {
      AssetFile::writeString(stream, bone);
    }
    for (const AnimationState& state : layer.states) {
      AssetFile::writeString(stream, state.name);
      stream << StateFileData{.kind = static_cast<uint32>(state.kind),
                              .clip = state.clip,
                              .parameterX = state.parameterX,
                              .parameterY = state.parameterY,
                              .speed = state.speed,
                              .speedParameter = state.speedParameter,
                              .looping = state.bLooping ? 1u : 0u,
                              .sampleCount = static_cast<uint32>(state.samples.size())};
      for (const BlendSpaceSample& sample : state.samples) {
        stream << SampleFileData{.clip = sample.clip, .x = sample.x, .y = sample.y};
      }
    }
    for (const AnimationTransition& transition : layer.transitions) {
      stream << TransitionFileData{
          .source = transition.source,
          .target = transition.target,
          .duration = transition.duration,
          .hasExitTime = transition.bHasExitTime ? 1u : 0u,
          .exitTime = transition.exitTime,
          .interruptible = transition.bInterruptible ? 1u : 0u,
          .canTransitionToSelf = transition.bCanTransitionToSelf ? 1u : 0u,
          .priority = transition.priority,
          .conditionCount = static_cast<uint32>(transition.conditions.size())};
      for (const AnimationCondition& condition : transition.conditions) {
        stream << ConditionFileData{.parameter = condition.parameter,
                                    .op = static_cast<uint32>(condition.op),
                                    .floatValue = condition.threshold.floatValue,
                                    .intValue = condition.threshold.intValue};
      }
    }
  }
}

/*
 */
SPtr<AnimationGraph>
AnimationGraph::deserialize(DataStream& stream, String* outError, bool bValidate)
{
  uint32 version = 0;
  GraphFileData data;
  if (!AssetFile::readValue(stream, version) || version != SERIALIZATION_VERSION
      || !AssetFile::readValue(stream, data)
      || !canRead(stream, data.parameterCount, kMaxParameters, sizeof(ParameterFileData))
      || !canRead(stream, data.clipCount, kMaxClips, sizeof(UUID))
      || !canRead(stream, data.layerCount, kMaxLayers, sizeof(LayerFileData))) {
    return nullptr;
  }

  SPtr<AnimationGraph> graph = chMakeShared<AnimationGraph>();
  graph->m_parameters.resize(data.parameterCount);
  for (AnimationParameter& parameter : graph->m_parameters) {
    ParameterFileData parameterData;
    if (!readName(stream, parameter.name) || !AssetFile::readValue(stream, parameterData)) {
      return nullptr;
    }
    parameter.type = static_cast<AnimationParameterType>(parameterData.type);
    parameter.defaultValue = {.floatValue = parameterData.floatValue,
                              .intValue = parameterData.intValue};
  }
  graph->m_clips.resize(data.clipCount);
  for (AnimationGraphClip& clip : graph->m_clips) {
    if (!AssetFile::readValue(stream, clip.clipId)) {
      return nullptr;
    }
  }
  graph->m_layers.resize(data.layerCount);
  for (AnimationLayer& layer : graph->m_layers) {
    if (!readLayer(stream, layer)) {
      return nullptr;
    }
  }
  return !bValidate || graph->validate(outError) ? graph : nullptr;
}

/*
 */
const ANSICHAR*
AnimationGraph::getParameterTypeName(AnimationParameterType type) noexcept
{
  switch (type) {
  case AnimationParameterType::Float:
    return "Float";
  case AnimationParameterType::Bool:
    return "Bool";
  case AnimationParameterType::Int:
    return "Int";
  case AnimationParameterType::Trigger:
    return "Trigger";
  }
  return "Unknown";
}

} // namespace chEngineSDK
