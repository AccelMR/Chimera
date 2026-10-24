/************************************************************************/
/**
 * @file chAnimationGraphEditorUI.cpp
 * @author AccelMR
 * @date 2026/10/24
 * @brief Window that edits an animation graph as a node canvas.
 */
/************************************************************************/
#include "chAnimationGraphEditorUI.h"

#include "chAnimationClipAsset.h"
#include "chAnimationGraph.h"
#include "chAnimationGraphAsset.h"
#include "chAssetManager.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chLogger.h"
#include "chMath.h"
#include "chStringUtils.h"

#include "imgui.h"
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

// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 1100.0f;
constexpr float kWindowHeight = 650.0f;
constexpr float kListWidth = 220.0f;
constexpr float kDetailsWidth = 300.0f;

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

// Ids of the canvas: states count from 1, so 0 (invalid for the node editor) is never used.
constexpr SIZE_T kAnyStateNode = 0x40000000u;

constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);
constexpr ImVec4 kValidColor(0.45f, 0.85f, 0.45f, 1.0f);
constexpr ImVec4 kDefaultStateColor(1.0f, 0.75f, 0.3f, 1.0f);
constexpr ImVec4 kLinkColor(0.85f, 0.85f, 0.85f, 1.0f);
constexpr float kLinkThickness = 2.0f;

constexpr const ANSICHAR* kUnsavedPopup = "Unsaved Animation Graph###UnsavedAnimationGraph";

/**
 * What runs once the open graph is closed after the question about its changes.
 */
enum class AfterClose : uint8 { None, HideWindow, OpenOther, Callback };

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

  uint32 layer = 0;
  uint32 selectedState = kNone;
  uint32 selectedTransition = kNone;
  bool bAnyStateSelected = false;

  uint32 validatedId = kNone;
  bool bValid = false;
  String error;

  String title;
  const IAsset* titleAsset = nullptr;
  bool bTitleDirty = false;

  ed::EditorContext* context = nullptr;
  bool bApplyPositions = false;
  bool bClearSelection = false;
  uint32 navigateFrames = 0;
  bool bFocus = false;
  bool bHasFocus = false;

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
  return parameter < parameters.size() ? parameters[parameter].name.c_str()
                                       : "(no parameter)";
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
  state.layer = 0;
  state.validatedId = kNone;
  state.bTitleDirty = true;
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
  state.id = state.savedId = 0;
  state.validatedId = kNone;
  state.bTitleDirty = true;
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
 * Takes the last step of from, keeping the current state in to under the same name.
 */
void
applyStep(GraphEditorState& state, Vector<GraphSnapshot>& from, Vector<GraphSnapshot>& to)
{
  if (from.empty()) {
    return;
  }
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
  state.bApplyPositions = true;
}

NODISCARD bool
saveDocument(GraphEditorState& state)
{
  if (!state.asset) {
    return false;
  }
  state.asset->setGraph(chMakeShared<AnimationGraph>(state.graph));
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
  if (state.bValid) {
    ImGui::TextColored(kValidColor, ICON_FA_CHECK_CIRCLE "  Ready to play");
  }
  else {
    ImGui::TextColored(kErrorColor, ICON_FA_EXCLAMATION_TRIANGLE "  %s (it does not play)",
                       state.error.c_str());
  }
}

void
renderLists(GraphEditorState& state)
{
  const AnimationGraph& graph = state.graph;
  ImGui::SeparatorText(ICON_FA_LAYER_GROUP "  Layers");
  const Vector<AnimationLayer>& layers = graph.getLayers();
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

  ImGui::SeparatorText(ICON_FA_SLIDERS_H "  Parameters");
  for (const AnimationParameter& parameter : graph.getParameters()) {
    ImGui::TextUnformatted(parameter.name.c_str());
    ImGui::SameLine();
    switch (parameter.type) {
    case AnimationParameterType::Float:
      ImGui::TextDisabled("Float %.3g", parameter.defaultValue.floatValue);
      break;
    case AnimationParameterType::Int:
      ImGui::TextDisabled("Int %d", parameter.defaultValue.intValue);
      break;
    case AnimationParameterType::Bool:
      ImGui::TextDisabled("Bool %s", parameter.defaultValue.intValue != 0 ? "true" : "false");
      break;
    default:
      ImGui::TextDisabled("%s", AnimationGraph::getParameterTypeName(parameter.type));
      break;
    }
  }
  if (graph.getParameters().empty()) {
    ImGui::TextDisabled("No parameters.");
  }

  ImGui::SeparatorText(ICON_FA_FILM "  Clips");
  for (uint32 c = 0; c < graph.getClips().size(); ++c) {
    ImGui::TextUnformatted(getClipName(graph, c));
  }
  if (graph.getClips().empty()) {
    ImGui::TextDisabled("No clips.");
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
  ed::BeginNode(getStateNodeId(index));
  ed::BeginPin(getInputPinId(index), ed::PinKind::Input);
  ImGui::TextUnformatted(ICON_FA_CARET_RIGHT);
  ed::EndPin();
  ImGui::SameLine();

  ImGui::BeginGroup();
  if (index == layer.defaultState) {
    ImGui::TextColored(kDefaultStateColor, ICON_FA_PLAY "  %s", animationState.name.c_str());
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
  ImGui::EndGroup();

  ImGui::SameLine();
  ed::BeginPin(getOutputPinId(index), ed::PinKind::Output);
  ImGui::TextUnformatted(ICON_FA_CARET_RIGHT);
  ed::EndPin();
  ed::EndNode();
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
  ed::BeginPin(ed::PinId(kAnyStateNode * 2 + 1), ed::PinKind::Output);
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

void
renderCanvas(GraphEditorState& state)
{
  if (!state.context) {
    ed::Config config;
    // Node places are saved in the graph asset, not in a file of the library's own.
    config.SettingsFile = nullptr;
    state.context = ed::CreateEditor(&config);
  }
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

    if (state.bApplyPositions) {
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
          bFromAny ? ed::PinId(kAnyStateNode * 2 + 1) : getOutputPinId(transition.source);
      ed::Link(getLinkId(t), source, getInputPinId(transition.target), kLinkColor,
               kLinkThickness);
    }

    if (state.navigateFrames > 0 && --state.navigateFrames == 0) {
      ed::NavigateToContent(0.0f);
    }
    if (state.bApplyPositions) {
      state.bApplyPositions = false;
    }
    else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      readNodePositions(state, stateCount);
    }
    readSelection(state, stateCount, transitionCount);
  }

  ed::End();
  ed::SetCurrentEditor(nullptr);
}

void
renderConditions(const AnimationGraph& graph, const AnimationTransition& transition)
{
  if (transition.conditions.empty()) {
    ImGui::TextDisabled("No conditions.");
    return;
  }
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  for (const AnimationCondition& condition : transition.conditions) {
    const ANSICHAR* const name = getParameterName(graph, condition.parameter);
    const ANSICHAR* const op = getOpText(condition.op);
    if (condition.op == AnimationConditionOp::IsSet ||
        condition.op == AnimationConditionOp::IsNotSet) {
      ImGui::BulletText("%s %s", name, op);
    }
    else if (condition.parameter < parameters.size() &&
             parameters[condition.parameter].type == AnimationParameterType::Float) {
      ImGui::BulletText("%s %s %.3g", name, op, condition.threshold.floatValue);
    }
    else {
      ImGui::BulletText("%s %s %d", name, op, condition.threshold.intValue);
    }
  }
}

void
renderStateDetails(const GraphEditorState& state, const AnimationLayer& layer, uint32 index)
{
  const AnimationGraph& graph = state.graph;
  const AnimationState& animationState = layer.states[index];
  ImGui::SeparatorText(animationState.name.c_str());
  if (index == layer.defaultState) {
    ImGui::TextColored(kDefaultStateColor, ICON_FA_PLAY "  Default state of the layer");
  }
  switch (animationState.kind) {
  case AnimationStateKind::Clip:
    ImGui::Text("Clip: %s", getClipName(graph, animationState.clip));
    break;
  case AnimationStateKind::BlendSpace1D:
  case AnimationStateKind::BlendSpace2D: {
    const bool b2D = animationState.kind == AnimationStateKind::BlendSpace2D;
    ImGui::Text("Blend space %s", b2D ? "2D" : "1D");
    ImGui::Text("X: %s", getParameterName(graph, animationState.parameterX));
    if (b2D) {
      ImGui::Text("Y: %s", getParameterName(graph, animationState.parameterY));
    }
    for (const BlendSpaceSample& sample : animationState.samples) {
      if (b2D) {
        ImGui::BulletText("%s at %.3g, %.3g", getClipName(graph, sample.clip), sample.x,
                          sample.y);
      }
      else {
        ImGui::BulletText("%s at %.3g", getClipName(graph, sample.clip), sample.x);
      }
    }
    break;
  }
  }
  ImGui::Text("Speed: %.3g", animationState.speed);
  if (animationState.speedParameter != AnimationGraph::NO_PARAMETER) {
    ImGui::Text("Times: %s", getParameterName(graph, animationState.speedParameter));
  }
  ImGui::Text("Looping: %s", animationState.bLooping ? "yes" : "no");

  ImGui::SeparatorText("Transitions out");
  bool bAny = false;
  for (const AnimationTransition& transition : layer.transitions) {
    if (transition.source == index) {
      ImGui::BulletText("To %s", getStateName(layer, transition.target));
      bAny = true;
    }
  }
  if (!bAny) {
    ImGui::TextDisabled("None.");
  }
}

void
renderTransitionDetails(const GraphEditorState& state,
                        const AnimationLayer& layer,
                        uint32 index)
{
  const AnimationTransition& transition = layer.transitions[index];
  ImGui::SeparatorText("Transition");
  ImGui::Text("%s  " ICON_FA_CARET_RIGHT "  %s", getStateName(layer, transition.source),
              getStateName(layer, transition.target));
  ImGui::Text("Fade: %.3g s", transition.duration);
  if (transition.bHasExitTime) {
    ImGui::Text("Exit time: %.3g", transition.exitTime);
  }
  else {
    ImGui::TextDisabled("No exit time");
  }
  ImGui::Text("Interruptible: %s", transition.bInterruptible ? "yes" : "no");
  if (transition.source == AnimationGraph::ANY_STATE) {
    ImGui::Text("To itself: %s", transition.bCanTransitionToSelf ? "yes" : "no");
  }
  ImGui::Text("Priority: %d", transition.priority);
  ImGui::SeparatorText("Conditions (all must hold)");
  renderConditions(state.graph, transition);
}

void
renderDetails(const GraphEditorState& state)
{
  const Vector<AnimationLayer>& layers = state.graph.getLayers();
  if (state.layer >= layers.size()) {
    return;
  }
  const AnimationLayer& layer = layers[state.layer];
  if (state.selectedState < layer.states.size()) {
    renderStateDetails(state, layer, state.selectedState);
  }
  else if (state.selectedTransition < layer.transitions.size()) {
    renderTransitionDetails(state, layer, state.selectedTransition);
  }
  else if (state.bAnyStateSelected) {
    ImGui::SeparatorText("Any State");
    ImGui::TextWrapped("Its transitions can start from whatever state the layer is in.");
  }
  else {
    ImGui::SeparatorText(layer.name.c_str());
    ImGui::Text("Weight: %.3g", layer.weight);
    if (!layer.maskBones.empty()) {
      ImGui::TextUnformatted("Mask:");
      for (const String& bone : layer.maskBones) {
        ImGui::BulletText("%s", bone.c_str());
      }
    }
    ImGui::Spacing();
    ImGui::TextDisabled("Select a state or a transition.");
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
        renderLists(state);
      }
      ImGui::EndChild();

      ImGui::SameLine();
      const float canvasWidth =
          Math::max(ImGui::GetContentRegionAvail().x - kDetailsWidth -
                        ImGui::GetStyle().ItemSpacing.x,
                    1.0f);
      if (ImGui::BeginChild("##canvas", ImVec2(canvasWidth, 0.0f))) {
        renderCanvas(state);
      }
      ImGui::EndChild();

      ImGui::SameLine();
      if (ImGui::BeginChild("##details", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
        renderDetails(state);
      }
      ImGui::EndChild();
    }
  }
  ImGui::End();

  if (!bOpen) {
    requestClose(state, AfterClose::HideWindow);
  }
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
  state.assetToOpen.reset();
  state.callback = nullptr;
  if (state.context) {
    ed::DestroyEditor(state.context);
    state.context = nullptr;
  }
}

} // namespace chEngineSDK
