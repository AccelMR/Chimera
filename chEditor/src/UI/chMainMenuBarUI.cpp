/************************************************************************/
/**
 * @file chMainMenuBar.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chMainMenuBarUI.h"

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "chAssetManager.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chEditorTheme.h"
#include "chEnginePaths.h"
#include "chImportDialog.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPrefabAsset.h"
#include "chProfilerUI.h"
#include "chAnimationPreviewUI.h"
#include "chAssetInspectorUI.h"
#include "chRenderSettings.h"
#include "chSceneAsset.h"
#include "chStringUtils.h"
#include "chUIHelpers.h"
#include "chViewportGrid.h"
#include "chViewportTools.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

CH_LOG_DECLARE_STATIC(MainMenuBarUILog, All);

namespace {
// Settings changed here use the Console source, which wins over the command line and config
// files, as a choice made in the editor should. Hovering one shows its help.
void
editSetting(const ANSICHAR* label,
            ConsoleVariable<float>& cvar,
            float speed,
            float min,
            float max,
            const ANSICHAR* format,
            ImGuiSliderFlags flags = ImGuiSliderFlags_None)
{
  float value = cvar.get();
  if (ImGui::DragFloat(label, &value, speed, min, max, format,
                       flags | ImGuiSliderFlags_AlwaysClamp)) {
    cvar.set(value, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
}

void
editSetting(const ANSICHAR* label, ConsoleVariable<int32>& cvar, int32 min, int32 max)
{
  int32 value = cvar.get();
  if (ImGui::SliderInt(label, &value, min, max, "%d", ImGuiSliderFlags_AlwaysClamp)) {
    cvar.set(value, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
}

template<typename T>
void
resetSetting(ConsoleVariable<T>& cvar)
{
  cvar.set(cvar.getDefault(), ConsoleVariableSource::Console);
}

void
renderMapSizeChoice(const ANSICHAR* label, ConsoleVariable<int32>& cvar, int32 maxSize = 4096)
{
  ImGui::TextUnformatted(label);
  constexpr int32 kMapSizes[] = {512, 1024, 2048, 4096};
  constexpr const ANSICHAR* kMapSizeNames[] = {"512", "1024", "2048", "4096"};
  constexpr SIZE_T kMapSizeCount = sizeof(kMapSizes) / sizeof(kMapSizes[0]);
  // Every choice uses the same button labels.
  ImGui::PushID(label);
  for (SIZE_T i = 0; i < kMapSizeCount && kMapSizes[i] <= maxSize; ++i) {
    if (i > 0) {
      ImGui::SameLine();
    }
    if (ImGui::RadioButton(kMapSizeNames[i], cvar.get() == kMapSizes[i])) {
      cvar.set(kMapSizes[i], ConsoleVariableSource::Console);
    }
    ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
  }
  ImGui::PopID();
}

void
renderShadowSettings()
{
  bool bShadows = g_cvarShadows.get();
  if (ImGui::Checkbox("Enabled", &bShadows)) {
    g_cvarShadows.set(bShadows, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", g_cvarShadows.getHelp().c_str());

  ImGui::SeparatorText("Directional Light");
  // The same texels cover the whole distance, so it depends on the scale of the scene.
  editSetting("Distance", g_cvarShadowDistance, 0.5f, 1.0f, 10000.0f, "%.1f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Cascades", g_cvarShadowCascades, 1, 4);
  editSetting("Split Lambda", g_cvarShadowSplitLambda, 0.005f, 0.0f, 1.0f, "%.2f");
  editSetting("Cascade Blend", g_cvarShadowCascadeBlend, 0.005f, 0.0f, 0.5f, "%.2f");
  editSetting("Fade", g_cvarShadowFade, 0.005f, 0.0f, 1.0f, "%.2f");
  editSetting("Filter Radius", g_cvarShadowFilterRadius, 0, 3);
  editSetting("Normal Offset", g_cvarShadowNormalOffset, 0.05f, 0.0f, 10.0f, "%.2f texels");
  editSetting("Depth Bias", g_cvarShadowDepthBias, 0.05f, 0.0f, 20.0f, "%.2f");
  editSetting("Slope Bias", g_cvarShadowSlopeBias, 0.05f, 0.0f, 20.0f, "%.2f");

  renderMapSizeChoice("Map size per cascade", g_cvarShadowMapSize);

  ImGui::SeparatorText("Spot Lights");
  editSetting("Max Lights", g_cvarSpotShadowMaxLights, 0, 16);
  editSetting("Spot Distance", g_cvarSpotShadowDistance, 0.5f, 1.0f, 10000.0f, "%.1f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Spot Fade", g_cvarSpotShadowFade, 0.005f, 0.0f, 1.0f, "%.2f");
  editSetting("Near Plane", g_cvarSpotShadowNearPlane, 0.005f, 0.001f, 10.0f, "%.3f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Spot Filter Radius", g_cvarSpotShadowFilterRadius, 0, 3);
  editSetting("Spot Normal Offset", g_cvarSpotShadowNormalOffset, 0.05f, 0.0f, 10.0f,
              "%.2f texels");
  editSetting("Spot Depth Bias", g_cvarSpotShadowDepthBias, 0.05f, 0.0f, 20.0f, "%.2f");
  editSetting("Spot Slope Bias", g_cvarSpotShadowSlopeBias, 0.05f, 0.0f, 20.0f, "%.2f");
  renderMapSizeChoice("Map size per spot light", g_cvarSpotShadowMapSize);

  ImGui::SeparatorText("Point Lights");
  editSetting("Max Point Lights", g_cvarPointShadowMaxLights, 0, 8);
  editSetting("Point Distance", g_cvarPointShadowDistance, 0.5f, 1.0f, 10000.0f, "%.1f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Point Fade", g_cvarPointShadowFade, 0.005f, 0.0f, 1.0f, "%.2f");
  editSetting("Point Near Plane", g_cvarPointShadowNearPlane, 0.005f, 0.001f, 10.0f, "%.3f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Point Filter Radius", g_cvarPointShadowFilterRadius, 0, 3);
  editSetting("Point Normal Offset", g_cvarPointShadowNormalOffset, 0.05f, 0.0f, 10.0f,
              "%.2f texels");
  editSetting("Point Depth Bias", g_cvarPointShadowDepthBias, 0.05f, 0.0f, 20.0f, "%.2f");
  editSetting("Point Slope Bias", g_cvarPointShadowSlopeBias, 0.05f, 0.0f, 20.0f, "%.2f");
  renderMapSizeChoice("Map size per cube face", g_cvarPointShadowMapSize, 2048);

  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_UNDO_ALT "  Reset to Defaults")) {
    resetSetting(g_cvarSpotShadowMaxLights);
    resetSetting(g_cvarSpotShadowDistance);
    resetSetting(g_cvarSpotShadowFade);
    resetSetting(g_cvarSpotShadowNearPlane);
    resetSetting(g_cvarSpotShadowFilterRadius);
    resetSetting(g_cvarSpotShadowNormalOffset);
    resetSetting(g_cvarSpotShadowDepthBias);
    resetSetting(g_cvarSpotShadowSlopeBias);
    resetSetting(g_cvarSpotShadowMapSize);
    resetSetting(g_cvarPointShadowMaxLights);
    resetSetting(g_cvarPointShadowDistance);
    resetSetting(g_cvarPointShadowFade);
    resetSetting(g_cvarPointShadowNearPlane);
    resetSetting(g_cvarPointShadowFilterRadius);
    resetSetting(g_cvarPointShadowNormalOffset);
    resetSetting(g_cvarPointShadowDepthBias);
    resetSetting(g_cvarPointShadowSlopeBias);
    resetSetting(g_cvarPointShadowMapSize);
    resetSetting(g_cvarShadows);
    resetSetting(g_cvarShadowDistance);
    resetSetting(g_cvarShadowCascades);
    resetSetting(g_cvarShadowSplitLambda);
    resetSetting(g_cvarShadowCascadeBlend);
    resetSetting(g_cvarShadowFade);
    resetSetting(g_cvarShadowFilterRadius);
    resetSetting(g_cvarShadowNormalOffset);
    resetSetting(g_cvarShadowDepthBias);
    resetSetting(g_cvarShadowSlopeBias);
    resetSetting(g_cvarShadowMapSize);
  }
}

/*
 * Every power of two from minSize to maxSize, as radio buttons.
 */
void
renderPowerOfTwoChoice(const ANSICHAR* label,
                       ConsoleVariable<int32>& cvar,
                       int32 minSize,
                       int32 maxSize)
{
  ImGui::TextUnformatted(label);
  ImGui::PushID(label);
  for (int32 size = minSize; size <= maxSize; size *= 2) {
    if (size > minSize) {
      ImGui::SameLine();
    }
    // One more for the '\0' ImGui needs, which toChars does not write.
    ANSICHAR text[StringUtils::MAX_INTEGER_CHARS + 1];
    text[StringUtils::toChars(text, size).size()] = '\0';
    ImGui::PushID(size);
    if (ImGui::RadioButton(text, cvar.get() == size)) {
      cvar.set(size, ConsoleVariableSource::Console);
    }
    ImGui::PopID();
    ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
  }
  ImGui::PopID();
}

/*
 * The cubes and the table are built again after a change, which takes a few milliseconds,
 * so dragging a slider rebuilds them on every frame of the drag.
 */
void
renderImageBasedLightingSettings()
{
  bool bEnabled = g_cvarImageBasedLighting.get();
  if (ImGui::Checkbox("Enabled##IBL", &bEnabled)) {
    g_cvarImageBasedLighting.set(bEnabled, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", g_cvarImageBasedLighting.getHelp().c_str());
  editSetting("Ambient (without sky light)", g_cvarAmbient, 0.001f, 0.0f, 1.0f, "%.3f");

  ImGui::SeparatorText("Environment");
  renderPowerOfTwoChoice("Cube size", g_cvarIBLCubeSize, 128, 2048);
  ImGui::SeparatorText("Diffuse");
  renderPowerOfTwoChoice("Irradiance size", g_cvarIBLIrradianceSize, 8, 128);
  editSetting("Irradiance Samples", g_cvarIBLIrradianceSamples, 16, 4096);
  ImGui::SeparatorText("Reflections");
  renderPowerOfTwoChoice("Prefiltered size", g_cvarIBLPrefilterSize, 32, 1024);
  editSetting("Prefiltered Mips", g_cvarIBLPrefilterMips, 2, 10);
  editSetting("Prefiltered Samples", g_cvarIBLPrefilterSamples, 16, 4096);
  ImGui::SeparatorText("Lookup Table");
  renderPowerOfTwoChoice("Table size", g_cvarIBLBrdfLutSize, 32, 1024);
  editSetting("Table Samples", g_cvarIBLBrdfLutSamples, 16, 8192);

  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_UNDO_ALT "  Reset to Defaults##IBL")) {
    resetSetting(g_cvarImageBasedLighting);
    resetSetting(g_cvarAmbient);
    resetSetting(g_cvarIBLCubeSize);
    resetSetting(g_cvarIBLIrradianceSize);
    resetSetting(g_cvarIBLIrradianceSamples);
    resetSetting(g_cvarIBLPrefilterSize);
    resetSetting(g_cvarIBLPrefilterMips);
    resetSetting(g_cvarIBLPrefilterSamples);
    resetSetting(g_cvarIBLBrdfLutSize);
    resetSetting(g_cvarIBLBrdfLutSamples);
  }
}

void
editSetting(const ANSICHAR* label, ConsoleVariable<bool>& cvar)
{
  bool bValue = cvar.get();
  if (ImGui::Checkbox(label, &bValue)) {
    cvar.set(bValue, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
}

void
renderAmbientOcclusionSettings()
{
  editSetting("Screen Space AO", g_cvarSSAO);
  editSetting("Half Resolution", g_cvarSSAOHalfResolution);
  // The radius is in world units, so it depends on the scale of the scene.
  editSetting("Radius", g_cvarSSAORadius, 0.01f, 0.01f, 100.0f, "%.2f",
              ImGuiSliderFlags_Logarithmic);
  editSetting("Falloff", g_cvarSSAOFalloff, 0.005f, 0.01f, 1.0f, "%.2f");
  editSetting("Power", g_cvarSSAOPower, 0.01f, 0.5f, 4.0f, "%.2f");
  editSetting("Slices", g_cvarSSAOSlices, 1, 8);
  editSetting("Steps", g_cvarSSAOSteps, 1, 16);
  editSetting("Max Pixel Radius", g_cvarSSAOMaxPixelRadius, 1.0f, 16.0f, 1024.0f, "%.0f px",
              ImGuiSliderFlags_Logarithmic);
  ImGui::SeparatorText("Denoise");
  editSetting("Blur Radius", g_cvarSSAOBlurRadius, 0, 6);
  editSetting("Depth Tolerance", g_cvarSSAODepthTolerance, 0.001f, 0.001f, 1.0f, "%.3f",
              ImGuiSliderFlags_Logarithmic);
  ImGui::Separator();
  editSetting("Multi Bounce", g_cvarAOMultiBounce);

  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_UNDO_ALT "  Reset to Defaults##AO")) {
    resetSetting(g_cvarSSAO);
    resetSetting(g_cvarSSAOHalfResolution);
    resetSetting(g_cvarSSAORadius);
    resetSetting(g_cvarSSAOFalloff);
    resetSetting(g_cvarSSAOPower);
    resetSetting(g_cvarSSAOSlices);
    resetSetting(g_cvarSSAOSteps);
    resetSetting(g_cvarSSAOMaxPixelRadius);
    resetSetting(g_cvarSSAOBlurRadius);
    resetSetting(g_cvarSSAODepthTolerance);
    resetSetting(g_cvarAOMultiBounce);
  }
}
} // namespace

/*
 */
void
MainMenuBarUI::renderMainMenuBar() {
  // ImGui only wants EndMainMenuBar after a BeginMainMenuBar that returned true.
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  renderFileMenu();
  renderEditMenu();

  if (ImGui::BeginMenu("Render")) {
    if (ImGui::BeginMenu(ICON_FA_EYE "  View Mode")) {
      const Optional<ViewMode> current =
          RenderSettingsUtils::viewModeFromName(g_cvarViewMode.get());
      for (uint32 i = 0; i < static_cast<uint32>(ViewMode::COUNT); ++i) {
        const ViewMode mode = static_cast<ViewMode>(i);
        const StringView name = RenderSettingsUtils::getName(mode);
        if (ImGui::MenuItem(name.data(), nullptr, current == mode)) {
          // Console wins over the command line and config files, as a choice made in the
          // editor should.
          g_cvarViewMode.set(String(name), ConsoleVariableSource::Console);
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_ROUTE "  Render Path")) {
      const Optional<RenderPathType> current =
          RenderSettingsUtils::renderPathFromName(g_cvarRenderPath.get());
      for (uint32 i = 0; i < static_cast<uint32>(RenderPathType::COUNT); ++i) {
        const RenderPathType path = static_cast<RenderPathType>(i);
        const StringView name = RenderSettingsUtils::getName(path);
        if (ImGui::MenuItem(name.data(), nullptr, current == path)) {
          g_cvarRenderPath.set(String(name), ConsoleVariableSource::Console);
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_MOON "  Shadows")) {
      renderShadowSettings();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_GLOBE_AMERICAS "  Image Based Lighting")) {
      renderImageBasedLightingSettings();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_ADJUST "  Ambient Occlusion")) {
      renderAmbientOcclusionSettings();
      ImGui::EndMenu();
    }
    ImGui::Separator(); //--------------------------------------------------------------

    // The scene renderer reads the color every frame.
    ImGui::ColorEdit4(ICON_FA_FILL_DRIP "  Renderer Color",
                      UIHelpers::rendererColor.toFloatPtr(), ImGuiColorEditFlags_NoInputs);

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    EditorTheme::renderSettings();
    ImGui::Separator(); //--------------------------------------------------------------
    if (ImGui::MenuItem(ICON_FA_FILE_ALT "  Asset Inspector", nullptr,
                        AssetInspectorUI::isWindowVisible())) {
      AssetInspectorUI::setWindowVisible(!AssetInspectorUI::isWindowVisible());
    }
    if (ImGui::MenuItem(ICON_FA_RUNNING "  Animation Preview", nullptr,
                        AnimationPreviewUI::isWindowVisible())) {
      AnimationPreviewUI::setWindowVisible(!AnimationPreviewUI::isWindowVisible());
    }
    if (ImGui::MenuItem(ICON_FA_CHART_LINE "  Profiler", nullptr,
                        ProfilerUI::isWindowVisible())) {
      ProfilerUI::setWindowVisible(!ProfilerUI::isWindowVisible());
    }
    if (ImGui::MenuItem(ICON_FA_TACHOMETER_ALT "  Stats Overlay", nullptr,
                        ProfilerUI::isOverlayVisible())) {
      ProfilerUI::setOverlayVisible(!ProfilerUI::isOverlayVisible());
    }
    if (ImGui::BeginMenu(ICON_FA_TH "  Grid")) {
      ViewportGrid::renderSettings();
      ImGui::EndMenu();
    }
#if USING(CH_DEBUG_DRAW)
    if (ImGui::MenuItem(ICON_FA_VECTOR_SQUARE "  Selection Bounds", nullptr,
                        ViewportTools::areBoundsVisible())) {
      ViewportTools::setBoundsVisible(!ViewportTools::areBoundsVisible());
    }
    if (ImGui::MenuItem(ICON_FA_BONE "  Selection Skeleton", nullptr,
                        ViewportTools::areSkeletonsVisible())) {
      ViewportTools::setSkeletonsVisible(!ViewportTools::areSkeletonsVisible());
    }
    bool bDebugLines = g_cvarDebugLines.get();
    if (ImGui::MenuItem(ICON_FA_DRAW_POLYGON "  Debug Lines", nullptr, &bDebugLines)) {
      g_cvarDebugLines.set(bDebugLines, ConsoleVariableSource::Console);
    }
    ImGui::SetItemTooltip("%s", g_cvarDebugLines.getHelp().c_str());
#endif // USING(CH_DEBUG_DRAW)
    ImGui::MenuItem(ICON_FA_QUESTION_CIRCLE "  Show ImGui Demo Window", nullptr,
                    &UIHelpers::bShowDemoWindow);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Asset")) {
    renderImportMenu();
    ImGui::EndMenu();
  }

  // Prefab mode looks like a scene, so the bar says what is being edited.
  if (const SPtr<PrefabAsset>& prefab = EditorScene::getPrefab()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.45f, 0.7f, 1.0f, 1.0f), "Editing prefab %s",
                       prefab->getName());
    if (ImGui::SmallButton(ICON_FA_ARROW_LEFT "  Back to Scene")) {
      EditorScene::requestBackToScene();
    }
  }
  ImGui::EndMainMenuBar();
}

/*
 */
void
MainMenuBarUI::renderFileMenu()
{
  if (!ImGui::BeginMenu("File")) {
    return;
  }

  if (ImGui::MenuItem(ICON_FA_FILE "  New Scene", "Ctrl+N")) {
    EditorScene::requestNew();
  }
  if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN "  Open Scene")) {
    // Listed only while the menu is open, so the walk over every asset is rare.
    const SPtr<SceneAsset>& current = EditorScene::getAsset();
    bool bAny = false;
    for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
      if (!asset->isTypeOf<SceneAsset>()) {
        continue;
      }
      bAny = true;
      ImGui::PushID(asset.get());
      if (ImGui::MenuItem(asset->getName(), asset->getAssetPath(), asset == current)) {
        EditorScene::requestOpen(std::static_pointer_cast<SceneAsset>(asset));
      }
      ImGui::PopID();
    }
    if (!bAny) {
      ImGui::TextDisabled("No scenes saved yet");
    }
    ImGui::EndMenu();
  }
  ImGui::Separator();
  const bool bEditingPrefab = EditorScene::getPrefab() != nullptr;
  const ANSICHAR* saveLabel =
      bEditingPrefab ? ICON_FA_SAVE "  Save Prefab" : ICON_FA_SAVE "  Save Scene";
  if (ImGui::MenuItem(saveLabel, "Ctrl+S")) {
    EditorScene::requestSave();
  }
  if (ImGui::MenuItem(ICON_FA_SAVE "  Save Scene As...", "Ctrl+Shift+S", false,
                      !bEditingPrefab)) {
    EditorScene::requestSaveAs();
  }
  if (bEditingPrefab && ImGui::MenuItem(ICON_FA_ARROW_LEFT "  Back to Scene")) {
    EditorScene::requestBackToScene();
  }
  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_SIGN_OUT_ALT "  Exit")) {
    // The editor closes once the unsaved changes are saved or discarded.
    CH_PARAMETER_UNUSED(EditorScene::requestQuit());
  }
  ImGui::EndMenu();
}

/*
 * The labels are only built while the menu is open.
 */
void
MainMenuBarUI::renderEditMenu()
{
  if (!ImGui::BeginMenu("Edit")) {
    return;
  }

  const String undoLabel =
      EditorScene::canUndo()
          ? StringUtils::format(ICON_FA_UNDO "  Undo {0}", EditorScene::getUndoName())
          : String(ICON_FA_UNDO "  Undo");
  if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, EditorScene::canUndo())) {
    EditorScene::requestUndo();
  }
  const String redoLabel =
      EditorScene::canRedo()
          ? StringUtils::format(ICON_FA_REDO "  Redo {0}", EditorScene::getRedoName())
          : String(ICON_FA_REDO "  Redo");
  if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, EditorScene::canRedo())) {
    EditorScene::requestRedo();
  }

  ImGui::Separator();
  const bool bSelection = !EditorSelection::getSelection().empty();
  if (ImGui::MenuItem(ICON_FA_COPY "  Copy", "Ctrl+C", false, bSelection)) {
    EditorScene::copySelection();
  }
  if (ImGui::MenuItem(ICON_FA_PASTE "  Paste", "Ctrl+V", false, EditorScene::canPaste())) {
    EditorScene::requestPaste();
  }
  if (ImGui::MenuItem(ICON_FA_CLONE "  Duplicate", "Ctrl+D", false, bSelection)) {
    EditorScene::requestDuplicate();
  }
  if (ImGui::MenuItem(ICON_FA_TRASH_ALT "  Delete", "Del", false, bSelection)) {
    EditorScene::requestDelete();
  }
  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_CHECK_DOUBLE "  Select All", "Ctrl+A", false,
                      EditorScene::getScene() != nullptr)) {
    EditorSelection::selectAll(*EditorScene::getScene(), EditorScene::getPrefabRoot().get());
  }
  ImGui::EndMenu();
}

/*
 */
void
MainMenuBarUI::renderImportMenu()
{
#if USING(CH_CODECS)
  if (!ImGui::BeginMenu(ICON_FA_FILE_IMPORT "  Import")) {
    return;
  }

  AssetManager& assetManager = AssetManager::instance();
  for (const auto& codec : AssetCodecManager::instance().getAllCodecs()) {
    for (const auto& assetType : codec->getSupportedAssetTypes()) {
      const String& typeName = assetManager.getAssetTypeName(assetType);
      if (ImGui::MenuItem(typeName.c_str())) {
        ImportDialog::open(*codec, EnginePaths::getGameAssetDirectory());
      }
    }
  }

  ImGui::EndMenu();
#endif // USING(CH_CODECS)
}

} // namespace chEngineSDK
