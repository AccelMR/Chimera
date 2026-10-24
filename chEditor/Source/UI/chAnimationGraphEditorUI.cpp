/************************************************************************/
/**
 * @file chAnimationGraphEditorUI.cpp
 * @author AccelMR
 * @date 2026/10/24
 * @brief Window that edits an animation graph as a node canvas.
 */
/************************************************************************/
#include "chAnimationGraphEditorUI.h"

#include "chAlgorithm.h"
#include "chAnimationClipAsset.h"
#include "chAnimationGraph.h"
#include "chAnimationGraphAsset.h"
#include "chAnimationGraphInstance.h"
#include "chAnimationPreviewUI.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chBlendSpace.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chLogger.h"
#include "chMath.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelPreview.h"
#include "chStringUtils.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(AnimationGraphEditorLog, All);

namespace ed = ax::NodeEditor;

namespace {
ConsoleVariable<bool> g_cvarShowAnimationGraphEditor("Editor.ShowAnimationGraphEditor",
                                                     false,
                                                     "Shows the Animation Graph Editor window.",
                                                     "ShowAnimationGraphEditor");
ConsoleVariable<bool> g_cvarGraphPreview("Editor.GraphPreview",
                                         true,
                                         "Shows the model preview under the animation graph.");
ConsoleVariable<float> g_cvarGraphPreviewHeight(
    "Editor.GraphPreviewHeight",
    280.0f,
    "Height in pixels of the model preview under the animation graph.");

// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 1100.0f;
constexpr float kWindowHeight = 650.0f;
constexpr float kListWidth = 260.0f;
constexpr float kDetailsWidth = 320.0f;

// Where states without a saved place go: a grid, in canvas units.
constexpr float kGridSpacingX = 260.0f;
constexpr float kGridSpacingY = 140.0f;
constexpr uint32 kGridColumns = 4;

// Node positions read back from the canvas may differ from the stored ones by rounding.
constexpr float kMoveTolerance = 0.5f;
// Node sizes are known only once they were drawn, so framing waits for the second frame.
constexpr uint32 kFramesBeforeNavigate = 2;
constexpr uint32 kMaxUndoSteps = 100;
constexpr uint32 kNone = ~0u;
constexpr SIZE_T kNameBufferSize = 128;

// Ids of the canvas: states count from 1, so 0 (invalid for the node editor) is never used.
// Input pins are even and output pins odd.
constexpr SIZE_T kAnyStateNode = 0x40000000u;
constexpr SIZE_T kAnyStateOutputPin = kAnyStateNode * 2 + 1;

constexpr float kDefaultTransitionDuration = 0.2f;

// The blend space plot.
constexpr float kPlotMargin = 0.15f;
constexpr float kMinPlotRange = 1e-4f;
constexpr float kMinGridSpacing = 8.0f;
constexpr float kMinSnapStep = 0.01f;
constexpr float kWeightLineWidth = 3.0f;

// The model preview under the canvas.
constexpr float kSplitterThickness = 6.0f;
constexpr float kMinPreviewHeight = 80.0f;
constexpr float kMinCanvasHeight = 80.0f;
constexpr float kPreviewPanelWidth = 240.0f;
constexpr float kActiveNodeBorderWidth = 3.0f;
constexpr float kStateTimeBarHeight = 4.0f;

/**
 * A test value of the preview, kept by name so it survives parameters being added,
 * removed or moved.
 */
struct PreviewValue
{
  String name;
  AnimationParameterType type = AnimationParameterType::Float;
  AnimationParameterValue value{};
};

constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);
constexpr ImVec4 kValidColor(0.45f, 0.85f, 0.45f, 1.0f);
constexpr ImVec4 kDefaultStateColor(1.0f, 0.75f, 0.3f, 1.0f);
constexpr ImVec4 kLinkColor(0.85f, 0.85f, 0.85f, 1.0f);
constexpr float kLinkThickness = 2.0f;

constexpr const ANSICHAR* kUnsavedPopup = "Unsaved Animation Graph###UnsavedAnimationGraph";
constexpr const ANSICHAR* kRemoveInUsePopup = "Remove?###AnimationGraphRemoveInUse";
constexpr const ANSICHAR* kAddStatePopup = "AddStatePopup";
constexpr const ANSICHAR* kNodePopup = "StatePopup";
constexpr const ANSICHAR* kLinkPopup = "TransitionPopup";
constexpr const ANSICHAR* kAddParameterPopup = "AddParameterPopup";

const ANSICHAR* const kStateKindNames[] = {"Clip", "Blend Space 1D", "Blend Space 2D"};
const ANSICHAR* const kParameterTypeNames[] = {"Float", "Bool", "Int", "Trigger"};

/**
 * What runs once the open graph is closed after the question about its changes.
 */
enum class AfterClose : uint8 { None, HideWindow, OpenOther, Callback };

/**
 * What the question about removing something still in use is about.
 */
enum class RemoveTarget : uint8 { None, Parameter, Clip };

struct GraphSnapshot
{
  AnimationGraph graph;
  AnimationGraphLayout layout;
  uint32 id = 0;
  // The change that leaves this state, shown as "Undo <name>" or "Redo <name>".
  String name;
};

struct GraphEditorState
{
  SPtr<AnimationGraphAsset> asset;
  // The copy being edited; the asset gets it on Save.
  AnimationGraph graph;
  AnimationGraphLayout layout;

  // Every state of the graph gets a new id, so "unsaved" is the id differing from the saved.
  uint32 id = 0;
  uint32 nextId = 1;
  uint32 savedId = 0;
  Vector<GraphSnapshot> undoSteps;
  Vector<GraphSnapshot> redoSteps;
  // While an item is held (a drag, a text field) its edits stay in one undo step.
  bool bEditOpen = false;

  uint32 layer = 0;
  uint32 selectedState = kNone;
  uint32 selectedTransition = kNone;
  bool bAnyStateSelected = false;
  uint32 selectStateRequest = kNone;
  uint32 selectTransitionRequest = kNone;

  uint32 validatedId = kNone;
  bool bValid = false;
  String error;

  String title;
  const IAsset* titleAsset = nullptr;
  bool bTitleDirty = true;

  ed::EditorContext* context = nullptr;
  bool bApplyPositions = false;
  bool bClearSelection = false;
  uint32 navigateFrames = 0;
  bool bFocus = false;
  bool bHasFocus = false;

  // Canvas requests that need the node editor, run while it is current.
  bool bAddStateAtCenter = false;
  ImVec2 canvasCenter{};
  ImVec2 popupPosition{};
  ed::NodeId contextNode;
  ed::LinkId contextLink;
  Vector<uint32> deletedStates;
  Vector<uint32> deletedTransitions;

  // The name field being typed in keeps its text here, by its ImGui id.
  ImGuiID nameEditId = 0;
  ANSICHAR nameBuffer[kNameBufferSize]{};
  ANSICHAR newBoneBuffer[kNameBufferSize]{};

  // Every clip of the project, read when a clip combo opens.
  Vector<SPtr<IAsset>> projectClips;

  // The blend space plot: the runtime's blend space of the shown state, built again only
  // when the graph or the state changes.
  BlendSpace blendSpace;
  bool bBlendSpaceBuilt = false;
  uint32 blendSpaceId = kNone;
  uint32 blendSpaceLayer = kNone;
  uint32 blendSpaceState = kNone;
  // While a sample is dragged the axes keep the range they had, or they would move under
  // the mouse.
  uint32 draggedSample = kNone;
  bool bDraggingTestPoint = false;
  float plotMin[2] = {0.0f, 0.0f};
  float plotMax[2] = {1.0f, 1.0f};
  uint32 contextSample = kNone;
  bool bSnap = false;
  float snapStep = 0.5f;
  float testPoint[2] = {0.0f, 0.0f};

  // The model preview runs a copy of the graph, made again after every edit.
  ModelPreview preview{"AnimationGraphPreview"};
  AnimationGraphInstance instance;
  SPtr<AnimationGraph> previewGraph;
  uint32 previewId = kNone;
  const ModelComponent* previewComponent = nullptr;
  // Parallel to the graph's parameters once synced.
  Vector<PreviewValue> previewValues;
  // The models the combo offers, found from the graph's clips.
  Vector<SPtr<IAsset>> previewModels;
  bool bPreviewModelPicked = false;

  RemoveTarget removeTarget = RemoveTarget::None;
  uint32 removeIndex = kNone;
  uint32 removeUses = 0;
  bool bOpenRemovePopup = false;

  AfterClose afterClose = AfterClose::None;
  SPtr<AnimationGraphAsset> assetToOpen;
  Function<void()> callback;
  bool bOpenUnsavedPopup = false;
};

GraphEditorState&
getState()
{
  static GraphEditorState state;
  return state;
}

NODISCARD FORCEINLINE ed::NodeId
getStateNodeId(uint32 state)
{
  return ed::NodeId(static_cast<SIZE_T>(state) + 1);
}

NODISCARD FORCEINLINE ed::PinId
getInputPinId(uint32 state)
{
  return ed::PinId((static_cast<SIZE_T>(state) + 1) * 2);
}

NODISCARD FORCEINLINE ed::PinId
getOutputPinId(uint32 state)
{
  return ed::PinId((static_cast<SIZE_T>(state) + 1) * 2 + 1);
}

NODISCARD FORCEINLINE ed::LinkId
getLinkId(uint32 transition)
{
  return ed::LinkId(static_cast<SIZE_T>(transition) + 1);
}

/*
 * False for a pin of no state of the layer. ANY_STATE has the value kNone has, so the
 * answer cannot be the state alone.
 */
NODISCARD bool
getPinState(ed::PinId pin, uint32 stateCount, uint32& outState, bool& outIsOutput)
{
  const SIZE_T id = pin.Get();
  if (id == kAnyStateOutputPin) {
    outState = AnimationGraph::ANY_STATE;
    outIsOutput = true;
    return true;
  }
  outIsOutput = (id % 2) == 1;
  const SIZE_T state = id / 2;
  if (state < 1 || state > stateCount) {
    return false;
  }
  outState = static_cast<uint32>(state - 1);
  return true;
}

/*
 * The graph may not pass validate yet, so every index it holds is checked before use.
 */
NODISCARD const ANSICHAR*
getClipName(const AnimationGraph& graph, uint32 clip)
{
  const Vector<AnimationGraphClip>& clips = graph.getClips();
  if (clip >= clips.size()) {
    return "(no clip)";
  }
  return clips[clip].clip ? clips[clip].clip->getName() : "(missing clip)";
}

NODISCARD const ANSICHAR*
getParameterName(const AnimationGraph& graph, uint32 parameter)
{
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  return parameter < parameters.size() ? parameters[parameter].name.c_str() : "(none)";
}

NODISCARD const ANSICHAR*
getStateName(const AnimationLayer& layer, uint32 state)
{
  if (state == AnimationGraph::ANY_STATE) {
    return "Any State";
  }
  return state < layer.states.size() ? layer.states[state].name.c_str() : "(no state)";
}

NODISCARD const ANSICHAR*
getOpText(AnimationConditionOp op)
{
  switch (op) {
  case AnimationConditionOp::IsSet:
    return "is set";
  case AnimationConditionOp::IsNotSet:
    return "is not set";
  case AnimationConditionOp::Greater:
    return ">";
  case AnimationConditionOp::Less:
    return "<";
  case AnimationConditionOp::GreaterOrEqual:
    return ">=";
  case AnimationConditionOp::LessOrEqual:
    return "<=";
  case AnimationConditionOp::Equal:
    return "==";
  case AnimationConditionOp::NotEqual:
    return "!=";
  }
  return "?";
}

/*
 * The tests AnimationGraph::validate allows for a parameter of the type, the first one
 * being the one a new condition starts with.
 */
NODISCARD Span<const AnimationConditionOp>
getAllowedOps(AnimationParameterType type)
{
  static constexpr AnimationConditionOp kFloatOps[] = {
      AnimationConditionOp::Greater, AnimationConditionOp::Less,
      AnimationConditionOp::GreaterOrEqual, AnimationConditionOp::LessOrEqual};
  static constexpr AnimationConditionOp kIntOps[] = {
      AnimationConditionOp::Equal,          AnimationConditionOp::NotEqual,
      AnimationConditionOp::Greater,        AnimationConditionOp::Less,
      AnimationConditionOp::GreaterOrEqual, AnimationConditionOp::LessOrEqual};
  static constexpr AnimationConditionOp kBoolOps[] = {AnimationConditionOp::IsSet,
                                                      AnimationConditionOp::IsNotSet};
  static constexpr AnimationConditionOp kTriggerOps[] = {AnimationConditionOp::IsSet};
  switch (type) {
  case AnimationParameterType::Float:
    return kFloatOps;
  case AnimationParameterType::Int:
    return kIntOps;
  case AnimationParameterType::Bool:
    return kBoolOps;
  case AnimationParameterType::Trigger:
    return kTriggerOps;
  }
  return {};
}

NODISCARD bool
isBlendParameter(AnimationParameterType type)
{
  return type == AnimationParameterType::Float || type == AnimationParameterType::Int;
}

/*
 * True when the state points at a clip or parameter that does not fit, so its node shows
 * it in red.
 */
NODISCARD bool
isStateBroken(const AnimationGraph& graph, const AnimationState& state)
{
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  const auto badParameter = [&parameters](uint32 parameter) {
    return parameter >= parameters.size() || !isBlendParameter(parameters[parameter].type);
  };
  switch (state.kind) {
  case AnimationStateKind::Clip:
    return state.clip >= graph.getClips().size();
  case AnimationStateKind::BlendSpace1D:
    return badParameter(state.parameterX) || state.samples.empty();
  case AnimationStateKind::BlendSpace2D:
    return badParameter(state.parameterX) || badParameter(state.parameterY) ||
           state.parameterX == state.parameterY || state.samples.empty();
  }
  return true;
}

/*
 * The name with a number added until no other item of the list has it (ignoring case).
 */
template<typename T, typename GetName>
NODISCARD String
makeUniqueName(const Vector<T>& items, StringView base, GetName&& getName)
{
  String name(base);
  for (uint32 suffix = 1;; ++suffix) {
    const bool bTaken = Algorithm::containsIf(items, [&name, &getName](const T& item) {
      return StringUtils::equalsIgnoreCase(getName(item), name);
    });
    if (!bTaken) {
      return name;
    }
    name = StringUtils::format("{0} {1}", base, suffix);
  }
}

/*
 * Gives every layer and state a place, keeping the ones already saved.
 */
void
fitLayout(GraphEditorState& state)
{
  const Vector<AnimationLayer>& layers = state.graph.getLayers();
  const SIZE_T oldLayerCount = state.layout.layers.size();
  state.layout.layers.resize(layers.size());
  for (SIZE_T l = 0; l < layers.size(); ++l) {
    AnimationGraphLayerLayout& layerLayout = state.layout.layers[l];
    if (l >= oldLayerCount) {
      layerLayout.anyStatePosition = Vector2(-kGridSpacingX, 0.0f);
    }
    const SIZE_T oldStateCount = layerLayout.statePositions.size();
    layerLayout.statePositions.resize(layers[l].states.size());
    for (SIZE_T s = oldStateCount; s < layerLayout.statePositions.size(); ++s) {
      layerLayout.statePositions[s] =
          Vector2(kGridSpacingX * static_cast<float>(s % kGridColumns),
                  kGridSpacingY * static_cast<float>(s / kGridColumns));
    }
  }
}

void
resetView(GraphEditorState& state)
{
  state.bApplyPositions = true;
  state.bClearSelection = true;
  state.navigateFrames = kFramesBeforeNavigate;
  state.selectedState = kNone;
  state.selectedTransition = kNone;
  state.bAnyStateSelected = false;
}

void
loadDocument(GraphEditorState& state, const SPtr<AnimationGraphAsset>& asset)
{
  state.asset = asset;
  const SPtr<AnimationGraph>& graph = asset->getGraph();
  state.graph = graph ? *graph : AnimationGraph();
  state.layout = asset->getLayout();
  fitLayout(state);
  state.id = 0;
  state.nextId = 1;
  state.savedId = 0;
  state.undoSteps.clear();
  state.redoSteps.clear();
  state.bEditOpen = false;
  state.layer = 0;
  state.validatedId = kNone;
  state.bTitleDirty = true;
  state.bPreviewModelPicked = false;
  state.previewId = kNone;
  state.previewValues.clear();
  resetView(state);
}

void
closeDocument(GraphEditorState& state)
{
  state.asset.reset();
  state.graph = AnimationGraph();
  state.layout = AnimationGraphLayout();
  state.undoSteps.clear();
  state.redoSteps.clear();
  state.bEditOpen = false;
  state.id = state.savedId = 0;
  state.validatedId = kNone;
  state.bTitleDirty = true;
  state.projectClips.clear();
  state.bPreviewModelPicked = false;
  state.previewId = kNone;
  state.previewValues.clear();
  state.previewModels.clear();
  state.instance.setGraph(nullptr);
  state.previewGraph.reset();
  if (state.preview.getModel()) {
    state.preview.setModel(nullptr);
  }
  resetView(state);
}

NODISCARD bool
isDirty(const GraphEditorState& state)
{
  return state.asset && state.id != state.savedId;
}

/*
 * Call before changing the graph or its layout: keeps what they were as one undo step.
 */
void
beginChange(GraphEditorState& state, StringView name)
{
  state.bEditOpen = false;
  if (state.undoSteps.size() >= kMaxUndoSteps) {
    state.undoSteps.erase(state.undoSteps.begin());
  }
  state.undoSteps.push_back(
      {.graph = state.graph, .layout = state.layout, .id = state.id, .name = String(name)});
  state.redoSteps.clear();
  state.id = state.nextId++;
  state.bTitleDirty = true;
}

/*
 * Like beginChange, but edits made while the same item stays held join the step already
 * open, so a drag is one step. Call it before writing the new value.
 */
void
editValue(GraphEditorState& state, StringView name)
{
  if (!state.bEditOpen) {
    beginChange(state, name);
    state.bEditOpen = true;
  }
}

/*
 * After a change that adds or removes states or transitions: indices moved, so the canvas
 * places every node again and forgets its selection.
 */
void
onStructureChanged(GraphEditorState& state)
{
  fitLayout(state);
  state.bApplyPositions = true;
  state.bClearSelection = true;
}

/*
 * Takes the last step of from, keeping the current state in to under the same name.
 */
void
applyStep(GraphEditorState& state, Vector<GraphSnapshot>& from, Vector<GraphSnapshot>& to)
{
  if (from.empty()) {
    return;
  }
  state.bEditOpen = false;
  GraphSnapshot step = std::move(from.back());
  from.pop_back();
  to.push_back({.graph = std::move(state.graph),
                .layout = std::move(state.layout),
                .id = state.id,
                .name = step.name});
  state.graph = std::move(step.graph);
  state.layout = std::move(step.layout);
  state.id = step.id;
  if (state.layer >= state.layout.layers.size()) {
    state.layer = 0;
  }
  state.bTitleDirty = true;
  onStructureChanged(state);
}

/*
 * The saved graph keeps only the clips some state plays, so dropped clips stop being
 * references; the copy being edited keeps its list, so its indices do not move.
 */
NODISCARD bool
saveDocument(GraphEditorState& state)
{
  if (!state.asset) {
    return false;
  }
  const SPtr<AnimationGraph> saved = chMakeShared<AnimationGraph>(state.graph);
  saved->removeUnusedClips();
  state.asset->setGraph(saved);
  state.asset->setLayout(state.layout);
  if (!AssetManager::instance().saveAsset(state.asset)) {
    CH_LOG_ERROR(AnimationGraphEditorLog, "Could not save the animation graph {0}.",
                 state.asset->getName());
    return false;
  }
  state.savedId = state.id;
  state.bTitleDirty = true;
  return true;
}

void
setVisibleValue(bool bVisible)
{
  g_cvarShowAnimationGraphEditor.set(bVisible, ConsoleVariableSource::Console);
  EngineConfig::setUserValue("Editor", "Editor", "ShowAnimationGraphEditor",
                             bVisible ? "true" : "false");
}

void
finishClose(GraphEditorState& state)
{
  const AfterClose afterClose = state.afterClose;
  state.afterClose = AfterClose::None;
  switch (afterClose) {
  case AfterClose::HideWindow:
    closeDocument(state);
    setVisibleValue(false);
    break;
  case AfterClose::OpenOther:
    loadDocument(state, state.assetToOpen);
    state.bFocus = true;
    break;
  case AfterClose::Callback: {
    closeDocument(state);
    // The callback may ask again, so it runs once the state no longer holds it.
    const Function<void()> callback = std::move(state.callback);
    state.callback = nullptr;
    if (callback) {
      callback();
    }
    break;
  }
  case AfterClose::None:
    break;
  }
  state.assetToOpen.reset();
}

void
requestClose(GraphEditorState& state, AfterClose afterClose)
{
  state.afterClose = afterClose;
  if (isDirty(state)) {
    state.bOpenUnsavedPopup = true;
  }
  else {
    finishClose(state);
  }
}

void
renderUnsavedPopup(GraphEditorState& state)
{
  if (state.bOpenUnsavedPopup) {
    state.bOpenUnsavedPopup = false;
    ImGui::OpenPopup(kUnsavedPopup);
  }
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(kUnsavedPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::Text("Save the changes to the animation graph %s?",
              state.asset ? state.asset->getName() : "");
  ImGui::Separator();
  const float buttonWidth = ImGui::GetFontSize() * 7.0f;
  const bool bSave = ImGui::Button("Save", ImVec2(buttonWidth, 0.0f));
  ImGui::SameLine();
  const bool bDiscard = ImGui::Button("Don't Save", ImVec2(buttonWidth, 0.0f));
  ImGui::SameLine();
  const bool bCancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) ||
                       ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (bSave || bDiscard || bCancel) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if ((bSave && saveDocument(state)) || bDiscard) {
    finishClose(state);
  }
  else if (bSave || bCancel) {
    state.afterClose = AfterClose::None;
    state.assetToOpen.reset();
    state.callback = nullptr;
  }
}

void
removeNow(GraphEditorState& state, RemoveTarget target, uint32 index)
{
  if (target == RemoveTarget::Parameter) {
    beginChange(state, "Remove Parameter");
    state.graph.removeParameter(index);
  }
  else if (target == RemoveTarget::Clip) {
    beginChange(state, "Remove Clip");
    state.graph.removeClip(index);
  }
}

/*
 * Removes at once what nothing uses; otherwise asks first, saying how many places use it.
 */
void
requestRemove(GraphEditorState& state, RemoveTarget target, uint32 index)
{
  const uint32 uses = target == RemoveTarget::Parameter ? state.graph.countParameterUses(index)
                                                        : state.graph.countClipUses(index);
  if (uses == 0) {
    removeNow(state, target, index);
    return;
  }
  state.removeTarget = target;
  state.removeIndex = index;
  state.removeUses = uses;
  state.bOpenRemovePopup = true;
}

void
renderRemovePopup(GraphEditorState& state)
{
  if (state.bOpenRemovePopup) {
    state.bOpenRemovePopup = false;
    ImGui::OpenPopup(kRemoveInUsePopup);
  }
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(kRemoveInUsePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  const bool bParameter = state.removeTarget == RemoveTarget::Parameter;
  const ANSICHAR* const name = bParameter ? getParameterName(state.graph, state.removeIndex)
                                          : getClipName(state.graph, state.removeIndex);
  ImGui::Text("%s is used in %u place%s.", name, state.removeUses,
              state.removeUses == 1 ? "" : "s");
  ImGui::TextUnformatted(bParameter
                             ? "Conditions on it are removed; blend spaces that read it are "
                               "left without a parameter."
                             : "States that play it are left without a clip; blend space "
                               "samples of it are removed.");
  ImGui::Separator();
  const float buttonWidth = ImGui::GetFontSize() * 7.0f;
  const bool bRemove = ImGui::Button("Remove", ImVec2(buttonWidth, 0.0f));
  ImGui::SameLine();
  const bool bCancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) ||
                       ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (bRemove || bCancel) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if (bRemove) {
    removeNow(state, state.removeTarget, state.removeIndex);
  }
  if (bRemove || bCancel) {
    state.removeTarget = RemoveTarget::None;
  }
}

void
updateTitle(GraphEditorState& state)
{
  if (!state.bTitleDirty && state.titleAsset == state.asset.get()) {
    return;
  }
  state.bTitleDirty = false;
  state.titleAsset = state.asset.get();
  state.title = state.asset
                    ? StringUtils::format(ICON_FA_PROJECT_DIAGRAM "  {0}{1}###AnimationGraphEditor",
                                          state.asset->getName(), isDirty(state) ? "*" : "")
                    : String(ICON_FA_PROJECT_DIAGRAM "  Animation Graph###AnimationGraphEditor");
}

/*
 * A text field for a name that gives the new text once the field is left or Enter is
 * pressed. The text being typed is kept in the state, because the field is drawn from the
 * graph's value again every frame.
 */
NODISCARD bool
inputName(GraphEditorState& state, const ANSICHAR* label, StringView value, String& outValue)
{
  const ImGuiID id = ImGui::GetID(label);
  ANSICHAR scratch[kNameBufferSize];
  ANSICHAR* buffer = state.nameBuffer;
  if (state.nameEditId != id) {
    CH_PARAMETER_UNUSED(StringUtils::copyToBuffer(scratch, value));
    buffer = scratch;
  }
  ImGui::InputText(label, buffer, kNameBufferSize);
  if (ImGui::IsItemActivated()) {
    state.nameEditId = id;
    CH_PARAMETER_UNUSED(StringUtils::copyToBuffer(state.nameBuffer, value));
  }
  const bool bDone = ImGui::IsItemDeactivatedAfterEdit();
  if (bDone) {
    outValue = buffer;
  }
  if (ImGui::IsItemDeactivated()) {
    state.nameEditId = 0;
  }
  return bDone && outValue != value;
}

/*
 * A combo of every clip of the project, which also takes a clip dropped on it. Gives the
 * chosen clip, loaded, or null.
 */
NODISCARD SPtr<AnimationClipAsset>
pickClip(GraphEditorState& state, const ANSICHAR* label, uint32 current)
{
  SPtr<IAsset> chosen;
  if (ImGui::BeginCombo(label, getClipName(state.graph, current))) {
    if (ImGui::IsWindowAppearing()) {
      state.projectClips.clear();
      AssetManager::instance().getAssetsOfType(
          AssetTypeTraits<AnimationClipAsset>::getTypeId(), state.projectClips);
      Algorithm::sort(state.projectClips, [](const SPtr<IAsset>& a, const SPtr<IAsset>& b) {
        return StringUtils::compareIgnoreCase(a->getName(), b->getName()) < 0;
      });
    }
    const Vector<AnimationGraphClip>& clips = state.graph.getClips();
    const UUID currentId = current < clips.size() ? clips[current].clipId : UUID::null();
    for (const SPtr<IAsset>& clip : state.projectClips) {
      ImGui::PushID(clip.get());
      const bool bSelected = clip->getUUID() == currentId;
      if (ImGui::Selectable(clip->getName(), bSelected)) {
        chosen = clip;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", clip->getAssetPath());
      }
      ImGui::PopID();
    }
    if (state.projectClips.empty()) {
      ImGui::TextDisabled("The project has no animation clips.");
    }
    ImGui::EndCombo();
  }
  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<AnimationClipAsset> dropped = AssetDragDrop::acceptAnimation()) {
      chosen = dropped;
    }
    ImGui::EndDragDropTarget();
  }
  if (!chosen) {
    return nullptr;
  }
  if (!chosen->isLoaded() && !AssetManager::instance().syncLoadAsset(chosen)) {
    CH_LOG_ERROR(AnimationGraphEditorLog, "Could not load the clip {0}.", chosen->getName());
    return nullptr;
  }
  return std::static_pointer_cast<AnimationClipAsset>(chosen);
}

/*
 * A combo of the parameters whose type passes accepts, with "(none)" first when allowed.
 * True when the choice changed, written to outParameter.
 */
template<typename Accepts>
NODISCARD bool
pickParameter(const AnimationGraph& graph,
              const ANSICHAR* label,
              uint32 current,
              bool bAllowNone,
              Accepts&& accepts,
              uint32& outParameter)
{
  bool bChanged = false;
  if (ImGui::BeginCombo(label, getParameterName(graph, current))) {
    if (bAllowNone && ImGui::Selectable("(none)", current == AnimationGraph::NO_PARAMETER)) {
      outParameter = AnimationGraph::NO_PARAMETER;
      bChanged = current != outParameter;
    }
    const Vector<AnimationParameter>& parameters = graph.getParameters();
    for (uint32 p = 0; p < parameters.size(); ++p) {
      if (!accepts(parameters[p].type)) {
        continue;
      }
      ImGui::PushID(static_cast<int32>(p));
      if (ImGui::Selectable(parameters[p].name.c_str(), current == p) && current != p) {
        outParameter = p;
        bChanged = true;
      }
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  return bChanged;
}

/*
 * A new state at position on the canvas, selected once the canvas has drawn it.
 */
void
addState(GraphEditorState& state, AnimationStateKind kind, const ImVec2& position,
         const SPtr<AnimationClipAsset>& clip = nullptr)
{
  if (state.layer >= state.graph.getLayers().size()) {
    return;
  }
  beginChange(state, "Add State");
  AnimationLayer& layer = state.graph.getLayersForEdit()[state.layer];
  AnimationState newState{.kind = kind, .clip = AnimationGraph::NOT_FOUND};
  const StringView baseName = clip ? StringView(clip->getName()) : StringView("New State");
  newState.name = makeUniqueName(layer.states, baseName,
                                 [](const AnimationState& s) -> StringView { return s.name; });
  if (clip) {
    newState.clip = state.graph.findOrAddClip(clip->getUUID(), clip);
  }
  if (kind != AnimationStateKind::Clip) {
    // The first parameters a blend space can read, so it starts as close to working as it can.
    newState.parameterX = AnimationGraph::NO_PARAMETER;
    newState.parameterY = AnimationGraph::NO_PARAMETER;
    const Vector<AnimationParameter>& parameters = state.graph.getParameters();
    for (uint32 p = 0; p < parameters.size(); ++p) {
      if (!isBlendParameter(parameters[p].type)) {
        continue;
      }
      if (newState.parameterX == AnimationGraph::NO_PARAMETER) {
        newState.parameterX = p;
      }
      else if (newState.parameterY == AnimationGraph::NO_PARAMETER) {
        newState.parameterY = p;
      }
    }
  }
  layer.states.push_back(std::move(newState));
  const uint32 index = static_cast<uint32>(layer.states.size() - 1);
  onStructureChanged(state);
  state.layout.layers[state.layer].statePositions[index] = Vector2(position.x, position.y);
  state.selectStateRequest = index;
}

/*
 * A transition from the Any State starts without an exit time, which it cannot have, so it
 * needs a condition before the graph plays.
 */
void
addTransition(GraphEditorState& state, uint32 source, uint32 target)
{
  beginChange(state, "Add Transition");
  AnimationLayer& layer = state.graph.getLayersForEdit()[state.layer];
  const bool bFromAny = source == AnimationGraph::ANY_STATE;
  layer.transitions.push_back({.source = source,
                               .target = target,
                               .duration = kDefaultTransitionDuration,
                               .bHasExitTime = !bFromAny,
                               .exitTime = 1.0f});
  onStructureChanged(state);
  state.selectTransitionRequest = static_cast<uint32>(layer.transitions.size() - 1);
}

/*
 * Transitions first, then states, each from the highest index, so the indices still to
 * remove do not move.
 */
void
applyDeletes(GraphEditorState& state)
{
  if (state.deletedStates.empty() && state.deletedTransitions.empty()) {
    return;
  }
  beginChange(state, "Delete");
  AnimationLayer& layer = state.graph.getLayersForEdit()[state.layer];
  Algorithm::sort(state.deletedTransitions, [](uint32 a, uint32 b) { return a > b; });
  for (const uint32 transition : state.deletedTransitions) {
    if (transition < layer.transitions.size()) {
      layer.transitions.erase(layer.transitions.begin() + transition);
    }
  }
  Algorithm::sort(state.deletedStates, [](uint32 a, uint32 b) { return a > b; });
  Vector<Vector2>& positions = state.layout.layers[state.layer].statePositions;
  for (const uint32 removed : state.deletedStates) {
    if (removed < layer.states.size()) {
      state.graph.removeState(state.layer, removed);
      positions.erase(positions.begin() + removed);
    }
  }
  state.deletedStates.clear();
  state.deletedTransitions.clear();
  onStructureChanged(state);
}

void
renderToolbar(GraphEditorState& state)
{
  if (ImGui::Button(ICON_FA_SAVE "  Save")) {
    CH_PARAMETER_UNUSED(saveDocument(state));
  }
  ImGui::SetItemTooltip("Ctrl+S");
  ImGui::SameLine();
  ImGui::BeginDisabled(state.undoSteps.empty());
  if (ImGui::Button(ICON_FA_UNDO)) {
    applyStep(state, state.undoSteps, state.redoSteps);
  }
  if (!state.undoSteps.empty()) {
    ImGui::SetItemTooltip("Undo %s (Ctrl+Z)", state.undoSteps.back().name.c_str());
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.redoSteps.empty());
  if (ImGui::Button(ICON_FA_REDO)) {
    applyStep(state, state.redoSteps, state.undoSteps);
  }
  if (!state.redoSteps.empty()) {
    ImGui::SetItemTooltip("Redo %s (Ctrl+Y)", state.redoSteps.back().name.c_str());
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.layer >= state.graph.getLayers().size());
  if (ImGui::Button(ICON_FA_PLUS "  State")) {
    state.bAddStateAtCenter = true;
  }
  ImGui::SetItemTooltip("Adds a clip state in the middle of the canvas. Right click the "
                        "canvas for blend spaces, or drop a clip on it.");
  ImGui::EndDisabled();
  ImGui::SameLine();
  const bool bPreview = g_cvarGraphPreview.get();
  if (bPreview) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  }
  if (ImGui::Button(ICON_FA_EYE "  Preview")) {
    g_cvarGraphPreview.set(!bPreview, ConsoleVariableSource::Console);
    EngineConfig::setUserValue("Editor", "Editor", "GraphPreview", bPreview ? "false" : "true");
  }
  if (bPreview) {
    ImGui::PopStyleColor();
  }
  ImGui::SetItemTooltip("Shows the graph running on a model under the canvas");

  ImGui::SameLine();
  if (state.bValid) {
    ImGui::TextColored(kValidColor, ICON_FA_CHECK_CIRCLE "  Ready to play");
  }
  else {
    ImGui::TextColored(kErrorColor, ICON_FA_EXCLAMATION_TRIANGLE "  %s (it does not play)",
                       state.error.c_str());
  }
}

/*
 * Puts the next item, one frame high and wide, at the right end of the current line.
 */
void
sameLineAtRight()
{
  ImGui::SameLine();
  const float space = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Math::max(space, 0.0f));
}

/*
 * A small button on the right end of a separator line made by SeparatorText.
 */
NODISCARD bool
headerButton(const ANSICHAR* label)
{
  sameLineAtRight();
  return ImGui::SmallButton(label);
}

void
renderLayerList(GraphEditorState& state)
{
  ImGui::SeparatorText(ICON_FA_LAYER_GROUP "  Layers");
  if (headerButton(ICON_FA_PLUS "##AddLayer")) {
    beginChange(state, "Add Layer");
    const String name = makeUniqueName(state.graph.getLayers(), "Layer",
                                       [](const AnimationLayer& l) -> StringView {
                                         return l.name;
                                       });
    state.layer = state.graph.addLayer({.name = name});
    onStructureChanged(state);
    resetView(state);
  }
  ImGui::SetItemTooltip("Adds a layer, which replaces the nodes of its mask over the "
                        "layers above it.");

  const Vector<AnimationLayer>& layers = state.graph.getLayers();
  for (uint32 l = 0; l < layers.size(); ++l) {
    ImGui::PushID(static_cast<int32>(l));
    if (ImGui::Selectable(layers[l].name.c_str(), state.layer == l) && state.layer != l) {
      state.layer = l;
      resetView(state);
    }
    ImGui::PopID();
  }
  if (layers.empty()) {
    ImGui::TextDisabled("No layers.");
  }
}

void
renderParameterDefault(GraphEditorState& state, uint32 index)
{
  AnimationParameter& parameter = state.graph.getParametersForEdit()[index];
  AnimationParameterValue value = parameter.defaultValue;
  bool bChanged = false;
  switch (parameter.type) {
  case AnimationParameterType::Float:
    bChanged = ImGui::DragFloat("##Default", &value.floatValue, 0.01f);
    break;
  case AnimationParameterType::Int:
    bChanged = ImGui::DragInt("##Default", &value.intValue);
    break;
  case AnimationParameterType::Bool: {
    bool bValue = value.intValue != 0;
    bChanged = ImGui::Checkbox("##Default", &bValue);
    value.intValue = bValue ? 1 : 0;
    break;
  }
  case AnimationParameterType::Trigger:
    ImGui::TextDisabled("off");
    break;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Value the parameter starts with");
  }
  if (bChanged) {
    editValue(state, "Edit Parameter Default");
    state.graph.getParametersForEdit()[index].defaultValue = value;
  }
}

void
renderParameterList(GraphEditorState& state)
{
  ImGui::SeparatorText(ICON_FA_SLIDERS_H "  Parameters");
  if (headerButton(ICON_FA_PLUS "##AddParameter")) {
    ImGui::OpenPopup(kAddParameterPopup);
  }
  ImGui::SetItemTooltip("Adds a parameter, which scripts set to drive the graph.");
  if (ImGui::BeginPopup(kAddParameterPopup)) {
    for (uint32 t = 0; t < 4; ++t) {
      if (ImGui::MenuItem(kParameterTypeNames[t])) {
        beginChange(state, "Add Parameter");
        const String name = makeUniqueName(
            state.graph.getParameters(), StringUtils::format("New{0}", kParameterTypeNames[t]),
            [](const AnimationParameter& p) -> StringView { return p.name; });
        CH_PARAMETER_UNUSED(state.graph.addParameter(
            {.name = name, .type = static_cast<AnimationParameterType>(t)}));
      }
    }
    ImGui::EndPopup();
  }

  const float typeWidth = ImGui::GetFontSize() * 4.5f;
  const float valueWidth = ImGui::GetFontSize() * 3.5f;
  const uint32 count = static_cast<uint32>(state.graph.getParameters().size());
  for (uint32 p = 0; p < count; ++p) {
    ImGui::PushID(static_cast<int32>(p));
    const AnimationParameter& parameter = state.graph.getParameters()[p];
    const float nameWidth = ImGui::GetContentRegionAvail().x - typeWidth - valueWidth -
                            ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x * 3.0f;
    ImGui::SetNextItemWidth(Math::max(nameWidth, ImGui::GetFontSize() * 3.0f));
    String newName;
    if (inputName(state, "##Name", parameter.name, newName)) {
      beginChange(state, "Rename Parameter");
      state.graph.getParametersForEdit()[p].name = std::move(newName);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(typeWidth);
    const uint32 type = static_cast<uint32>(state.graph.getParameters()[p].type);
    if (ImGui::BeginCombo("##Type", kParameterTypeNames[Math::min(type, 3u)])) {
      for (uint32 t = 0; t < 4; ++t) {
        if (ImGui::Selectable(kParameterTypeNames[t], t == type) && t != type) {
          // Conditions that no longer fit the type show as errors until changed.
          beginChange(state, "Change Parameter Type");
          AnimationParameter& changed = state.graph.getParametersForEdit()[p];
          changed.type = static_cast<AnimationParameterType>(t);
          changed.defaultValue = {};
        }
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(valueWidth);
    renderParameterDefault(state, p);

    ImGui::SameLine();
    if (ImGui::SmallButton(ICON_FA_TRASH)) {
      requestRemove(state, RemoveTarget::Parameter, p);
    }
    ImGui::PopID();
    if (state.graph.getParameters().size() != count) {
      break;
    }
  }
  if (state.graph.getParameters().empty()) {
    ImGui::TextDisabled("No parameters.");
  }
}

void
renderClipList(GraphEditorState& state)
{
  ImGui::SeparatorText(ICON_FA_FILM "  Clips");
  const uint32 count = static_cast<uint32>(state.graph.getClips().size());
  for (uint32 c = 0; c < count; ++c) {
    ImGui::PushID(static_cast<int32>(c));
    const uint32 uses = state.graph.countClipUses(c);
    if (uses == 0) {
      ImGui::TextDisabled("%s", getClipName(state.graph, c));
      ImGui::SetItemTooltip("Not used; left out when the graph is saved.");
    }
    else {
      ImGui::TextUnformatted(getClipName(state.graph, c));
    }
    sameLineAtRight();
    if (ImGui::SmallButton(ICON_FA_TRASH)) {
      requestRemove(state, RemoveTarget::Clip, c);
    }
    ImGui::PopID();
    if (state.graph.getClips().size() != count) {
      break;
    }
  }
  if (state.graph.getClips().empty()) {
    ImGui::TextDisabled("States add the clips they play.");
  }
}

/*
 * The canvas follows the editor theme, so its colors are taken from the ImGui style.
 */
void
applyCanvasStyle()
{
  const ImGuiStyle& style = ImGui::GetStyle();
  ed::Style& canvasStyle = ed::GetStyle();
  canvasStyle.Colors[ed::StyleColor_Bg] = style.Colors[ImGuiCol_FrameBg];
  canvasStyle.Colors[ed::StyleColor_NodeBg] = style.Colors[ImGuiCol_WindowBg];
  canvasStyle.Colors[ed::StyleColor_NodeBorder] = style.Colors[ImGuiCol_Border];
  canvasStyle.NodeRounding = style.FrameRounding + 2.0f;
}

void
renderStateNode(const GraphEditorState& state, const AnimationLayer& layer, uint32 index)
{
  const AnimationState& animationState = layer.states[index];
  // The state the preview is in has a green border and a bar with its time.
  const bool bActive = g_cvarGraphPreview.get() && state.instance.isBound() &&
                       state.instance.getCurrentState(state.layer) == index;
  if (bActive) {
    ed::PushStyleColor(ed::StyleColor_NodeBorder, kValidColor);
    ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, kActiveNodeBorderWidth);
  }
  ed::BeginNode(getStateNodeId(index));
  ed::BeginPin(getInputPinId(index), ed::PinKind::Input);
  ImGui::TextUnformatted(ICON_FA_CARET_RIGHT);
  ed::EndPin();
  ImGui::SameLine();

  ImGui::BeginGroup();
  const bool bBroken = isStateBroken(state.graph, animationState);
  if (index == layer.defaultState) {
    ImGui::TextColored(bBroken ? kErrorColor : kDefaultStateColor, ICON_FA_PLAY "  %s",
                       animationState.name.c_str());
  }
  else if (bBroken) {
    ImGui::TextColored(kErrorColor, "%s", animationState.name.c_str());
  }
  else {
    ImGui::TextUnformatted(animationState.name.c_str());
  }
  switch (animationState.kind) {
  case AnimationStateKind::Clip:
    ImGui::TextDisabled("%s", getClipName(state.graph, animationState.clip));
    break;
  case AnimationStateKind::BlendSpace1D:
    ImGui::TextDisabled("Blend 1D: %s", getParameterName(state.graph, animationState.parameterX));
    break;
  case AnimationStateKind::BlendSpace2D:
    ImGui::TextDisabled("Blend 2D: %s, %s",
                        getParameterName(state.graph, animationState.parameterX),
                        getParameterName(state.graph, animationState.parameterY));
    break;
  }
  if (bActive) {
    const float time = state.instance.getStateTime(state.layer);
    const float fraction =
        animationState.bLooping ? time - Math::floor(time) : Math::min(time, 1.0f);
    ImGui::ProgressBar(fraction, ImVec2(ImGui::GetItemRectSize().x, kStateTimeBarHeight), "");
  }
  ImGui::EndGroup();

  ImGui::SameLine();
  ed::BeginPin(getOutputPinId(index), ed::PinKind::Output);
  ImGui::TextUnformatted(ICON_FA_CARET_RIGHT);
  ed::EndPin();
  ed::EndNode();
  if (bActive) {
    ed::PopStyleVar();
    ed::PopStyleColor();
  }
}

void
renderAnyStateNode()
{
  ed::BeginNode(ed::NodeId(kAnyStateNode));
  ImGui::BeginGroup();
  ImGui::TextUnformatted(ICON_FA_ASTERISK "  Any State");
  ImGui::TextDisabled("From every state");
  ImGui::EndGroup();
  ImGui::SameLine();
  ed::BeginPin(ed::PinId(kAnyStateOutputPin), ed::PinKind::Output);
  ImGui::TextUnformatted(ICON_FA_CARET_RIGHT);
  ed::EndPin();
  ed::EndNode();
}

/*
 * Positions moved on the canvas become one undo step, once the mouse is released.
 */
void
readNodePositions(GraphEditorState& state, uint32 stateCount)
{
  AnimationGraphLayerLayout& layout = state.layout.layers[state.layer];
  const auto moved = [](const ImVec2& position, const Vector2& stored) {
    return Math::abs(position.x - stored.x) > kMoveTolerance ||
           Math::abs(position.y - stored.y) > kMoveTolerance;
  };

  bool bMoved = moved(ed::GetNodePosition(ed::NodeId(kAnyStateNode)), layout.anyStatePosition);
  for (uint32 s = 0; s < stateCount && !bMoved; ++s) {
    bMoved = moved(ed::GetNodePosition(getStateNodeId(s)), layout.statePositions[s]);
  }
  if (!bMoved) {
    return;
  }

  beginChange(state, "Move States");
  AnimationGraphLayerLayout& newLayout = state.layout.layers[state.layer];
  const ImVec2 anyPosition = ed::GetNodePosition(ed::NodeId(kAnyStateNode));
  newLayout.anyStatePosition = Vector2(anyPosition.x, anyPosition.y);
  for (uint32 s = 0; s < stateCount; ++s) {
    const ImVec2 position = ed::GetNodePosition(getStateNodeId(s));
    newLayout.statePositions[s] = Vector2(position.x, position.y);
  }
}

void
readSelection(GraphEditorState& state, uint32 stateCount, uint32 transitionCount)
{
  state.selectedState = kNone;
  state.selectedTransition = kNone;
  state.bAnyStateSelected = false;
  ed::NodeId node;
  ed::LinkId link;
  if (ed::GetSelectedNodes(&node, 1) == 1) {
    const SIZE_T id = node.Get();
    if (id == kAnyStateNode) {
      state.bAnyStateSelected = true;
    }
    else if (id >= 1 && id <= stateCount) {
      state.selectedState = static_cast<uint32>(id - 1);
    }
  }
  else if (ed::GetSelectedLinks(&link, 1) == 1) {
    const SIZE_T id = link.Get();
    if (id >= 1 && id <= transitionCount) {
      state.selectedTransition = static_cast<uint32>(id - 1);
    }
  }
}

/*
 * A link dragged from an output pin to an input pin of another state becomes a transition.
 */
void
handleNewLinks(GraphEditorState& state, uint32 stateCount)
{
  if (ed::BeginCreate(kLinkColor, kLinkThickness)) {
    ed::PinId startPin;
    ed::PinId endPin;
    if (ed::QueryNewLink(&startPin, &endPin)) {
      bool bStartOutput = false;
      bool bEndOutput = false;
      uint32 source = 0;
      uint32 target = 0;
      const bool bKnownPins = getPinState(startPin, stateCount, source, bStartOutput) &&
                              getPinState(endPin, stateCount, target, bEndOutput);
      if (!bStartOutput) {
        std::swap(source, target);
        std::swap(bStartOutput, bEndOutput);
      }
      // The Any State has only an output, so a valid target is always a state.
      const bool bValid = bKnownPins && bStartOutput && !bEndOutput && source != target;
      if (!bValid) {
        ed::RejectNewItem(kErrorColor, kLinkThickness);
      }
      else if (ed::AcceptNewItem(kValidColor, kLinkThickness)) {
        addTransition(state, source, target);
      }
    }
  }
  ed::EndCreate();
}

/*
 * The Delete key and the canvas' own deletes are only gathered here; applyDeletes changes
 * the graph once the canvas is done.
 */
void
handleDeletes(GraphEditorState& state, uint32 stateCount, uint32 transitionCount)
{
  if (ed::BeginDelete()) {
    ed::LinkId link;
    while (ed::QueryDeletedLink(&link)) {
      const SIZE_T id = link.Get();
      if (id >= 1 && id <= transitionCount && ed::AcceptDeletedItem()) {
        state.deletedTransitions.push_back(static_cast<uint32>(id - 1));
      }
    }
    ed::NodeId node;
    while (ed::QueryDeletedNode(&node)) {
      const SIZE_T id = node.Get();
      if (id >= 1 && id <= stateCount) {
        if (ed::AcceptDeletedItem()) {
          state.deletedStates.push_back(static_cast<uint32>(id - 1));
        }
      }
      else {
        // The Any State node is part of every layer.
        ed::RejectDeletedItem();
      }
    }
  }
  ed::EndDelete();
}

void
renderCanvasPopups(GraphEditorState& state, const AnimationLayer& layer)
{
  // Popups inside the canvas open at the mouse in canvas units, read before suspending.
  const ImVec2 mouse = ImGui::GetMousePos();
  ed::Suspend();
  if (ed::ShowNodeContextMenu(&state.contextNode)) {
    ImGui::OpenPopup(kNodePopup);
  }
  else if (ed::ShowLinkContextMenu(&state.contextLink)) {
    ImGui::OpenPopup(kLinkPopup);
  }
  else if (ed::ShowBackgroundContextMenu()) {
    state.popupPosition = mouse;
    ImGui::OpenPopup(kAddStatePopup);
  }

  if (ImGui::BeginPopup(kAddStatePopup)) {
    ImGui::TextDisabled("Add State");
    for (uint32 k = 0; k < 3; ++k) {
      if (ImGui::MenuItem(kStateKindNames[k])) {
        addState(state, static_cast<AnimationStateKind>(k), state.popupPosition);
      }
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(kNodePopup)) {
    const SIZE_T id = state.contextNode.Get();
    if (id >= 1 && id <= layer.states.size()) {
      const uint32 index = static_cast<uint32>(id - 1);
      ImGui::TextDisabled("%s", layer.states[index].name.c_str());
      if (ImGui::MenuItem(ICON_FA_PLAY "  Set as Default State", nullptr, false,
                          layer.defaultState != index)) {
        beginChange(state, "Set Default State");
        state.graph.getLayersForEdit()[state.layer].defaultState = index;
      }
      if (ImGui::MenuItem(ICON_FA_TRASH "  Delete", "Del")) {
        state.deletedStates.push_back(index);
      }
    }
    else {
      ImGui::TextDisabled("Any State");
      ImGui::TextUnformatted("Every layer has it; it cannot be deleted.");
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(kLinkPopup)) {
    const SIZE_T id = state.contextLink.Get();
    if (id >= 1 && id <= layer.transitions.size() &&
        ImGui::MenuItem(ICON_FA_TRASH "  Delete", "Del")) {
      state.deletedTransitions.push_back(static_cast<uint32>(id - 1));
    }
    ImGui::EndPopup();
  }
  ed::Resume();
}

void
renderCanvas(GraphEditorState& state)
{
  if (!state.context) {
    ed::Config config;
    // Node places are saved in the graph asset, not in a file of the library's own.
    config.SettingsFile = nullptr;
    state.context = ed::CreateEditor(&config);
  }
  const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
  const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
  state.canvasCenter =
      ImVec2(canvasMin.x + canvasSize.x * 0.5f, canvasMin.y + canvasSize.y * 0.5f);

  ed::SetCurrentEditor(state.context);
  applyCanvasStyle();
  ed::Begin("##GraphCanvas");
  if (state.bClearSelection) {
    state.bClearSelection = false;
    ed::ClearSelection();
  }

  const Vector<AnimationLayer>& layers = state.graph.getLayers();
  if (state.layer < layers.size()) {
    const AnimationLayer& layer = layers[state.layer];
    const AnimationGraphLayerLayout& layout = state.layout.layers[state.layer];
    const uint32 stateCount = static_cast<uint32>(layer.states.size());
    const uint32 transitionCount = static_cast<uint32>(layer.transitions.size());

    const bool bApplyPositions = state.bApplyPositions;
    if (bApplyPositions) {
      state.bApplyPositions = false;
      ed::SetNodePosition(ed::NodeId(kAnyStateNode),
                          ImVec2(layout.anyStatePosition.x, layout.anyStatePosition.y));
      for (uint32 s = 0; s < stateCount; ++s) {
        ed::SetNodePosition(getStateNodeId(s), ImVec2(layout.statePositions[s].x,
                                                      layout.statePositions[s].y));
      }
    }
    renderAnyStateNode();
    for (uint32 s = 0; s < stateCount; ++s) {
      renderStateNode(state, layer, s);
    }
    for (uint32 t = 0; t < transitionCount; ++t) {
      const AnimationTransition& transition = layer.transitions[t];
      const bool bFromAny = transition.source == AnimationGraph::ANY_STATE;
      if ((!bFromAny && transition.source >= stateCount) || transition.target >= stateCount) {
        continue;
      }
      const ed::PinId source =
          bFromAny ? ed::PinId(kAnyStateOutputPin) : getOutputPinId(transition.source);
      ed::Link(getLinkId(t), source, getInputPinId(transition.target), kLinkColor,
               kLinkThickness);
    }

    // Requests from the last frame, now that their nodes and links exist.
    if (state.selectStateRequest < stateCount) {
      ed::SelectNode(getStateNodeId(state.selectStateRequest));
    }
    if (state.selectTransitionRequest < transitionCount) {
      ed::SelectLink(getLinkId(state.selectTransitionRequest));
    }
    state.selectStateRequest = kNone;
    state.selectTransitionRequest = kNone;

    if (state.navigateFrames > 0 && --state.navigateFrames == 0) {
      ed::NavigateToContent(0.0f);
    }
    if (!bApplyPositions && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      readNodePositions(state, stateCount);
    }
    readSelection(state, stateCount, transitionCount);

    // The graph may change below, so nothing after reads layer or layout.
    handleNewLinks(state, stateCount);
    handleDeletes(state, stateCount, transitionCount);
    renderCanvasPopups(state, state.graph.getLayers()[state.layer]);
    if (state.bAddStateAtCenter) {
      state.bAddStateAtCenter = false;
      addState(state, AnimationStateKind::Clip, ed::ScreenToCanvas(state.canvasCenter));
    }
  }

  ed::End();

  // Dropping a clip on the canvas makes a state that plays it, where it was dropped.
  if (state.layer < state.graph.getLayers().size() &&
      ImGui::BeginDragDropTargetCustom(
          ImRect(canvasMin, ImVec2(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y)),
          ImGui::GetID("##CanvasDrop"))) {
    if (SPtr<AnimationClipAsset> clip = AssetDragDrop::acceptAnimation()) {
      if (clip->isLoaded() || AssetManager::instance().syncLoadAsset(clip)) {
        addState(state, AnimationStateKind::Clip, ed::ScreenToCanvas(ImGui::GetMousePos()),
                 clip);
      }
    }
    ImGui::EndDragDropTarget();
  }

  ed::SetCurrentEditor(nullptr);
  applyDeletes(state);
}

/*
 * The models the graph's clips were made for: the skeleton of the first clip that has
 * one, or, for rigid clips, the models in the first clip's folder.
 */
void
collectPreviewModels(GraphEditorState& state)
{
  UUID skeletonId = UUID::null();
  String folder;
  bool bFound = false;
  for (const AnimationGraphClip& clip : state.graph.getClips()) {
    if (!clip.clip) {
      continue;
    }
    if (!bFound) {
      folder = clip.clip->getAssetPath();
      bFound = true;
    }
    if (clip.clip->getClip() && !clip.clip->getClip()->getSkeletonId().isNull()) {
      skeletonId = clip.clip->getClip()->getSkeletonId();
      folder = clip.clip->getAssetPath();
      break;
    }
  }
  state.previewModels.clear();
  if (bFound) {
    AnimationPreviewUI::findModels(skeletonId, folder, state.previewModels);
  }
}

void
setPreviewModel(GraphEditorState& state, const SPtr<IAsset>& asset)
{
  SPtr<ModelAsset> model;
  if (asset && (asset->isLoaded() || AssetManager::instance().syncLoadAsset(asset))) {
    model = asset->as<ModelAsset>();
  }
  state.preview.setModel(model);
}

/*
 * The model saved with the graph, else the first one that fits; once per opened graph.
 */
void
pickPreviewModel(GraphEditorState& state)
{
  if (state.bPreviewModelPicked) {
    return;
  }
  state.bPreviewModelPicked = true;
  collectPreviewModels(state);
  SPtr<IAsset> asset;
  if (!state.layout.previewModel.isNull()) {
    asset = AssetManager::instance().getAsset(state.layout.previewModel);
  }
  if (!asset && !state.previewModels.empty()) {
    asset = state.previewModels.front();
  }
  setPreviewModel(state, asset);
}

/*
 * A choice made by hand is saved with the graph, so it is an edit like any other.
 */
void
choosePreviewModel(GraphEditorState& state, const SPtr<IAsset>& asset)
{
  beginChange(state, "Set Preview Model");
  state.layout.previewModel = asset ? asset->getUUID() : UUID::null();
  setPreviewModel(state, asset);
}

void
applyPreviewValues(GraphEditorState& state)
{
  for (uint32 i = 0; i < state.previewValues.size(); ++i) {
    const PreviewValue& value = state.previewValues[i];
    switch (value.type) {
    case AnimationParameterType::Float:
      CH_PARAMETER_UNUSED(state.instance.setFloat(i, value.value.floatValue));
      break;
    case AnimationParameterType::Int:
      CH_PARAMETER_UNUSED(state.instance.setInt(i, value.value.intValue));
      break;
    case AnimationParameterType::Bool:
      CH_PARAMETER_UNUSED(state.instance.setBool(i, value.value.intValue != 0));
      break;
    case AnimationParameterType::Trigger:
      // Fired by its button only.
      break;
    }
  }
}

void
pushPreviewPose(GraphEditorState& state)
{
  if (ModelComponent* component = state.preview.getModelComponent()) {
    component->setPose(state.instance.getPoseMatrices());
  }
}

/*
 * After an edit or a model change the preview runs a fresh copy of the graph from its
 * default states, keeping the test values of parameters that still exist (by name and
 * type). A graph with errors is not run, so the model stays at rest.
 */
void
syncPreview(GraphEditorState& state)
{
  const ModelComponent* component = state.preview.getModelComponent();
  if (state.previewId == state.id && state.previewComponent == component) {
    return;
  }
  if (state.previewId != state.id) {
    collectPreviewModels(state);
  }
  state.previewId = state.id;
  state.previewComponent = component;

  const Vector<AnimationParameter>& parameters = state.graph.getParameters();
  Vector<PreviewValue> values;
  values.reserve(parameters.size());
  for (const AnimationParameter& parameter : parameters) {
    PreviewValue& value = values.emplace_back(PreviewValue{
        .name = parameter.name, .type = parameter.type, .value = parameter.defaultValue});
    for (const PreviewValue& old : state.previewValues) {
      if (old.type == parameter.type && parameter.type != AnimationParameterType::Trigger &&
          StringUtils::equalsIgnoreCase(old.name, parameter.name)) {
        value.value = old.value;
        break;
      }
    }
  }
  state.previewValues = std::move(values);

  const SPtr<ModelAsset>& model = state.preview.getModel();
  state.instance.setModel(model ? model->getModel() : nullptr);
  state.previewGraph = state.bValid ? chMakeShared<AnimationGraph>(state.graph) : nullptr;
  state.instance.setGraph(state.previewGraph);
  applyPreviewValues(state);
  pushPreviewPose(state);
}

void
advancePreview(GraphEditorState& state)
{
  if (!state.instance.isBound()) {
    return;
  }
  applyPreviewValues(state);
  if (state.instance.advance(ImGui::GetIO().DeltaTime)) {
    pushPreviewPose(state);
  }
}

void
renderPreviewPanel(GraphEditorState& state)
{
  SPtr<IAsset> picked;
  bool bPicked = false;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 2.0f -
                          ImGui::GetStyle().ItemSpacing.x * 2.0f);
  AnimationPreviewUI::renderAssetCombo(ICON_FA_CUBE "##PreviewModel",
                                       state.preview.getModel().get(), "No model",
                                       state.previewModels, picked, bPicked);
  ImGui::SetItemTooltip("Model the graph plays on; drop any model on the view too. Saved "
                        "with the graph.");
  if (bPicked) {
    choosePreviewModel(state, picked);
  }
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_EXPAND "##Frame")) {
    state.preview.requestFrame();
  }
  ImGui::SetItemTooltip("Frame the model (F over the view)");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_COG "##Settings")) {
    ImGui::OpenPopup("PreviewSettings");
  }
  ImGui::SetItemTooltip("Preview settings");
  if (ImGui::BeginPopup("PreviewSettings")) {
    ModelPreview::renderSettings();
    ImGui::EndPopup();
  }

  if (ImGui::Button(ICON_FA_UNDO "  Restart")) {
    state.instance.reset();
    applyPreviewValues(state);
    pushPreviewPose(state);
  }
  ImGui::SetItemTooltip("Every layer back to its default state");
  ImGui::SameLine();
  if (ImGui::Button("Reset Values")) {
    const Vector<AnimationParameter>& parameters = state.graph.getParameters();
    for (uint32 i = 0; i < state.previewValues.size() && i < parameters.size(); ++i) {
      state.previewValues[i].value = parameters[i].defaultValue;
    }
    state.instance.reset();
    applyPreviewValues(state);
    pushPreviewPose(state);
  }
  ImGui::SetItemTooltip("Test values back to the parameters' defaults");

  ImGui::SeparatorText("Parameters (not saved)");
  const bool bTable = !state.previewValues.empty() && ImGui::BeginTable("##Values", 2);
  for (uint32 i = 0; bTable && i < state.previewValues.size(); ++i) {
    PreviewValue& value = state.previewValues[i];
    ImGui::PushID(static_cast<int32>(i));
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(value.name.c_str());
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    switch (value.type) {
    case AnimationParameterType::Float:
      ImGui::DragFloat("##Value", &value.value.floatValue, 0.01f);
      break;
    case AnimationParameterType::Int:
      ImGui::DragInt("##Value", &value.value.intValue);
      break;
    case AnimationParameterType::Bool: {
      bool bValue = value.value.intValue != 0;
      if (ImGui::Checkbox("##Value", &bValue)) {
        value.value.intValue = bValue ? 1 : 0;
      }
      break;
    }
    case AnimationParameterType::Trigger:
      if (ImGui::Button("Fire")) {
        CH_PARAMETER_UNUSED(state.instance.setTrigger(i));
      }
      if (state.instance.isBound() && state.instance.getBool(i)) {
        ImGui::SameLine();
        ImGui::TextColored(kValidColor, "set");
      }
      break;
    }
    ImGui::PopID();
  }
  if (bTable) {
    ImGui::EndTable();
  }
  if (state.previewValues.empty()) {
    ImGui::TextDisabled("No parameters.");
  }

  ImGui::SeparatorText("Running");
  if (!state.instance.isBound()) {
    ImGui::TextDisabled(state.preview.getModel() ? "Not running: the graph has errors."
                                                 : "Not running: no model.");
    return;
  }
  const Vector<AnimationLayer>& layers = state.previewGraph->getLayers();
  for (uint32 l = 0; l < layers.size(); ++l) {
    const uint32 current = state.instance.getCurrentState(l);
    ImGui::Text("%s: %s%s", layers[l].name.c_str(), getStateName(layers[l], current),
                state.instance.isInTransition(l) ? " (fading in)" : "");
  }
}

/*
 * The canvas and the preview share the middle column; dragging the bar between them moves
 * the split, saved when released.
 */
void
renderPreviewSplitter(float& height, float maxHeight)
{
  const float width = ImGui::GetContentRegionAvail().x;
  ImGui::InvisibleButton("##PreviewSplitter", ImVec2(width, kSplitterThickness));
  if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
  }
  if (ImGui::IsItemActive()) {
    height = Math::clamp(height - ImGui::GetIO().MouseDelta.y, kMinPreviewHeight,
                         Math::max(maxHeight, kMinPreviewHeight));
    g_cvarGraphPreviewHeight.set(height, ConsoleVariableSource::Console);
  }
  if (ImGui::IsItemDeactivated()) {
    ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
    EngineConfig::setUserValue("Editor", "Editor", "GraphPreviewHeight",
                               StringUtils::toChars(buffer, height));
  }
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  const float middle = (min.y + max.y) * 0.5f;
  ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, middle), ImVec2(max.x, middle),
                                      ImGui::GetColorU32(ImGui::IsItemActive()
                                                             ? ImGuiCol_SeparatorActive
                                                             : ImGuiCol_Separator));
}

void
renderPreview(GraphEditorState& state, float height)
{
  const ImGuiStyle& style = ImGui::GetStyle();
  const float panelWidth = Math::min(kPreviewPanelWidth, ImGui::GetContentRegionAvail().x * 0.5f);
  const float viewWidth = ImGui::GetContentRegionAvail().x - panelWidth - style.ItemSpacing.x;
  state.preview.renderView(ImVec2(viewWidth, height), "Drop a model here to preview the graph.");
  if (ImGui::BeginDragDropTarget()) {
    if (const SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
      choosePreviewModel(state, model);
    }
    ImGui::EndDragDropTarget();
  }
  if (state.preview.getModel() && !state.instance.isBound()) {
    const ImVec2 min = ImGui::GetItemRectMin();
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(min.x + style.FramePadding.x, min.y + style.FramePadding.y),
        ImGui::GetColorU32(kErrorColor), "Rest pose: the graph has errors.");
  }

  ImGui::SameLine();
  if (ImGui::BeginChild("##PreviewPanel", ImVec2(0.0f, height), ImGuiChildFlags_Borders)) {
    renderPreviewPanel(state);
  }
  ImGui::EndChild();
}

/*
 * The runtime refuses two samples at one place, and so does its triangulation.
 */
NODISCARD bool
hasDistinctSamples(const AnimationState& animationState)
{
  const bool b2D = animationState.kind == AnimationStateKind::BlendSpace2D;
  const Vector<BlendSpaceSample>& samples = animationState.samples;
  for (SIZE_T i = 0; i < samples.size(); ++i) {
    for (SIZE_T j = 0; j < i; ++j) {
      if (samples[i].x == samples[j].x && (!b2D || samples[i].y == samples[j].y)) {
        return false;
      }
    }
  }
  return !samples.empty();
}

NODISCARD float
snapValue(const GraphEditorState& state, float value)
{
  if (!state.bSnap || state.snapStep <= 0.0f) {
    return value;
  }
  return Math::floor(value / state.snapStep + 0.5f) * state.snapStep;
}

/*
 * The samples' range on each axis plus a margin, so points at the ends stay inside; a
 * range of zero gets one unit each way.
 */
void
fitPlotRange(GraphEditorState& state, const AnimationState& animationState)
{
  const Vector<BlendSpaceSample>& samples = animationState.samples;
  for (uint32 axis = 0; axis < 2; ++axis) {
    float minValue = 0.0f;
    float maxValue = 0.0f;
    for (SIZE_T i = 0; i < samples.size(); ++i) {
      const float value = axis == 0 ? samples[i].x : samples[i].y;
      minValue = i == 0 ? value : Math::min(minValue, value);
      maxValue = i == 0 ? value : Math::max(maxValue, value);
    }
    if (maxValue - minValue < kMinPlotRange) {
      minValue -= 1.0f;
      maxValue += 1.0f;
    }
    const float margin = (maxValue - minValue) * kPlotMargin;
    state.plotMin[axis] = minValue - margin;
    state.plotMax[axis] = maxValue + margin;
  }
}

/*
 * Grid lines at every multiple of the snap step, or at a fifth of the range without snap,
 * as long as they stay apart enough to read.
 */
template<typename LineStart, typename LineEnd>
void
drawPlotGrid(ImDrawList& draw, const GraphEditorState& state, uint32 axis, float length,
             LineStart&& lineStart, LineEnd&& lineEnd)
{
  const float range = state.plotMax[axis] - state.plotMin[axis];
  const float step = state.bSnap && state.snapStep > 0.0f ? state.snapStep : range / 5.0f;
  if (step <= 0.0f || length * step / range < kMinGridSpacing) {
    return;
  }
  const ImU32 color = ImGui::GetColorU32(ImGuiCol_Border, 0.6f);
  const ImU32 zeroColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
  for (float value = Math::ceil(state.plotMin[axis] / step) * step;
       value <= state.plotMax[axis]; value += step) {
    const bool bZero = Math::abs(value) < step * 0.5f;
    draw.AddLine(lineStart(value), lineEnd(value), bZero ? zeroColor : color);
  }
}

/*
 * The samples of a blend space on its axes: drag a point to move it, drop a clip to add
 * one, right click a point to change or remove it. Dragging elsewhere moves the test point,
 * whose weights show which clips the runtime blends there.
 */
void
renderBlendSpacePlot(GraphEditorState& state, uint32 index)
{
  const AnimationState& animationState = state.graph.getLayers()[state.layer].states[index];
  const bool b2D = animationState.kind == AnimationStateKind::BlendSpace2D;
  const auto editSamples = [&state, index]() -> Vector<BlendSpaceSample>& {
    return state.graph.getLayersForEdit()[state.layer].states[index].samples;
  };

  ImGui::Checkbox("Snap", &state.bSnap);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
  ImGui::DragFloat("##SnapStep", &state.snapStep, 0.01f, kMinSnapStep, 1000.0f, "%.2f");
  ImGui::SetItemTooltip("Step the samples snap to");

  // An edit still held (a drag here or in the list below) keeps one id, so it rebuilds too.
  if (state.blendSpaceId != state.id || state.blendSpaceLayer != state.layer ||
      state.blendSpaceState != index || state.bEditOpen) {
    state.blendSpaceId = state.id;
    state.blendSpaceLayer = state.layer;
    state.blendSpaceState = index;
    state.bBlendSpaceBuilt = hasDistinctSamples(animationState);
    if (state.bBlendSpaceBuilt) {
      state.blendSpace.build(animationState);
    }
  }
  if (state.draggedSample == kNone) {
    fitPlotRange(state, animationState);
  }

  const float width = Math::max(ImGui::GetContentRegionAvail().x, 1.0f);
  const float height = b2D ? width : ImGui::GetFrameHeight() * 2.5f;
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const ImVec2 corner(origin.x + width, origin.y + height);
  ImGui::InvisibleButton("##BlendSpacePlot", ImVec2(width, height),
                         ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
  const bool bHovered = ImGui::IsItemHovered();

  const float rangeX = state.plotMax[0] - state.plotMin[0];
  const float rangeY = state.plotMax[1] - state.plotMin[1];
  // Y grows upward, like the axes of a chart.
  const auto toScreen = [&](float x, float y) {
    const float u = (x - state.plotMin[0]) / rangeX;
    const float v = b2D ? (y - state.plotMin[1]) / rangeY : 0.5f;
    return ImVec2(origin.x + u * width, origin.y + (1.0f - v) * height);
  };
  const auto toValue = [&](const ImVec2& point, float out[2]) {
    const float u = Math::clamp((point.x - origin.x) / width, 0.0f, 1.0f);
    const float v = Math::clamp(1.0f - (point.y - origin.y) / height, 0.0f, 1.0f);
    out[0] = snapValue(state, state.plotMin[0] + u * rangeX);
    out[1] = b2D ? snapValue(state, state.plotMin[1] + v * rangeY) : 0.0f;
  };

  ImDrawList& draw = *ImGui::GetWindowDrawList();
  draw.AddRectFilled(origin, corner, ImGui::GetColorU32(ImGuiCol_FrameBg));
  drawPlotGrid(
      draw, state, 0, width, [&](float x) { return ImVec2(toScreen(x, 0.0f).x, origin.y); },
      [&](float x) { return ImVec2(toScreen(x, 0.0f).x, corner.y); });
  if (b2D) {
    drawPlotGrid(
        draw, state, 1, height, [&](float y) { return ImVec2(origin.x, toScreen(0.0f, y).y); },
        [&](float y) { return ImVec2(corner.x, toScreen(0.0f, y).y); });
  }
  draw.AddRect(origin, corner, ImGui::GetColorU32(ImGuiCol_Border));

  const Vector<BlendSpaceSample>& samples = animationState.samples;
  const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
  if (b2D && state.bBlendSpaceBuilt) {
    const Vector<uint32>& triangles = state.blendSpace.getTriangles();
    for (SIZE_T t = 0; t + 2 < triangles.size(); t += 3) {
      const BlendSpaceSample& a = samples[triangles[t]];
      const BlendSpaceSample& b = samples[triangles[t + 1]];
      const BlendSpaceSample& c = samples[triangles[t + 2]];
      draw.AddTriangle(toScreen(a.x, a.y), toScreen(b.x, b.y), toScreen(c.x, c.y), lineColor);
    }
  }
  else if (!b2D) {
    draw.AddLine(toScreen(state.plotMin[0], 0.0f), toScreen(state.plotMax[0], 0.0f), lineColor);
  }

  // The sample under the mouse, the closest within reach.
  const float radius = ImGui::GetFontSize() * 0.35f;
  const ImVec2 mouse = ImGui::GetMousePos();
  uint32 hoveredSample = kNone;
  float closest = radius * radius * 4.0f;
  if (bHovered && !state.bDraggingTestPoint) {
    for (uint32 i = 0; i < samples.size(); ++i) {
      const ImVec2 point = toScreen(samples[i].x, samples[i].y);
      const float distance = (point.x - mouse.x) * (point.x - mouse.x) +
                              (point.y - mouse.y) * (point.y - mouse.y);
      if (distance < closest) {
        closest = distance;
        hoveredSample = i;
      }
    }
  }

  state.testPoint[0] = Math::clamp(state.testPoint[0], state.plotMin[0], state.plotMax[0]);
  state.testPoint[1] = Math::clamp(state.testPoint[1], state.plotMin[1], state.plotMax[1]);
  const ImVec2 testScreen = toScreen(state.testPoint[0], state.testPoint[1]);
  BlendSpaceWeights weights;
  if (state.bBlendSpaceBuilt) {
    weights = state.blendSpace.computeWeights(state.testPoint[0], state.testPoint[1]);
  }
  const ImVec4 testColor = kValidColor;
  for (uint32 w = 0; w < weights.count; ++w) {
    const BlendSpaceSample& sample = samples[weights.items[w].sample];
    const float weight = weights.items[w].weight;
    draw.AddLine(testScreen, toScreen(sample.x, sample.y),
                 ImGui::GetColorU32(ImVec4(testColor.x, testColor.y, testColor.z, weight)),
                 1.0f + kWeightLineWidth * weight);
  }

  const ImU32 sampleColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
  const ImU32 hoveredColor = ImGui::GetColorU32(ImGuiCol_Text);
  for (uint32 i = 0; i < samples.size(); ++i) {
    const ImVec2 point = toScreen(samples[i].x, samples[i].y);
    const bool bHighlight = i == hoveredSample || i == state.draggedSample;
    draw.AddCircleFilled(point, radius, bHighlight ? hoveredColor : sampleColor);
    draw.AddCircle(point, radius, ImGui::GetColorU32(ImGuiCol_Border));
  }
  const float cross = radius * 1.5f;
  const ImU32 testU32 = ImGui::GetColorU32(testColor);
  draw.AddLine(ImVec2(testScreen.x - cross, testScreen.y),
               ImVec2(testScreen.x + cross, testScreen.y), testU32, 2.0f);
  draw.AddLine(ImVec2(testScreen.x, testScreen.y - cross),
               ImVec2(testScreen.x, testScreen.y + cross), testU32, 2.0f);
  if (samples.empty()) {
    const ANSICHAR* const hint = "Drop clips here";
    const ImVec2 size = ImGui::CalcTextSize(hint);
    draw.AddText(ImVec2(origin.x + (width - size.x) * 0.5f, origin.y + (height - size.y) * 0.5f),
                 lineColor, hint);
  }

  // Axis ends, inside the plot.
  ANSICHAR text[StringUtils::MAX_FLOAT_CHARS + 2];
  const float padding = ImGui::GetStyle().FramePadding.x;
  const auto drawValue = [&](float value, ImVec2 at, bool bRightAligned) {
    const StringView number = StringUtils::toChars(text, value);
    if (bRightAligned) {
      at.x -= ImGui::CalcTextSize(number.data(), number.data() + number.size()).x;
    }
    draw.AddText(at, lineColor, number.data(), number.data() + number.size());
  };
  const float textHeight = ImGui::GetTextLineHeight();
  drawValue(state.plotMin[0], ImVec2(origin.x + padding, corner.y - textHeight), false);
  drawValue(state.plotMax[0], ImVec2(corner.x - padding, corner.y - textHeight), true);
  if (b2D) {
    drawValue(state.plotMax[1], ImVec2(origin.x + padding, origin.y), false);
  }

  // Input, once everything above has read the samples.
  if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    state.draggedSample = hoveredSample;
    state.bDraggingTestPoint = hoveredSample == kNone;
  }
  if (state.draggedSample != kNone || state.bDraggingTestPoint) {
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      state.draggedSample = kNone;
      state.bDraggingTestPoint = false;
    }
    else {
      float value[2];
      toValue(mouse, value);
      if (state.bDraggingTestPoint) {
        state.testPoint[0] = value[0];
        state.testPoint[1] = value[1];
      }
      else if (state.draggedSample < samples.size() &&
               (samples[state.draggedSample].x != value[0] ||
                samples[state.draggedSample].y != value[1])) {
        editValue(state, "Move Sample");
        BlendSpaceSample& moved = editSamples()[state.draggedSample];
        moved.x = value[0];
        moved.y = value[1];
      }
    }
  }
  if (bHovered && hoveredSample == kNone && state.draggedSample == kNone &&
      !state.bDraggingTestPoint) {
    ImGui::SetTooltip("Test point at %.3g, %.3g", state.testPoint[0], state.testPoint[1]);
  }
  else if (hoveredSample != kNone && state.draggedSample == kNone) {
    ImGui::SetTooltip("%s\nat %.3g, %.3g", getClipName(state.graph, samples[hoveredSample].clip),
                      samples[hoveredSample].x, samples[hoveredSample].y);
  }

  if (bHovered && hoveredSample != kNone && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    state.contextSample = hoveredSample;
    ImGui::OpenPopup("SamplePopup");
  }
  if (ImGui::BeginPopup("SamplePopup")) {
    if (state.contextSample < editSamples().size()) {
      const uint32 sample = state.contextSample;
      if (SPtr<AnimationClipAsset> clip =
              pickClip(state, "Clip", editSamples()[sample].clip)) {
        beginChange(state, "Set Sample Clip");
        const uint32 clipIndex = state.graph.findOrAddClip(clip->getUUID(), clip);
        editSamples()[sample].clip = clipIndex;
      }
      if (ImGui::MenuItem(ICON_FA_TRASH "  Remove Sample")) {
        beginChange(state, "Remove Sample");
        editSamples().erase(editSamples().begin() + sample);
        state.contextSample = kNone;
      }
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<AnimationClipAsset> clip = AssetDragDrop::acceptAnimation()) {
      if (clip->isLoaded() || AssetManager::instance().syncLoadAsset(clip)) {
        float value[2];
        toValue(mouse, value);
        beginChange(state, "Add Sample");
        const uint32 clipIndex = state.graph.findOrAddClip(clip->getUUID(), clip);
        editSamples().push_back({.clip = clipIndex, .x = value[0], .y = value[1]});
      }
    }
    ImGui::EndDragDropTarget();
  }

  // What the runtime blends at the test point.
  if (!state.bBlendSpaceBuilt && !animationState.samples.empty()) {
    ImGui::TextColored(kErrorColor, "Two samples are at the same place.");
  }
  const Vector<BlendSpaceSample>& shownSamples =
      state.graph.getLayers()[state.layer].states[index].samples;
  for (uint32 w = 0; w < weights.count; ++w) {
    const BlendSpaceWeight& item = weights.items[w];
    // A sample removed this frame is gone from the list the weights were computed for.
    if (item.sample >= shownSamples.size()) {
      continue;
    }
    ImGui::TextColored(testColor, "%3.0f%%", item.weight * 100.0f);
    ImGui::SameLine();
    ImGui::TextUnformatted(getClipName(state.graph, shownSamples[item.sample].clip));
  }
}

void
renderStateDetails(GraphEditorState& state, uint32 index)
{
  const AnimationGraph& graph = state.graph;
  const AnimationLayer& layer = graph.getLayers()[state.layer];
  const AnimationState& animationState = layer.states[index];
  const auto editState = [&state, index]() -> AnimationState& {
    return state.graph.getLayersForEdit()[state.layer].states[index];
  };

  ImGui::SeparatorText("State");
  String newName;
  if (inputName(state, "Name", animationState.name, newName)) {
    beginChange(state, "Rename State");
    editState().name = std::move(newName);
  }
  if (index == layer.defaultState) {
    ImGui::TextColored(kDefaultStateColor, ICON_FA_PLAY "  Default state of the layer");
  }
  else if (ImGui::Button(ICON_FA_PLAY "  Set as Default State")) {
    beginChange(state, "Set Default State");
    state.graph.getLayersForEdit()[state.layer].defaultState = index;
  }

  const uint32 kind = static_cast<uint32>(animationState.kind);
  if (ImGui::BeginCombo("Kind", kStateKindNames[Math::min(kind, 2u)])) {
    for (uint32 k = 0; k < 3; ++k) {
      if (ImGui::Selectable(kStateKindNames[k], k == kind) && k != kind) {
        beginChange(state, "Change State Kind");
        editState().kind = static_cast<AnimationStateKind>(k);
      }
    }
    ImGui::EndCombo();
  }

  if (animationState.kind == AnimationStateKind::Clip) {
    if (SPtr<AnimationClipAsset> clip = pickClip(state, "Clip", animationState.clip)) {
      beginChange(state, "Set Clip");
      editState().clip = state.graph.findOrAddClip(clip->getUUID(), clip);
    }
  }
  else {
    const bool b2D = animationState.kind == AnimationStateKind::BlendSpace2D;
    uint32 parameter = 0;
    if (pickParameter(graph, b2D ? "X" : "Parameter", animationState.parameterX, false,
                      isBlendParameter, parameter)) {
      beginChange(state, "Set Blend Parameter");
      editState().parameterX = parameter;
    }
    if (b2D && pickParameter(graph, "Y", animationState.parameterY, false, isBlendParameter,
                             parameter)) {
      beginChange(state, "Set Blend Parameter");
      editState().parameterY = parameter;
    }

    renderBlendSpacePlot(state, index);

    ImGui::SeparatorText("Samples");
    const uint32 sampleCount = static_cast<uint32>(animationState.samples.size());
    for (uint32 i = 0; i < sampleCount; ++i) {
      ImGui::PushID(static_cast<int32>(i));
      const BlendSpaceSample& sample = state.graph.getLayers()[state.layer].states[index].samples[i];
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() -
                              ImGui::GetStyle().ItemSpacing.x);
      if (SPtr<AnimationClipAsset> clip = pickClip(state, "##Clip", sample.clip)) {
        beginChange(state, "Set Sample Clip");
        const uint32 clipIndex = state.graph.findOrAddClip(clip->getUUID(), clip);
        editState().samples[i].clip = clipIndex;
      }
      ImGui::SameLine();
      const bool bRemove = ImGui::SmallButton(ICON_FA_TRASH);
      float position[2] = {sample.x, sample.y};
      const bool bMoved = b2D ? ImGui::DragFloat2("Position", position, 0.01f)
                              : ImGui::DragFloat("Position", &position[0], 0.01f);
      if (bMoved) {
        editValue(state, "Move Sample");
        editState().samples[i].x = position[0];
        editState().samples[i].y = position[1];
      }
      ImGui::PopID();
      if (bRemove) {
        beginChange(state, "Remove Sample");
        editState().samples.erase(editState().samples.begin() + i);
        break;
      }
    }
    if (SPtr<AnimationClipAsset> clip = pickClip(state, "Add Sample", AnimationGraph::NOT_FOUND)) {
      beginChange(state, "Add Sample");
      const uint32 clipIndex = state.graph.findOrAddClip(clip->getUUID(), clip);
      AnimationState& edited = editState();
      // Next to the last sample, so two samples never start at one place.
      const float x = edited.samples.empty() ? 0.0f : edited.samples.back().x + 1.0f;
      edited.samples.push_back({.clip = clipIndex, .x = x, .y = 0.0f});
    }
  }

  ImGui::SeparatorText("Playback");
  float speed = animationState.speed;
  if (ImGui::DragFloat("Speed", &speed, 0.01f)) {
    editValue(state, "Edit State Speed");
    editState().speed = speed;
  }
  uint32 speedParameter = 0;
  if (pickParameter(
          graph, "Speed times", animationState.speedParameter, true,
          [](AnimationParameterType type) { return type == AnimationParameterType::Float; },
          speedParameter)) {
    beginChange(state, "Set Speed Parameter");
    editState().speedParameter = speedParameter;
  }
  bool bLooping = animationState.bLooping;
  if (ImGui::Checkbox("Looping", &bLooping)) {
    beginChange(state, "Edit State Looping");
    editState().bLooping = bLooping;
  }

  ImGui::SeparatorText("Transitions out");
  bool bAny = false;
  for (uint32 t = 0; t < layer.transitions.size(); ++t) {
    if (layer.transitions[t].source != index) {
      continue;
    }
    ImGui::PushID(static_cast<int32>(t));
    if (ImGui::Selectable(StringUtils::format("To {0}", getStateName(layer,
                                                                    layer.transitions[t].target))
                              .c_str())) {
      state.selectTransitionRequest = t;
    }
    ImGui::PopID();
    bAny = true;
  }
  if (!bAny) {
    ImGui::TextDisabled("None. Drag from the right pin to another state.");
  }

  ImGui::Spacing();
  if (ImGui::Button(ICON_FA_TRASH "  Delete State")) {
    state.deletedStates.push_back(index);
  }
}

void
renderCondition(GraphEditorState& state, uint32 transitionIndex, uint32 conditionIndex)
{
  const AnimationGraph& graph = state.graph;
  const AnimationCondition& condition =
      graph.getLayers()[state.layer].transitions[transitionIndex].conditions[conditionIndex];
  const auto editCondition = [&state, transitionIndex, conditionIndex]() -> AnimationCondition& {
    return state.graph.getLayersForEdit()[state.layer]
        .transitions[transitionIndex]
        .conditions[conditionIndex];
  };
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  const bool bKnown = condition.parameter < parameters.size();
  const AnimationParameterType type =
      bKnown ? parameters[condition.parameter].type : AnimationParameterType::Float;

  const float width = ImGui::GetContentRegionAvail().x;
  ImGui::SetNextItemWidth(width * 0.4f);
  uint32 parameter = 0;
  if (pickParameter(graph, "##Parameter", condition.parameter, false,
                    [](AnimationParameterType) { return true; }, parameter)) {
    beginChange(state, "Set Condition Parameter");
    AnimationCondition& edited = editCondition();
    edited.parameter = parameter;
    // A test the new type does not allow goes back to its first one.
    const Span<const AnimationConditionOp> ops = getAllowedOps(parameters[parameter].type);
    if (!Algorithm::contains(ops, edited.op) && !ops.empty()) {
      edited.op = ops[0];
    }
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth(width * 0.25f);
  if (ImGui::BeginCombo("##Op", getOpText(condition.op))) {
    for (const AnimationConditionOp op : getAllowedOps(type)) {
      if (ImGui::Selectable(getOpText(op), op == condition.op) && op != condition.op) {
        beginChange(state, "Set Condition Test");
        editCondition().op = op;
      }
    }
    ImGui::EndCombo();
  }

  const bool bHasThreshold = condition.op != AnimationConditionOp::IsSet &&
                             condition.op != AnimationConditionOp::IsNotSet;
  if (bHasThreshold) {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width * 0.2f);
    AnimationParameterValue threshold = condition.threshold;
    const bool bChanged = type == AnimationParameterType::Float
                              ? ImGui::DragFloat("##Threshold", &threshold.floatValue, 0.01f)
                              : ImGui::DragInt("##Threshold", &threshold.intValue);
    if (bChanged) {
      editValue(state, "Edit Condition Value");
      editCondition().threshold = threshold;
    }
  }
}

void
renderTransitionDetails(GraphEditorState& state, uint32 index)
{
  const AnimationLayer& layer = state.graph.getLayers()[state.layer];
  const AnimationTransition& transition = layer.transitions[index];
  const auto editTransition = [&state, index]() -> AnimationTransition& {
    return state.graph.getLayersForEdit()[state.layer].transitions[index];
  };
  const bool bFromAny = transition.source == AnimationGraph::ANY_STATE;

  ImGui::SeparatorText("Transition");
  ImGui::Text("%s  " ICON_FA_CARET_RIGHT "  %s", getStateName(layer, transition.source),
              getStateName(layer, transition.target));

  float duration = transition.duration;
  if (ImGui::DragFloat("Fade (s)", &duration, 0.01f, 0.0f, 10.0f)) {
    editValue(state, "Edit Transition Fade");
    editTransition().duration = Math::max(duration, 0.0f);
  }

  ImGui::BeginDisabled(bFromAny);
  bool bHasExitTime = transition.bHasExitTime;
  if (ImGui::Checkbox("Exit time", &bHasExitTime)) {
    beginChange(state, "Edit Transition Exit Time");
    editTransition().bHasExitTime = bHasExitTime;
  }
  ImGui::EndDisabled();
  if (bFromAny) {
    ImGui::SetItemTooltip("Any State transitions start on their conditions alone.");
  }
  if (transition.bHasExitTime) {
    ImGui::SameLine();
    float exitTime = transition.exitTime;
    if (ImGui::DragFloat("##ExitTime", &exitTime, 0.01f, 0.0f, 10.0f)) {
      editValue(state, "Edit Transition Exit Time");
      editTransition().exitTime = Math::max(exitTime, 0.0f);
    }
    ImGui::SetItemTooltip("Fraction of the source state's length it waits for");
  }

  bool bInterruptible = transition.bInterruptible;
  if (ImGui::Checkbox("Interruptible", &bInterruptible)) {
    beginChange(state, "Edit Transition");
    editTransition().bInterruptible = bInterruptible;
  }
  ImGui::SetItemTooltip("While it fades, transitions out of its target may start");
  if (bFromAny) {
    bool bToSelf = transition.bCanTransitionToSelf;
    if (ImGui::Checkbox("Can start from its target", &bToSelf)) {
      beginChange(state, "Edit Transition");
      editTransition().bCanTransitionToSelf = bToSelf;
    }
  }
  int32 priority = transition.priority;
  if (ImGui::DragInt("Priority", &priority)) {
    editValue(state, "Edit Transition Priority");
    editTransition().priority = priority;
  }
  ImGui::SetItemTooltip("When several could start, the highest goes first");

  ImGui::SeparatorText("Conditions (all must hold)");
  const uint32 conditionCount = static_cast<uint32>(transition.conditions.size());
  for (uint32 c = 0; c < conditionCount; ++c) {
    ImGui::PushID(static_cast<int32>(c));
    renderCondition(state, index, c);
    ImGui::SameLine();
    const bool bRemove = ImGui::SmallButton(ICON_FA_TRASH);
    ImGui::PopID();
    if (bRemove) {
      beginChange(state, "Remove Condition");
      editTransition().conditions.erase(editTransition().conditions.begin() + c);
      break;
    }
  }
  if (transition.conditions.empty()) {
    ImGui::TextDisabled(bFromAny ? "None: it needs one to start." : "None.");
  }
  const Vector<AnimationParameter>& parameters = state.graph.getParameters();
  ImGui::BeginDisabled(parameters.empty());
  if (ImGui::Button(ICON_FA_PLUS "  Add Condition")) {
    beginChange(state, "Add Condition");
    const Span<const AnimationConditionOp> ops = getAllowedOps(parameters[0].type);
    editTransition().conditions.push_back(
        {.parameter = 0, .op = ops.empty() ? AnimationConditionOp::IsSet : ops[0]});
  }
  ImGui::EndDisabled();
  if (parameters.empty()) {
    ImGui::SetItemTooltip("Add a parameter first");
  }

  ImGui::Spacing();
  if (ImGui::Button(ICON_FA_TRASH "  Delete Transition")) {
    state.deletedTransitions.push_back(index);
  }
}

void
renderLayerDetails(GraphEditorState& state)
{
  const AnimationLayer& layer = state.graph.getLayers()[state.layer];
  const auto editLayer = [&state]() -> AnimationLayer& {
    return state.graph.getLayersForEdit()[state.layer];
  };
  const bool bFirst = state.layer == 0;

  ImGui::SeparatorText("Layer");
  String newName;
  if (inputName(state, "Name", layer.name, newName)) {
    beginChange(state, "Rename Layer");
    editLayer().name = std::move(newName);
  }

  if (bFirst) {
    ImGui::TextDisabled("The first layer moves every node at full weight.");
  }
  else {
    float weight = layer.weight;
    if (ImGui::SliderFloat("Weight", &weight, 0.0f, 1.0f)) {
      editValue(state, "Edit Layer Weight");
      editLayer().weight = weight;
    }

    ImGui::SeparatorText("Mask");
    ImGui::TextDisabled("Bones it moves, each with everything under it.");
    for (uint32 b = 0; b < layer.maskBones.size(); ++b) {
      ImGui::PushID(static_cast<int32>(b));
      ImGui::BulletText("%s", layer.maskBones[b].c_str());
      ImGui::SameLine();
      const bool bRemove = ImGui::SmallButton(ICON_FA_TRASH);
      ImGui::PopID();
      if (bRemove) {
        beginChange(state, "Remove Mask Bone");
        editLayer().maskBones.erase(editLayer().maskBones.begin() + b);
        break;
      }
    }
    if (layer.maskBones.empty()) {
      ImGui::TextDisabled("Empty: the layer moves every node.");
    }
    const bool bEnter = ImGui::InputTextWithHint("##NewBone", "Bone name",
                                                 state.newBoneBuffer, kNameBufferSize,
                                                 ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    const bool bAdd = ImGui::Button(ICON_FA_PLUS "##AddBone") || bEnter;
    if (bAdd && state.newBoneBuffer[0] != '\0') {
      beginChange(state, "Add Mask Bone");
      editLayer().maskBones.emplace_back(state.newBoneBuffer);
      state.newBoneBuffer[0] = '\0';
    }
  }

  ImGui::Spacing();
  ImGui::TextDisabled("Select a state or a transition to edit it.");
  ImGui::Spacing();
  if (ImGui::Button(ICON_FA_TRASH "  Delete Layer")) {
    beginChange(state, "Delete Layer");
    state.graph.removeLayer(state.layer);
    state.layout.layers.erase(state.layout.layers.begin() + state.layer);
    state.layer = 0;
    onStructureChanged(state);
    resetView(state);
  }
}

void
renderDetails(GraphEditorState& state)
{
  const Vector<AnimationLayer>& layers = state.graph.getLayers();
  if (state.layer >= layers.size()) {
    ImGui::TextDisabled("Add a layer with the + next to Layers.");
    return;
  }
  const AnimationLayer& layer = layers[state.layer];
  if (state.selectedState < layer.states.size()) {
    renderStateDetails(state, state.selectedState);
  }
  else if (state.selectedTransition < layer.transitions.size()) {
    renderTransitionDetails(state, state.selectedTransition);
  }
  else if (state.bAnyStateSelected) {
    ImGui::SeparatorText("Any State");
    ImGui::TextWrapped("Its transitions can start from whatever state the layer is in. Drag "
                       "from its pin to a state to add one.");
  }
  else {
    renderLayerDetails(state);
  }
}
} // namespace

/*
 */
void
AnimationGraphEditorUI::render()
{
  GraphEditorState& state = getState();
  renderUnsavedPopup(state);
  renderRemovePopup(state);
  if (!g_cvarShowAnimationGraphEditor.get()) {
    state.bHasFocus = false;
    return;
  }

  if (state.validatedId != state.id) {
    state.validatedId = state.id;
    state.error.clear();
    state.bValid = state.graph.validate(&state.error);
  }
  updateTitle(state);

  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  if (state.bFocus) {
    state.bFocus = false;
    ImGui::SetNextWindowFocus();
  }
  bool bOpen = true;
  state.bHasFocus = false;
  if (ImGui::Begin(state.title.c_str(), &bOpen)) {
    state.bHasFocus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (!state.asset) {
      ImGui::TextDisabled("Double click an animation graph in the Content Browser to edit it.");
    }
    else {
      renderToolbar(state);
      if (ImGui::BeginChild("##lists", ImVec2(kListWidth, 0.0f),
                            ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX)) {
        renderLayerList(state);
        renderParameterList(state);
        renderClipList(state);
      }
      ImGui::EndChild();

      ImGui::SameLine();
      const float canvasWidth =
          Math::max(ImGui::GetContentRegionAvail().x - kDetailsWidth -
                        ImGui::GetStyle().ItemSpacing.x,
                    1.0f);
      if (ImGui::BeginChild("##middle", ImVec2(canvasWidth, 0.0f))) {
        const bool bPreview = g_cvarGraphPreview.get();
        if (bPreview) {
          // Before the canvas, which shows the state the preview is in.
          pickPreviewModel(state);
          syncPreview(state);
          advancePreview(state);
        }
        const float available = ImGui::GetContentRegionAvail().y;
        const float maxPreviewHeight = available - kMinCanvasHeight - kSplitterThickness;
        float previewHeight = Math::clamp(g_cvarGraphPreviewHeight.get(), kMinPreviewHeight,
                                          Math::max(maxPreviewHeight, kMinPreviewHeight));
        const float canvasHeight =
            bPreview ? Math::max(available - previewHeight - kSplitterThickness -
                                     ImGui::GetStyle().ItemSpacing.y * 2.0f,
                                 1.0f)
                     : 0.0f;
        if (ImGui::BeginChild("##canvas", ImVec2(0.0f, canvasHeight))) {
          renderCanvas(state);
        }
        ImGui::EndChild();
        if (bPreview) {
          renderPreviewSplitter(previewHeight, maxPreviewHeight);
          renderPreview(state, previewHeight);
        }
      }
      ImGui::EndChild();

      ImGui::SameLine();
      if (ImGui::BeginChild("##details", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
        renderDetails(state);
      }
      ImGui::EndChild();
      // A button of the details panel asks for these too.
      applyDeletes(state);
    }
  }
  ImGui::End();

  if (!ImGui::IsAnyItemActive()) {
    state.bEditOpen = false;
  }
  if (!bOpen) {
    requestClose(state, AfterClose::HideWindow);
  }
}

/*
 */
void
AnimationGraphEditorUI::update(float deltaTime)
{
  getState().preview.update(deltaTime);
}

/*
 */
void
AnimationGraphEditorUI::recordFrame(ICommandList& commandList)
{
  getState().preview.recordFrame(commandList);
}

/*
 */
bool
AnimationGraphEditorUI::canOpen(const IAsset& asset)
{
  return asset.isTypeOf<AnimationGraphAsset>();
}

/*
 */
void
AnimationGraphEditorUI::open(const SPtr<IAsset>& asset)
{
  if (!asset || !canOpen(*asset)) {
    return;
  }
  if (!asset->isLoaded() && !AssetManager::instance().syncLoadAsset(asset)) {
    CH_LOG_ERROR(AnimationGraphEditorLog, "Failed to load the animation graph {0}.",
                 asset->getName());
    return;
  }

  GraphEditorState& state = getState();
  if (!g_cvarShowAnimationGraphEditor.get()) {
    setVisibleValue(true);
  }
  state.bFocus = true;
  if (state.asset == asset) {
    return;
  }
  state.assetToOpen = std::static_pointer_cast<AnimationGraphAsset>(asset);
  requestClose(state, AfterClose::OpenOther);
}

/*
 */
void
AnimationGraphEditorUI::close(const IAsset& asset)
{
  GraphEditorState& state = getState();
  if (state.assetToOpen.get() == &asset) {
    state.assetToOpen.reset();
    state.afterClose = AfterClose::None;
  }
  if (state.asset.get() == &asset) {
    closeDocument(state);
  }
  // The clip and model lists hold assets too.
  const auto isAsset = [&asset](const SPtr<IAsset>& choice) { return choice.get() == &asset; };
  Algorithm::removeAllIf(state.projectClips, isAsset);
  Algorithm::removeAllIf(state.previewModels, isAsset);
  if (state.preview.getModel().get() == &asset) {
    setPreviewModel(state, nullptr);
  }
}

/*
 */
bool
AnimationGraphEditorUI::askToClose(Function<void()> then)
{
  GraphEditorState& state = getState();
  if (!isDirty(state)) {
    return true;
  }
  state.callback = std::move(then);
  requestClose(state, AfterClose::Callback);
  return false;
}

/*
 */
bool
AnimationGraphEditorUI::hasFocus()
{
  return getState().bHasFocus;
}

/*
 */
void
AnimationGraphEditorUI::save()
{
  CH_PARAMETER_UNUSED(saveDocument(getState()));
}

/*
 */
void
AnimationGraphEditorUI::undo()
{
  GraphEditorState& state = getState();
  applyStep(state, state.undoSteps, state.redoSteps);
}

/*
 */
void
AnimationGraphEditorUI::redo()
{
  GraphEditorState& state = getState();
  applyStep(state, state.redoSteps, state.undoSteps);
}

/*
 */
bool
AnimationGraphEditorUI::isWindowVisible()
{
  return g_cvarShowAnimationGraphEditor.get();
}

/*
 * Hiding asks first about unsaved changes, like the close button.
 */
void
AnimationGraphEditorUI::setWindowVisible(bool bVisible)
{
  if (bVisible) {
    getState().bFocus = true;
    setVisibleValue(true);
  }
  else {
    requestClose(getState(), AfterClose::HideWindow);
  }
}

/*
 */
void
AnimationGraphEditorUI::shutDown()
{
  GraphEditorState& state = getState();
  closeDocument(state);
  state.instance.setModel(nullptr);
  state.preview.shutDown();
  state.assetToOpen.reset();
  state.callback = nullptr;
  if (state.context) {
    ed::DestroyEditor(state.context);
    state.context = nullptr;
  }
}

} // namespace chEngineSDK
