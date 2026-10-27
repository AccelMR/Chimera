/************************************************************************/
/**
 * @file chLevelsUI.cpp
 * @author AccelMR
 * @date 2026/10/26
 * @brief The sublevels of the open map, edited and tried in the editor.
 */
/************************************************************************/
#include "chLevelsUI.h"

#include "chAlgorithm.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chConsoleVariable.h"
#include "chEditorScene.h"
#include "chEngineConfig.h"
#include "chScene.h"
#include "chSceneAsset.h"
#include "chSceneManager.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
namespace {
ConsoleVariable<bool> g_cvarShowLevels("Editor.ShowLevels",
                                       true,
                                       "Shows the Levels window.",
                                       "ShowLevels");

constexpr const ANSICHAR* kMissingScene = "Missing scene";
// When it opens outside the default layout (a layout saved before the window existed).
constexpr float kWindowWidth = 360.0f;
constexpr float kWindowHeight = 240.0f;

/*
 * Puts the next item on this line, against the right edge.
 */
void
sameLineAtRight(const float width)
{
  ImGui::SameLine();
  const float space = ImGui::GetContentRegionAvail().x;
  if (space > width) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space - width);
  }
}

/*
 */
SPtr<SceneAsset>
findSceneAsset(const UUID& id)
{
  const SPtr<IAsset> asset = AssetManager::instance().getAsset(id);
  return asset ? asset->as<SceneAsset>() : nullptr;
}

/*
 */
bool
isListed(const Vector<SceneSubLevel>& subLevels, const UUID& asset)
{
  return Algorithm::containsIf(subLevels, [&asset](const SceneSubLevel& subLevel) {
    return subLevel.asset == asset;
  });
}

/*
 * Adds a scene to the open map's list, loaded at start. The map itself and scenes already
 * listed are refused.
 */
void
addSubLevel(const Scene& scene, const IAsset& asset)
{
  if (asset.getAssetTypeId() != AssetTypeTraits<SceneAsset>::getTypeId() ||
      asset.getUUID() == scene.getLevels()[0].asset ||
      isListed(scene.getSubLevels(), asset.getUUID())) {
    return;
  }
  Vector<SceneSubLevel> subLevels = scene.getSubLevels();
  subLevels.push_back({.asset = asset.getUUID(), .bLoadAtStart = true});
  EditorScene::setSubLevels(subLevels, "Add Level");
}

/*
 * A button that lists every scene that can be added when clicked, and takes scenes dragged
 * from the Content Browser.
 */
void
renderAddButton(const Scene& scene)
{
  if (ImGui::Button(ICON_FA_PLUS "  Add Level (or drop a scene)",
                    ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
    ImGui::OpenPopup("##addLevel");
  }
  if (ImGui::BeginDragDropTarget()) {
    if (const SPtr<IAsset> dropped = AssetDragDrop::acceptAnyAsset()) {
      addSubLevel(scene, *dropped);
    }
    ImGui::EndDragDropTarget();
  }
  if (ImGui::BeginPopup("##addLevel")) {
    // Only while the list is open, so walking every asset costs nothing on other frames.
    Vector<SPtr<IAsset>> scenes;
    AssetManager::instance().getAssetsOfType(AssetTypeTraits<SceneAsset>::getTypeId(), scenes);
    bool bAny = false;
    for (const SPtr<IAsset>& listed : scenes) {
      if (listed->getUUID() == scene.getLevels()[0].asset ||
          isListed(scene.getSubLevels(), listed->getUUID())) {
        continue;
      }
      bAny = true;
      ImGui::PushID(listed.get());
      if (ImGui::Selectable(listed->getName())) {
        addSubLevel(scene, *listed);
      }
      ImGui::SetItemTooltip("%s", listed->getAssetPath());
      ImGui::PopID();
    }
    if (!bAny) {
      ImGui::TextDisabled("No other scene to add");
    }
    ImGui::EndPopup();
  }
}

/*
 * The list as the map saves it: load at start, name, remove.
 */
void
renderEditRows(const Scene& scene)
{
  const Vector<SceneSubLevel>& subLevels = scene.getSubLevels();
  if (subLevels.empty()) {
    ImGui::TextDisabled("The map has no sublevels.");
  }
  // Applied after the loop, which reads the list.
  SIZE_T removed = subLevels.size();
  SIZE_T toggled = subLevels.size();
  for (SIZE_T i = 0; i < subLevels.size(); ++i) {
    ImGui::PushID(static_cast<int32>(i));
    bool bLoadAtStart = subLevels[i].bLoadAtStart;
    if (ImGui::Checkbox("##start", &bLoadAtStart)) {
      toggled = i;
    }
    ImGui::SetItemTooltip("Load with the map when it opens");
    ImGui::SameLine();
    const SPtr<SceneAsset> asset = findSceneAsset(subLevels[i].asset);
    ImGui::TextUnformatted(asset ? asset->getName() : kMissingScene);
    if (asset) {
      ImGui::SetItemTooltip("%s", asset->getAssetPath());
    }
    sameLineAtRight(ImGui::GetFrameHeight());
    if (ImGui::Button(ICON_FA_TRASH_ALT, ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
      removed = i;
    }
    ImGui::SetItemTooltip("Remove from the map's list (the scene stays)");
    ImGui::PopID();
  }

  if (toggled < subLevels.size()) {
    Vector<SceneSubLevel> changed = subLevels;
    changed[toggled].bLoadAtStart = !changed[toggled].bLoadAtStart;
    EditorScene::setSubLevels(changed, "Load Level At Start");
  }
  else if (removed < subLevels.size()) {
    Vector<SceneSubLevel> changed = subLevels;
    changed.erase(changed.begin() + removed);
    EditorScene::setSubLevels(changed, "Remove Level");
  }

  ImGui::Spacing();
  renderAddButton(scene);
}

/*
 * One level during Play: loaded or not, and the button that loads or unloads it when the
 * update ends.
 */
void
renderPlayRow(const Scene& scene, const UUID& assetId, const ANSICHAR* note)
{
  const SPtr<SceneAsset> asset = findSceneAsset(assetId);
  SceneManager& sceneManager = SceneManager::instance();
  const bool bLoaded = scene.findLevel(assetId) != nullptr;
  const bool bLoading = sceneManager.isLevelLoadRequested(assetId);

  ImGui::PushID(&assetId);
  ImGui::TextUnformatted(asset ? asset->getName() : kMissingScene);
  ImGui::SameLine();
  ImGui::TextDisabled("%s%s", bLoaded ? "loaded" : (bLoading ? "loading" : "not loaded"),
                      note);
  if (asset) {
    const ANSICHAR* label = bLoaded ? "Unload" : "Load";
    sameLineAtRight(ImGui::CalcTextSize("Unload").x +
                    ImGui::GetStyle().FramePadding.x * 2.0f);
    ImGui::BeginDisabled(bLoading);
    if (ImGui::SmallButton(label)) {
      if (bLoaded) {
        sceneManager.requestUnloadLevel(assetId);
      }
      else {
        sceneManager.requestLoadLevel(asset);
      }
    }
    ImGui::EndDisabled();
  }
  ImGui::PopID();
}

/*
 * The levels of the running game: those the map lists, then those a script loaded.
 */
void
renderPlayRows(const Scene& scene)
{
  const Vector<SceneSubLevel>& subLevels = scene.getSubLevels();
  for (const SceneSubLevel& subLevel : subLevels) {
    renderPlayRow(scene, subLevel.asset, "");
  }
  const Vector<SceneLevel>& levels = scene.getLevels();
  for (SIZE_T i = 1; i < levels.size(); ++i) {
    if (!isListed(subLevels, levels[i].asset)) {
      renderPlayRow(scene, levels[i].asset, ", by a script");
    }
  }
  if (subLevels.empty() && levels.size() == 1) {
    ImGui::TextDisabled("No sublevels.");
  }
  ImGui::Spacing();
  ImGui::TextDisabled("Changes here end with Play.");
}
} // namespace

/*
 */
void
LevelsUI::render()
{
  if (!g_cvarShowLevels.get()) {
    return;
  }
  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  bool bVisible = true;
  if (ImGui::Begin(ICON_FA_LAYER_GROUP "  Levels###Levels", &bVisible)) {
    const SPtr<Scene>& scene = EditorScene::getScene();
    if (!scene) {
      ImGui::TextDisabled("No scene is open.");
    }
    else if (EditorScene::getPrefab()) {
      ImGui::TextDisabled("Prefabs have no levels.");
    }
    else {
      ImGui::Text(ICON_FA_MAP "  %s", scene->getLevels()[0].name.c_str());
      ImGui::SetItemTooltip("The map: always loaded. Its sublevels are listed under it.");
      ImGui::Separator();
      if (EditorScene::isPlaying()) {
        renderPlayRows(*scene);
      }
      else {
        renderEditRows(*scene);
      }
    }
  }
  ImGui::End();
  if (!bVisible) {
    setWindowVisible(false);
  }
}

/*
 */
bool
LevelsUI::isWindowVisible()
{
  return g_cvarShowLevels.get();
}

/*
 */
void
LevelsUI::setWindowVisible(const bool bVisible)
{
  g_cvarShowLevels.set(bVisible, ConsoleVariableSource::Console);
  EngineConfig::setUserValue("Editor", "Editor", "ShowLevels", bVisible ? "true" : "false");
}

} // namespace chEngineSDK
