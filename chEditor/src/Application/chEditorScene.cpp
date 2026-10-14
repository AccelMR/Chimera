/************************************************************************/
/**
 * @file chEditorScene.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  The scene open in the editor and the requests to make, open and save scenes.
 */
/************************************************************************/
#include "chEditorScene.h"

#include "chAssetManager.h"
#include "chCamera.h"
#include "chConsoleVariable.h"
#include "chEditorCamera.h"
#include "chEditorSelection.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chLightComponent.h"
#include "chLogger.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneAsset.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

#include "imgui.h"

CH_LOG_DECLARE_STATIC(EditorSceneLog, All);

namespace chEngineSDK {
namespace {
ConsoleVariable<String> g_cvarStartupScene(
    "Editor.StartupScene",
    "DefaultScene",
    "Scene asset opened when the editor starts: a path such as /Game/Maps/Main, or a name.",
    "scene");

constexpr const ANSICHAR* kUnsavedPopup = "Unsaved Changes";
constexpr const ANSICHAR* kSaveAsPopup = "Save Scene As";
constexpr StringView kNewSceneName = "NewScene";
// Characters a file name cannot have on some platform.
constexpr StringView kInvalidNameChars = "/\\:*?\"<>|";

enum class PendingAction
{
  None,
  New,
  Open,
  Quit
};

struct EditorSceneState
{
  SPtr<Scene> scene;
  SPtr<SceneAsset> asset;
  EditorCamera* camera = nullptr;
  bool bDirty = false;
  uint32 revision = 0;

  // Waits for the unsaved changes question or the Save As name, then runs in applyPending.
  PendingAction action = PendingAction::None;
  SPtr<SceneAsset> actionAsset;
  bool bActionReady = false;

  bool bOpenUnsavedPopup = false;
  bool bOpenSaveAsPopup = false;
  bool bQuit = false;
  ANSICHAR saveAsName[sizeof(AssetMetadata::name)] = "";
  Event<void(const SPtr<IAsset>&)> onAssetCreated;
};

EditorSceneState&
getState()
{
  static EditorSceneState state;
  return state;
}

/*
 */
void
setDirty(EditorSceneState& state, bool bDirty)
{
  if (state.bDirty != bDirty) {
    state.bDirty = bDirty;
    ++state.revision;
  }
}

/*
 * A new scene starts with a sun, so what is dropped in it is lit.
 */
SPtr<Scene>
makeNewScene()
{
  SceneManager& sceneManager = SceneManager::instance();
  const SPtr<Scene> scene = sceneManager.createAndLoadScene(String(kNewSceneName)).lock();
  if (scene) {
    const SPtr<GameObject> sun = scene->createGameObject("Directional Light");
    sun->getTransform().setLocalRotation(Quaternion(Rotator(-50.0f, 30.0f, 0.0f)));
    sun->addComponent<DirectionalLightComponent>();
  }
  return scene;
}

/*
 * The next start of the editor opens this scene.
 */
void
rememberStartupScene(const SceneAsset& asset)
{
  String path(asset.getAssetPath());
  path += '/';
  path += asset.getName();
  if (!EngineConfig::setUserValue("Editor", "Editor", "StartupScene", path)) {
    CH_LOG_WARNING(EditorSceneLog, "Could not remember {0} as the startup scene.", path);
  }
}

/*
 */
void
applyEditorView(const EditorSceneState& state, const SceneAsset& asset)
{
  Vector3 position;
  Vector3 lookAt;
  if (state.camera && asset.getEditorView(position, lookAt)) {
    state.camera->setView(position, lookAt);
  }
}

/*
 */
void
setCurrent(EditorSceneState& state, const SPtr<Scene>& scene, const SPtr<SceneAsset>& asset)
{
  state.scene = scene;
  state.asset = asset;
  state.bDirty = false;
  ++state.revision;
  if (!scene || !SceneManager::instance().setActiveScene(scene)) {
    CH_LOG_ERROR(EditorSceneLog, "Could not make the scene active.");
  }
  if (asset) {
    applyEditorView(state, *asset);
    rememberStartupScene(*asset);
  }
}

/*
 * Drops the scene and unloads its asset, so changes that were not saved are gone and opening
 * it again reads its file.
 */
void
releaseCurrent(EditorSceneState& state)
{
  EditorSelection::setSelectedGameObject(nullptr);
  if (state.scene) {
    SceneManager::instance().unloadScene(*state.scene);
  }
  if (state.asset) {
    AssetManager::instance().unloadAsset(state.asset);
  }
  state.scene.reset();
  state.asset.reset();
}

/*
 */
bool
saveInto(EditorSceneState& state, const SPtr<SceneAsset>& asset)
{
  if (state.camera) {
    const Camera& camera = state.camera->getCamera();
    asset->setEditorView(camera.getPosition(), camera.getLookAt());
  }
  if (!AssetManager::instance().saveAsset(asset)) {
    CH_LOG_ERROR(EditorSceneLog, "Failed to save scene {0}.", asset->getName());
    return false;
  }
  setDirty(state, false);
  rememberStartupScene(*asset);
  CH_LOG_INFO(EditorSceneLog, "Saved scene {0}.", asset->getName());
  return true;
}

/*
 */
void
saveAsNew(EditorSceneState& state, const Path& folder)
{
  AssetManager& assetManager = AssetManager::instance();
  const SPtr<SceneAsset> asset =
      assetManager.createAsset<SceneAsset>(state.saveAsName, folder).lock();
  if (!asset) {
    CH_LOG_ERROR(EditorSceneLog, "Could not make scene {0} in {1}.", state.saveAsName, folder);
    state.action = PendingAction::None;
    return;
  }

  // The old asset keeps what its file says; the scene now belongs to the new one.
  if (state.asset) {
    assetManager.unloadAsset(state.asset);
  }
  asset->setScene(state.scene);
  state.scene->setName(asset->getName());
  state.asset = asset;
  ++state.revision;
  state.onAssetCreated(asset);

  if (!saveInto(state, asset)) {
    state.action = PendingAction::None;
    return;
  }
  state.bActionReady = state.action != PendingAction::None;
}

/*
 * Runs at once when there is nothing to lose; otherwise asks first.
 */
void
requestAction(PendingAction action, const SPtr<SceneAsset>& asset)
{
  EditorSceneState& state = getState();
  state.action = action;
  state.actionAsset = asset;
  if (state.bDirty) {
    state.bOpenUnsavedPopup = true;
  }
  else {
    state.bActionReady = true;
  }
}

/*
 */
const ANSICHAR*
getActionText(PendingAction action)
{
  switch (action) {
  case PendingAction::New:
    return "making a new scene";
  case PendingAction::Open:
    return "opening another scene";
  case PendingAction::Quit:
    return "closing the editor";
  default:
    return "going on";
  }
}

/*
 */
void
renderUnsavedPopup(EditorSceneState& state)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(kUnsavedPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::Text("Save the changes to %s before %s?", state.scene->getName().c_str(),
              getActionText(state.action));
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

  if (bSave) {
    if (!state.asset) {
      // The action waits for the name.
      state.bOpenSaveAsPopup = true;
    }
    else if (saveInto(state, state.asset)) {
      state.bActionReady = true;
    }
    else {
      state.action = PendingAction::None;
    }
  }
  else if (bDiscard) {
    state.bActionReady = true;
  }
  else if (bCancel) {
    state.action = PendingAction::None;
    state.actionAsset.reset();
  }
}

/*
 * Empty when the name can be used.
 */
const ANSICHAR*
getNameProblem(const ANSICHAR* name, const Path& folder)
{
  const StringView text(name);
  if (StringUtils::trimView(text).empty()) {
    return "Type a name.";
  }
  if (text.find_first_of(kInvalidNameChars) != StringView::npos) {
    return "The name cannot have / \\ : * ? \" < > |";
  }
  if (AssetManager::instance().makeUniqueAssetName(folder, text) != text) {
    return "An asset with this name is already in the folder.";
  }
  return "";
}

/*
 */
void
renderSaveAsPopup(EditorSceneState& state, const Path& folder)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(kSaveAsPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::Text("Into: %s", folder.toString().c_str());
  if (ImGui::IsWindowAppearing()) {
    ImGui::SetKeyboardFocusHere();
  }
  const bool bEnter = ImGui::InputText("Name", state.saveAsName, sizeof(state.saveAsName),
                                       ImGuiInputTextFlags_EnterReturnsTrue);
  const ANSICHAR* problem = getNameProblem(state.saveAsName, folder);
  const bool bValid = problem[0] == '\0';
  if (!bValid) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", problem);
  }

  ImGui::Separator();
  const float buttonWidth = ImGui::GetFontSize() * 7.0f;
  ImGui::BeginDisabled(!bValid);
  const bool bSave = ImGui::Button("Save", ImVec2(buttonWidth, 0.0f)) || (bEnter && bValid);
  ImGui::EndDisabled();
  ImGui::SameLine();
  const bool bCancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) ||
                       ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (bSave || bCancel) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if (bSave) {
    saveAsNew(state, folder);
  }
  else if (bCancel) {
    state.action = PendingAction::None;
    state.actionAsset.reset();
  }
}
} // namespace

/*
 */
void
EditorScene::initialize(EditorCamera& camera)
{
  EditorSceneState& state = getState();
  state.camera = &camera;

  const String& startupScene = g_cvarStartupScene.get();
  const SPtr<SceneAsset> asset = AssetManager::instance().findScene(startupScene);
  const SPtr<Scene> scene = asset ? SceneManager::instance().loadScene(asset).lock() : nullptr;
  if (scene) {
    setCurrent(state, scene, asset);
    CH_LOG_INFO(EditorSceneLog, "Opened scene {0}.", startupScene);
    return;
  }

  CH_LOG_WARNING(EditorSceneLog, "Could not open scene {0}; starting a new one.",
                 startupScene);
  setCurrent(state, makeNewScene(), nullptr);
}

/*
 */
void
EditorScene::shutDown()
{
  EditorSceneState& state = getState();
  state.scene.reset();
  state.asset.reset();
  state.actionAsset.reset();
  state.camera = nullptr;
}

/*
 */
const SPtr<Scene>&
EditorScene::getScene()
{
  return getState().scene;
}

/*
 */
const SPtr<SceneAsset>&
EditorScene::getAsset()
{
  return getState().asset;
}

/*
 */
void
EditorScene::markDirty()
{
  setDirty(getState(), true);
}

/*
 */
bool
EditorScene::isDirty()
{
  return getState().bDirty;
}

/*
 */
uint32
EditorScene::getRevision()
{
  return getState().revision;
}

/*
 */
String
EditorScene::getDisplayName()
{
  const EditorSceneState& state = getState();
  String name = state.scene ? state.scene->getName() : String();
  if (state.bDirty) {
    name += '*';
  }
  return name;
}

/*
 */
void
EditorScene::requestNew()
{
  requestAction(PendingAction::New, nullptr);
}

/*
 */
void
EditorScene::requestOpen(const SPtr<SceneAsset>& asset)
{
  // Opening the scene already open only means something when it has changes to throw away.
  if (!asset || (asset == getState().asset && !getState().bDirty)) {
    return;
  }
  requestAction(PendingAction::Open, asset);
}

/*
 */
void
EditorScene::requestSave()
{
  EditorSceneState& state = getState();
  if (state.asset) {
    saveInto(state, state.asset);
  }
  else {
    state.bOpenSaveAsPopup = true;
  }
}

/*
 */
void
EditorScene::requestSaveAs()
{
  getState().bOpenSaveAsPopup = true;
}

/*
 */
bool
EditorScene::requestQuit()
{
  EditorSceneState& state = getState();
  if (!state.bDirty) {
    state.bQuit = true;
    return true;
  }
  requestAction(PendingAction::Quit, nullptr);
  return false;
}

/*
 */
bool
EditorScene::shouldQuit()
{
  return getState().bQuit;
}

/*
 */
void
EditorScene::render(const Path& folder)
{
  EditorSceneState& state = getState();
  if (state.bOpenUnsavedPopup) {
    ImGui::OpenPopup(kUnsavedPopup);
    state.bOpenUnsavedPopup = false;
  }
  if (state.bOpenSaveAsPopup) {
    const String name = AssetManager::instance().makeUniqueAssetName(
        folder, state.scene ? StringView(state.scene->getName()) : kNewSceneName);
    StringUtils::copyToBuffer(state.saveAsName, name);
    ImGui::OpenPopup(kSaveAsPopup);
    state.bOpenSaveAsPopup = false;
  }

  renderUnsavedPopup(state);
  renderSaveAsPopup(state, folder);
}

/*
 */
void
EditorScene::applyPending()
{
  EditorSceneState& state = getState();
  if (!state.bActionReady) {
    return;
  }
  state.bActionReady = false;
  const PendingAction action = state.action;
  const SPtr<SceneAsset> asset = std::move(state.actionAsset);
  state.action = PendingAction::None;

  switch (action) {
  case PendingAction::New:
    releaseCurrent(state);
    setCurrent(state, makeNewScene(), nullptr);
    break;

  case PendingAction::Open: {
    // Released first, so opening the scene that was open reads its file again.
    releaseCurrent(state);
    const SPtr<Scene> scene = SceneManager::instance().loadScene(asset).lock();
    if (scene) {
      setCurrent(state, scene, asset);
    }
    else {
      CH_LOG_ERROR(EditorSceneLog, "Could not open scene {0}; starting a new one.",
                   asset->getName());
      setCurrent(state, makeNewScene(), nullptr);
    }
  } break;

  case PendingAction::Quit:
    state.bQuit = true;
    break;

  default:
    break;
  }
}

/*
 */
HEvent
EditorScene::onAssetCreated(const Function<void(const SPtr<IAsset>&)>& callback)
{
  return getState().onAssetCreated.connect(callback);
}

} // namespace chEngineSDK
