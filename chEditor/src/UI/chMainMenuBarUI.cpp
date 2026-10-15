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
#include "chEnginePaths.h"
#include "chImportDialog.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPrefabAsset.h"
#include "chRenderSettings.h"
#include "chSceneAsset.h"
#include "chStringUtils.h"
#include "chUIHelpers.h"

#include "imgui.h"

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

CH_LOG_DECLARE_STATIC(MainMenuBarUILog, All);

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
    if (ImGui::BeginMenu("View Mode")) {
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
    if (ImGui::BeginMenu("Render Path")) {
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
    ImGui::Separator(); //--------------------------------------------------------------

    // The scene renderer reads the color every frame.
    ImGui::ColorEdit4("Renderer Color", UIHelpers::rendererColor.toFloatPtr(),
                      ImGuiColorEditFlags_NoInputs);

    ImGui::Separator(); //--------------------------------------------------------------
    if (ImGui::SliderFloat("Font Size", &UIHelpers::baseFontSize, 1.0f, 5.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp)) {
      ImGui::GetStyle().FontScaleMain = UIHelpers::baseFontSize;
    }
    ImGui::Separator(); //--------------------------------------------------------------

    ImGui::MenuItem("Show ImGui Demo Window", nullptr, &UIHelpers::bShowDemoWindow);

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
    if (ImGui::SmallButton("Back to Scene")) {
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

  if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
    EditorScene::requestNew();
  }
  if (ImGui::BeginMenu("Open Scene")) {
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
  if (ImGui::MenuItem(bEditingPrefab ? "Save Prefab" : "Save Scene", "Ctrl+S")) {
    EditorScene::requestSave();
  }
  if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S", false, !bEditingPrefab)) {
    EditorScene::requestSaveAs();
  }
  if (bEditingPrefab && ImGui::MenuItem("Back to Scene")) {
    EditorScene::requestBackToScene();
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Exit")) {
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

  const String undoLabel = EditorScene::canUndo()
                               ? StringUtils::format("Undo {0}", EditorScene::getUndoName())
                               : String("Undo");
  if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, EditorScene::canUndo())) {
    EditorScene::requestUndo();
  }
  const String redoLabel = EditorScene::canRedo()
                               ? StringUtils::format("Redo {0}", EditorScene::getRedoName())
                               : String("Redo");
  if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, EditorScene::canRedo())) {
    EditorScene::requestRedo();
  }
  ImGui::EndMenu();
}

/*
 */
void
MainMenuBarUI::renderImportMenu()
{
#if USING(CH_CODECS)
  if (!ImGui::BeginMenu("Import")) {
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
