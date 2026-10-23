/************************************************************************/
/**
 * @file chAnimationGraphInstance.h
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Runs an AnimationGraph on the nodes of one model.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAnimationClip.h"
#include "chAnimationGraph.h"
#include "chMatrix4.h"

namespace chEngineSDK {
class BlendSpace;
class Model;

/**
 * Exists to turn a graph, its parameter values and the time into the pose of one model. It
 * keeps what changes per object: the parameter values, the state and time of every layer
 * and the fade of each running transition; the graph itself stays shared and read only.
 * Each advance moves every layer's time, starts at most one transition per layer and
 * builds the pose: each layer samples its state (and, during a fade, the state it leaves,
 * or the pose shown when a fade was interrupted), and every layer after the first replaces
 * the nodes of its mask, blended by its weight. Clips are bound to nodes by name and blend
 * spaces prepared once per model or graph change, and every buffer is sized then, so
 * advancing never allocates.
 */
class CH_CORE_EXPORT AnimationGraphInstance
{
 public:
  AnimationGraphInstance();
  ~AnimationGraphInstance();

  AnimationGraphInstance(const AnimationGraphInstance&) = delete;
  AnimationGraphInstance&
  operator=(const AnimationGraphInstance&) = delete;

  /**
   * Either change binds the graph to the model again and calls reset.
   */
  void
  setModel(const SPtr<Model>& model);

  void
  setGraph(const SPtr<AnimationGraph>& graph);

  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const noexcept
  {
    return m_model;
  }

  NODISCARD FORCEINLINE const SPtr<AnimationGraph>&
  getGraph() const noexcept
  {
    return m_graph;
  }

  /**
   * True when both a model and a graph are set, so there is a pose.
   */
  NODISCARD FORCEINLINE bool
  isBound() const noexcept
  {
    return m_bBound;
  }

  /**
   * Parameters back to their defaults, every layer to its default state at its start and
   * its weight from the graph; the pose follows at once.
   */
  void
  reset();

  /**
   * Parameters by the index AnimationGraph::findParameter gives. Setters return false and
   * change nothing when the index is wrong or the parameter has another type. A trigger
   * stays set until a transition uses it or resetTrigger clears it.
   */
  bool
  setFloat(uint32 parameter, float value) noexcept;

  bool
  setInt(uint32 parameter, int32 value) noexcept;

  bool
  setBool(uint32 parameter, bool bValue) noexcept;

  bool
  setTrigger(uint32 parameter) noexcept;

  bool
  resetTrigger(uint32 parameter) noexcept;

  /**
   * Zero or false for a wrong index. getFloat also reads Int parameters, getBool also
   * triggers.
   */
  NODISCARD float
  getFloat(uint32 parameter) const noexcept;

  NODISCARD int32
  getInt(uint32 parameter) const noexcept;

  NODISCARD bool
  getBool(uint32 parameter) const noexcept;

  /**
   * Clamped to 0 to 1. The first layer always counts as 1.
   */
  void
  setLayerWeight(uint32 layer, float weight) noexcept;

  NODISCARD float
  getLayerWeight(uint32 layer) const noexcept;

  /**
   * The state the layer is in; during a fade, the one it goes to. NOT_FOUND for a wrong
   * layer.
   */
  NODISCARD uint32
  getCurrentState(uint32 layer) const noexcept;

  /**
   * Time of the current state as a fraction of its length: 0 at the start, 1 at the end,
   * growing past 1 while a looping state repeats.
   */
  NODISCARD float
  getStateTime(uint32 layer) const noexcept;

  NODISCARD bool
  isInTransition(uint32 layer) const noexcept;

  /**
   * Moves every layer by deltaTime seconds, starts the transitions whose time has come and
   * builds the pose. False when nothing is bound.
   */
  bool
  advance(float deltaTime);

  /**
   * One per node of the model, relative to its parent and then in the model's space; empty
   * while nothing is bound. Valid until the next setModel or setGraph.
   */
  NODISCARD FORCEINLINE Span<const NodeTransform>
  getLocalPose() const noexcept
  {
    return m_bBound ? Span<const NodeTransform>(m_localPose) : Span<const NodeTransform>();
  }

  NODISCARD FORCEINLINE Span<const Matrix4>
  getPoseMatrices() const noexcept
  {
    return m_bBound ? Span<const Matrix4>(m_poseMatrices) : Span<const Matrix4>();
  }

  static constexpr uint32 NOT_FOUND = 0xFFFFFFFFu;

 private:
  /**
   * The weights of a blend space state, at most three clips; empty for a clip state.
   */
  struct StateWeights
  {
    uint32 samples[3] = {};
    float weights[3] = {};
    uint32 count = 0;
  };

  struct LayerRuntime
  {
    uint32 state = 0;
    float time = 0.0f;
    StateWeights stateWeights;
    float weight = 1.0f;

    bool bInTransition = false;
    uint32 transition = 0;
    // What the fade starts from: a state still playing, or the pose frozen when a fade
    // was interrupted.
    bool bFromFrozen = false;
    uint32 fromState = 0;
    float fromTime = 0.0f;
    StateWeights fromWeights;
    float fadeElapsed = 0.0f;
    float fadeDuration = 0.0f;

    // Transition indices, highest priority first, ties in list order.
    Vector<uint32> transitionOrder;
    // Per state, its index into m_blendSpaces, or NOT_FOUND for a clip state.
    Vector<uint32> blendSpaces;
    // Per node, how much the layer moves it (1 under a mask bone, else 0); empty without
    // a mask.
    Vector<float> nodeWeights;
    // The current state's pose, the pose the fade starts from and the layer's result.
    Vector<NodeTransform> statePose;
    Vector<NodeTransform> fromPose;
    Vector<NodeTransform> pose;
  };

  void
  bind();

  NODISCARD StateWeights
  computeStateWeights(uint32 layer, uint32 state) const noexcept;

  /**
   * Seconds the state lasts with these weights (blend spaces mix the lengths of their
   * clips, so they stay in step).
   */
  NODISCARD float
  getStateDuration(uint32 layer, uint32 state, const StateWeights& weights) const noexcept;

  NODISCARD float
  getStateSpeed(uint32 layer, uint32 state) const noexcept;

  /**
   * Normalized time after moving deltaTime seconds, wrapped or clamped as the state loops.
   */
  NODISCARD float
  advanceStateTime(uint32 layer, uint32 state, const StateWeights& weights, float time,
                   float deltaTime) const noexcept;

  void
  evaluateTransitions(uint32 layer) noexcept;

  NODISCARD bool
  conditionsHold(const AnimationTransition& transition) const noexcept;

  void
  startTransition(uint32 layer, uint32 transition) noexcept;

  void
  sampleState(uint32 layer, uint32 state, float time, const StateWeights& weights,
              Vector<NodeTransform>& outPose) noexcept;

  void
  sampleClip(uint32 clip, float phase, Span<NodeTransform> outPose) const noexcept;

  void
  computePose() noexcept;

  SPtr<Model> m_model;
  SPtr<AnimationGraph> m_graph;
  bool m_bBound = false;

  Vector<AnimationParameterValue> m_values;
  // Per graph clip: the clip (null when missing) and, per track, the node it moves.
  Vector<SPtr<AnimationClip>> m_clips;
  Vector<Vector<uint32>> m_clipTrackNodes;
  Vector<BlendSpace> m_blendSpaces;
  Vector<LayerRuntime> m_layers;

  Vector<NodeTransform> m_restPose;
  Vector<NodeTransform> m_scratchPose;
  Vector<NodeTransform> m_localPose;
  Vector<Matrix4> m_poseMatrices;
};

} // namespace chEngineSDK
