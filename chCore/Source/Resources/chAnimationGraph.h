/************************************************************************/
/**
 * @file chAnimationGraph.h
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Parameters, layers, states and transitions that decide how a model animates.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {
class AnimationClipAsset;
class DataStream;

/**
 * Values are stored in files, so they never change; new ones go at the end.
 */
enum class AnimationParameterType : uint32 {
  Float = 0,
  Bool = 1,
  Int = 2,
  // A bool that turns off on its own when a transition uses it.
  Trigger = 3,
};

/**
 * Values are stored in files, so they never change; new ones go at the end.
 */
enum class AnimationConditionOp : uint32 {
  // Bool and Trigger.
  IsSet = 0,
  IsNotSet = 1,
  // Float and Int.
  Greater = 2,
  Less = 3,
  GreaterOrEqual = 4,
  LessOrEqual = 5,
  // Int only: floats are almost never exactly equal.
  Equal = 6,
  NotEqual = 7,
};

/**
 * Values are stored in files, so they never change; new ones go at the end (a state that
 * holds another state machine would be the next one).
 */
enum class AnimationStateKind : uint32 {
  Clip = 0,
  BlendSpace1D = 1,
  BlendSpace2D = 2,
};

/**
 * The value of a parameter: floatValue for Float, intValue for the others (Bool and Trigger
 * are 0 or 1).
 */
struct AnimationParameterValue {
  float floatValue = 0.0f;
  int32 intValue = 0;
};

struct AnimationParameter {
  String name{};
  AnimationParameterType type = AnimationParameterType::Float;
  AnimationParameterValue defaultValue{};
};

struct AnimationCondition {
  uint32 parameter = 0;
  AnimationConditionOp op = AnimationConditionOp::IsSet;
  // Unused by IsSet and IsNotSet.
  AnimationParameterValue threshold{};
};

/**
 * A clip placed in a blend space; y is unused in 1D.
 */
struct BlendSpaceSample {
  uint32 clip = 0;
  float x = 0.0f;
  float y = 0.0f;
};

struct AnimationState {
  String name{};
  AnimationStateKind kind = AnimationStateKind::Clip;
  // Index into the graph's clips; Clip states only.
  uint32 clip = 0;
  // Blend spaces only: the parameters that place the blend (y in 2D only), Float or Int.
  uint32 parameterX = 0;
  uint32 parameterY = 0;
  Vector<BlendSpaceSample> samples{};
  float speed = 1.0f;
  // A Float parameter the speed is multiplied by, or NO_PARAMETER.
  uint32 speedParameter = 0xFFFFFFFFu;
  bool bLooping = true;
};

struct AnimationTransition {
  // A state of the same layer, or ANY_STATE.
  uint32 source = 0;
  uint32 target = 0;
  // Seconds the fade from the source pose to the target lasts.
  float duration = 0.2f;
  // When set, the transition waits until the source reaches exitTime, a fraction of its
  // length (in every loop of a looping state). Never on Any State transitions.
  bool bHasExitTime = false;
  float exitTime = 1.0f;
  // While this transition fades, transitions out of its target (and Any State ones) may
  // start; when off it always finishes first.
  bool bInterruptible = false;
  // Any State only: whether it may start while the target is already the current state.
  bool bCanTransitionToSelf = false;
  // Higher goes first; ties keep the order of the list.
  int32 priority = 0;
  // All must hold.
  Vector<AnimationCondition> conditions{};
};

struct AnimationLayer {
  String name{};
  // From 0 to 1; the first layer always counts as 1.
  float weight = 1.0f;
  uint32 defaultState = 0;
  Vector<AnimationState> states{};
  Vector<AnimationTransition> transitions{};
  // Bones (node names) the layer moves, each with everything under it. Empty moves every
  // node; the first layer must leave it empty.
  Vector<String> maskBones{};
};

/**
 * A clip of the graph. The UUID is kept even when the asset is missing, so saving the
 * graph again does not lose it.
 */
struct AnimationGraphClip {
  UUID clipId = UUID::null();
  SPtr<AnimationClipAsset> clip;
};

/**
 * Exists to describe, as data, how a model picks and blends its clips: scripts only set
 * parameters, and the graph's states and transitions decide the pose. Every layer is a
 * state machine; the first one moves the whole model and the others replace the nodes of
 * their mask, blended by their weight. States, clips and parameters refer to each other by
 * index, and nodes are named, so a graph works on any model whose nodes have those names,
 * like a clip. Built once (by a loader or an editor) and checked with validate.
 */
class CH_CORE_EXPORT AnimationGraph
{
 public:
  /**
   * Each returns the index of what it added.
   */
  uint32
  addParameter(AnimationParameter parameter);

  uint32
  addClip(const UUID& clipId, const SPtr<AnimationClipAsset>& clip = nullptr);

  uint32
  addLayer(AnimationLayer layer);

  /**
   * Gives a clip its loaded asset, or null when it is missing.
   */
  void
  setClipAsset(uint32 index, const SPtr<AnimationClipAsset>& clip);

  NODISCARD FORCEINLINE const Vector<AnimationParameter>&
  getParameters() const noexcept
  {
    return m_parameters;
  }

  NODISCARD FORCEINLINE const Vector<AnimationGraphClip>&
  getClips() const noexcept
  {
    return m_clips;
  }

  NODISCARD FORCEINLINE const Vector<AnimationLayer>&
  getLayers() const noexcept
  {
    return m_layers;
  }

  /**
   * Indices by name, or NOT_FOUND. They walk the lists, so they are meant for load time and
   * scripts that look names up once.
   */
  NODISCARD uint32
  findParameter(StringView name) const;

  NODISCARD uint32
  findLayer(StringView name) const;

  NODISCARD uint32
  findState(uint32 layer, StringView name) const;

  /**
   * True when every index points at something that exists, names are unique (ignoring
   * case) and every value fits its use; otherwise outError (when given) says what is wrong.
   */
  NODISCARD bool
  validate(String* outError = nullptr) const;

  void
  collectReferences(Vector<UUID>& outReferences) const;

  void
  serialize(DataStream& stream) const;

  /**
   * Null when the data is broken or does not pass validate (outError then says why when
   * validate failed).
   */
  NODISCARD static SPtr<AnimationGraph>
  deserialize(DataStream& stream, String* outError = nullptr);

  NODISCARD static const ANSICHAR*
  getParameterTypeName(AnimationParameterType type) noexcept;

  static constexpr uint32 NOT_FOUND = 0xFFFFFFFFu;
  static constexpr uint32 NO_PARAMETER = 0xFFFFFFFFu;
  static constexpr uint32 ANY_STATE = 0xFFFFFFFFu;
  static constexpr uint32 SERIALIZATION_VERSION = 1;

 private:
  Vector<AnimationParameter> m_parameters;
  Vector<AnimationGraphClip> m_clips;
  Vector<AnimationLayer> m_layers;
};

} // namespace chEngineSDK
