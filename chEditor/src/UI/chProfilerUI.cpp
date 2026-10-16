/************************************************************************/
/**
 * @file chProfilerUI.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Profiler window and stats overlay of the editor.
 */
/************************************************************************/

#include "chProfilerUI.h"

#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chMath.h"
#include "chProfiler.h"
#include "chRenderSettings.h"
#include "chSceneRenderer.h"
#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {
namespace {
ConsoleVariable<bool> g_cvarShowProfiler("Editor.ShowProfiler",
                                         false,
                                         "Shows the Profiler window.",
                                         "ShowProfiler");
ConsoleVariable<bool> g_cvarShowStats("Editor.ShowStats",
                                      false,
                                      "Shows the frame stats in the corner of the viewport.",
                                      "ShowStats");

constexpr float kOverlayMargin = 8.0f;
// The overlay sits in the top right corner under the view cube, where panels do not dock.
constexpr float kOverlayTop = 152.0f;
constexpr float kGraphHeight = 80.0f;
// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 560.0f;
constexpr float kWindowHeight = 720.0f;
// The graph never scales below the time of a 30 fps frame, so a steady frame does not look
// noisy.
constexpr float kMinGraphMilliseconds = 1000.0f / 30.0f;
// Scope tree levels open the first time the window shows.
constexpr uint32 kDefaultOpenDepth = 3;

void
saveVisibility(StringView key, bool bVisible)
{
  EngineConfig::setUserValue("Editor", "Editor", key, bVisible ? "true" : "false");
}

/**
 * Adds or removes a pass in Renderer.DisabledPasses. Only runs on a click, so building the
 * new text is fine.
 */
void
setPassDisabled(StringView passName, bool bDisabled)
{
  String text;
  for (const String& name : StringUtils::splitString(g_cvarDisabledPasses.get(), ',')) {
    const StringView trimmed = StringUtils::trimView(name);
    if (trimmed.empty() || trimmed == passName) {
      continue;
    }
    if (!text.empty()) {
      text += ',';
    }
    text += trimmed;
  }
  if (bDisabled) {
    if (!text.empty()) {
      text += ',';
    }
    text += passName;
  }
  g_cvarDisabledPasses.set(std::move(text), ConsoleVariableSource::Console);
}

void
featureCheckbox(const ANSICHAR* label, ConsoleVariable<bool>& variable)
{
  bool bOn = variable.get();
  if (ImGui::Checkbox(label, &bOn)) {
    variable.set(bOn, ConsoleVariableSource::Console);
  }
  const StringView help = variable.getHelp();
  const StringView name = variable.getName();
  ImGui::SetItemTooltip("%.*s (%.*s)", static_cast<int32>(help.size()), help.data(),
                        static_cast<int32>(name.size()), name.data());
}

/**
 * Switches for parts of the frame, to see what each one adds or costs. They are not saved.
 */
void
renderFeatures()
{
  if (ImGui::BeginTable("##Features", 2)) {
    ImGui::TableNextColumn();
    featureCheckbox("Shadows", g_cvarShadows);
    featureCheckbox("Normal Maps", g_cvarNormalMaps);
    featureCheckbox("Occlusion Maps", g_cvarOcclusionMaps);
    featureCheckbox("Emissive", g_cvarEmissive);
    featureCheckbox("Image Based Lighting", g_cvarImageBasedLighting);
    ImGui::TableNextColumn();
    featureCheckbox("Directional Lights", g_cvarDirectionalLights);
    featureCheckbox("Point Lights", g_cvarPointLights);
    featureCheckbox("Spot Lights", g_cvarSpotLights);
    featureCheckbox("Tonemap Curve", g_cvarTonemap);
    ImGui::EndTable();
  }

  float ambient = g_cvarAmbient.get();
  ImGui::SetNextItemWidth(160.0f);
  if (ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f, "%.3f")) {
    g_cvarAmbient.set(ambient, ConsoleVariableSource::Console);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset All")) {
    for (ConsoleVariable<bool>* variable :
         {&g_cvarShadows, &g_cvarNormalMaps, &g_cvarOcclusionMaps, &g_cvarEmissive,
          &g_cvarImageBasedLighting,
          &g_cvarDirectionalLights, &g_cvarPointLights, &g_cvarSpotLights, &g_cvarTonemap}) {
      variable->set(variable->getDefault(), ConsoleVariableSource::Console);
    }
    g_cvarAmbient.set(g_cvarAmbient.getDefault(), ConsoleVariableSource::Console);
    g_cvarDisabledPasses.set(String(), ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("Turns every feature and pass back on");
}

/**
 * Short form of a count: 950, 12.3K, 4.56M.
 */
void
textCount(uint64 value)
{
  const double count = static_cast<double>(value);
  if (count >= 1000000.0) {
    ImGui::Text("%.2fM", count / 1000000.0);
  }
  else if (count >= 10000.0) {
    ImGui::Text("%.1fK", count / 1000.0);
  }
  else {
    ImGui::Text("%.0f", count);
  }
}

void
textMilliseconds(float milliseconds)
{
  ImGui::Text("%.2f ms", static_cast<double>(milliseconds));
}

float
getFramesPerSecond()
{
  const float milliseconds = Profiler::getAverageFrameMilliseconds();
  return milliseconds > 0.0f ? 1000.0f / milliseconds : 0.0f;
}

void
renderFrameGraph()
{
  const Span<const float> history = Profiler::getFrameHistory();
  float highest = kMinGraphMilliseconds;
  for (const float milliseconds : history) {
    highest = Math::max(highest, milliseconds);
  }

  ImGui::Text("%.1f FPS  %.2f ms   GPU %.2f ms", static_cast<double>(getFramesPerSecond()),
              static_cast<double>(Profiler::getAverageFrameMilliseconds()),
              static_cast<double>(Profiler::getAverageGpuFrameMilliseconds()));
  ImGui::PlotLines("##FrameTimes", history.data(), static_cast<int32>(history.size()),
                   static_cast<int32>(Profiler::getFrameHistoryOffset()), nullptr, 0.0f,
                   highest, ImVec2(-1.0f, kGraphHeight));
  ImGui::SetItemTooltip("Milliseconds of the last %u frames; the top is %.1f ms",
                        Profiler::FRAME_HISTORY_SIZE, static_cast<double>(highest));
}

void
renderScopeTree(const ANSICHAR* tableId, Span<const ProfilerScopeStats> scopes)
{
  constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV |
                                          ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_SizingStretchProp;
  if (!ImGui::BeginTable(tableId, 3, kTableFlags)) {
    return;
  }
  ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch, 3.0f);
  ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthStretch, 0.6f);
  ImGui::TableHeadersRow();

  // The scopes are listed parent first; the children of a closed scope are skipped by
  // their depth.
  uint32 openDepth = 0;
  for (SIZE_T i = 0; i < scopes.size(); ++i) {
    const ProfilerScopeStats& scope = scopes[i];
    if (scope.depth > openDepth) {
      continue;
    }
    while (openDepth > scope.depth) {
      ImGui::TreePop();
      --openDepth;
    }

    const bool bHasChildren = i + 1 < scopes.size() && scopes[i + 1].depth > scope.depth;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (!bHasChildren) {
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    else if (scope.depth < kDefaultOpenDepth) {
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    // Scope names are literals, so their address is a stable ID.
    const bool bOpen = ImGui::TreeNodeEx(static_cast<const void*>(scope.name.data()), flags,
                                         "%.*s", static_cast<int32>(scope.name.size()),
                                         scope.name.data());
    if (bHasChildren && bOpen) {
      ++openDepth;
    }
    ImGui::TableNextColumn();
    textMilliseconds(scope.milliseconds);
    ImGui::TableNextColumn();
    ImGui::Text("%.1f", static_cast<double>(scope.callsPerFrame));
  }
  while (openDepth > 0) {
    ImGui::TreePop();
    --openDepth;
  }
  ImGui::EndTable();
}

/**
 * The averaged GPU time of a scope, or a negative value when there is none.
 */
float
findGpuMilliseconds(StringView name, uint32 depth)
{
  for (const ProfilerScopeStats& scope : Profiler::getGpuScopeStats()) {
    if (scope.depth == depth && scope.name == name) {
      return scope.milliseconds;
    }
  }
  return -1.0f;
}

/**
 * pass is null for the total row. Optional passes get a checkbox that turns them on or off.
 */
void
renderPassRow(StringView name,
              const RenderPassStats* pass,
              float gpuMilliseconds,
              const CommandListStats& commands)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  if (pass != nullptr && pass->bOptional) {
    ImGui::PushID(static_cast<const void*>(name.data()));
    bool bRun = pass->bRan;
    if (ImGui::Checkbox("##Run", &bRun)) {
      setPassDisabled(name, !bRun);
    }
    ImGui::SetItemTooltip("Runs the pass; off, it only clears its outputs");
    ImGui::PopID();
    ImGui::SameLine();
  }
  if (pass != nullptr && !pass->bRan) {
    ImGui::TextDisabled("%.*s", static_cast<int32>(name.size()), name.data());
  }
  else {
    ImGui::Text("%.*s", static_cast<int32>(name.size()), name.data());
  }
  ImGui::TableNextColumn();
  if (gpuMilliseconds >= 0.0f) {
    textMilliseconds(gpuMilliseconds);
  }
  else {
    ImGui::TextDisabled("-");
  }
  ImGui::TableNextColumn();
  textCount(commands.drawCalls);
  ImGui::TableNextColumn();
  textCount(commands.dispatches);
  ImGui::TableNextColumn();
  textCount(commands.triangles);
  ImGui::TableNextColumn();
  textCount(commands.pipelineBinds);
  ImGui::TableNextColumn();
  textCount(commands.barriers);
}

void
renderPassTable(const RenderFrameStats& stats)
{
  ImGui::Text("Visible items %u, lights %u", stats.visibleItems, stats.visibleLights);

  constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV |
                                          ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_SizingStretchProp;
  if (!ImGui::BeginTable("##Passes", 7, kTableFlags)) {
    return;
  }
  ImGui::TableSetupColumn("Pass", ImGuiTableColumnFlags_WidthStretch, 2.0f);
  ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthStretch, 1.2f);
  ImGui::TableSetupColumn("Draws", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Dispatches", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Triangles", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Pipelines", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Barriers", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableHeadersRow();

  // The GPU times of the passes are nested under the whole graph's.
  for (const RenderPassStats& pass : stats.passes) {
    renderPassRow(pass.name, &pass, findGpuMilliseconds(pass.name, 1), pass.commands);
  }
  // Also counts the barriers that end the frame, which belong to no pass.
  renderPassRow("Total", nullptr, findGpuMilliseconds("RenderGraph", 0), stats.commands);
  ImGui::EndTable();
}

void
renderWindow(const RenderFrameStats& stats)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                 viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
                          ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  bool bVisible = true;
  if (!ImGui::Begin("Profiler", &bVisible)) {
    ImGui::End();
    if (!bVisible) {
      ProfilerUI::setWindowVisible(false);
    }
    return;
  }

#if !USING(CH_PROFILER)
  ImGui::TextDisabled("Built without the profiler (CMake option CH_ENABLE_PROFILER).");
#endif // !USING(CH_PROFILER)
  renderFrameGraph();
  if (ImGui::CollapsingHeader("CPU", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderScopeTree("##CpuScopes", Profiler::getScopeStats());
  }
  if (ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderScopeTree("##GpuScopes", Profiler::getGpuScopeStats());
  }
  if (ImGui::CollapsingHeader("Render Passes", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderPassTable(stats);
  }
  if (ImGui::CollapsingHeader("Features", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderFeatures();
  }
  ImGui::End();

  if (!bVisible) {
    ProfilerUI::setWindowVisible(false);
  }
}

void
renderOverlay(const RenderFrameStats& stats)
{
  constexpr ImGuiWindowFlags kFlags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoInputs;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - kOverlayMargin,
                                 viewport->WorkPos.y + kOverlayTop),
                          ImGuiCond_Always, ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowBgAlpha(0.6f);
  if (!ImGui::Begin("##StatsOverlay", nullptr, kFlags)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTable("##Stats", 2, ImGuiTableFlags_SizingFixedFit)) {
    const auto label = [](const ANSICHAR* text) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextDisabled("%s", text);
      ImGui::TableNextColumn();
    };
    label("FPS");
    ImGui::Text("%.1f", static_cast<double>(getFramesPerSecond()));
    label("Frame");
    textMilliseconds(Profiler::getAverageFrameMilliseconds());
    label("GPU");
    textMilliseconds(Profiler::getAverageGpuFrameMilliseconds());
    label("Draws");
    textCount(stats.commands.drawCalls);
    label("Triangles");
    textCount(stats.commands.triangles);
    label("Passes");
    textCount(stats.passes.size());
    label("Items");
    textCount(stats.visibleItems);
    label("Lights");
    textCount(stats.visibleLights);
    ImGui::EndTable();
  }
  ImGui::End();
}
} // namespace

/*
 */
void
ProfilerUI::render(const RenderFrameStats& stats)
{
  const bool bWindow = g_cvarShowProfiler.get();
  const bool bOverlay = g_cvarShowStats.get();
  Profiler::setEnabled(bWindow || bOverlay);

  if (bWindow) {
    renderWindow(stats);
  }
  if (bOverlay) {
    renderOverlay(stats);
  }
}

/*
 */
bool
ProfilerUI::isWindowVisible()
{
  return g_cvarShowProfiler.get();
}

/*
 */
void
ProfilerUI::setWindowVisible(bool bVisible)
{
  g_cvarShowProfiler.set(bVisible, ConsoleVariableSource::Console);
  saveVisibility("ShowProfiler", bVisible);
}

/*
 */
bool
ProfilerUI::isOverlayVisible()
{
  return g_cvarShowStats.get();
}

/*
 */
void
ProfilerUI::setOverlayVisible(bool bVisible)
{
  g_cvarShowStats.set(bVisible, ConsoleVariableSource::Console);
  saveVisibility("ShowStats", bVisible);
}

} // namespace chEngineSDK
