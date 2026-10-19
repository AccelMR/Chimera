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

#include "chAlgorithm.h"
#include "chAssetManager.h"
#include "chCamera.h"
#include "chConsoleVariable.h"
#include "chEditorCamera.h"
#include "chEditorSelection.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chGameObjectSerializer.h"
#include "chLightComponent.h"
#include "chSkyLightComponent.h"
#include "chLogger.h"
#include "chModelComponent.h"
#include "chPrefabAsset.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneAsset.h"
#include "chSceneHistory.h"
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
  EditPrefab,
  BackToScene,
  Quit
};

struct EditorSceneState
{
  SPtr<Scene> scene;
  SPtr<SceneAsset> asset;
  // Set in prefab mode, where scene is made only to show and edit prefabRoot.
  SPtr<PrefabAsset> prefab;
  SPtr<GameObject> prefabRoot;
  // The scene prefab mode goes back to; null for a new scene.
  SPtr<SceneAsset> returnAsset;
  EditorCamera* camera = nullptr;
  bool bDirty = false;
  uint32 revision = 0;
  // Of the scene shown; a new scene or prefab mode starts an empty one.
  SceneHistory history;
  bool bUndoRequested = false;
  bool bRedoRequested = false;
  // Asked while panels draw and their trees are walked, so they wait for applyPending.
  bool bDuplicateRequested = false;
  bool bDeleteRequested = false;
  bool bPasteRequested = false;
  bool bCreatePrefabsRequested = false;
  Path prefabFolder;
  bool bReparentRequested = false;
  SPtr<GameObject> reparentParent;
  SPtr<GameObject> reparentSibling;
  bool bReparentAfterSibling = false;
  // What copySelection wrote, read back by every paste.
  Vector<uint8> clipboard;
  // Reused by every request, so they do not allocate once it has grown.
  Vector<SPtr<GameObject>> targets;

  // Waits for the unsaved changes question or the Save As name, then runs in applyPending.
  PendingAction action = PendingAction::None;
  SPtr<SceneAsset> actionAsset;
  SPtr<PrefabAsset> actionPrefab;
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
 */
void
addSun(Scene& scene)
{
  const SPtr<GameObject> sun = scene.createGameObject("Directional Light");
  sun->getTransform().setLocalRotation(Quaternion(Rotator(-50.0f, 30.0f, 0.0f)));
  sun->addComponent<DirectionalLightComponent>();
}

/*
 * A new scene starts with a sun, so what is dropped in it is lit, and a sky light without
 * an image, ready for one to be dropped on it.
 */
SPtr<Scene>
makeNewScene()
{
  SceneManager& sceneManager = SceneManager::instance();
  const SPtr<Scene> scene = sceneManager.createAndLoadScene(String(kNewSceneName)).lock();
  if (scene) {
    addSun(*scene);
    scene->createGameObject("Sky Light")->addComponent<SkyLightComponent>();
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
  state.history.clear();
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
  state.history.clear();
  state.bUndoRequested = false;
  state.bRedoRequested = false;
  if (state.scene) {
    SceneManager::instance().unloadScene(*state.scene);
  }
  if (state.asset) {
    AssetManager::instance().unloadAsset(state.asset);
  }
  state.scene.reset();
  state.asset.reset();
  state.prefab.reset();
  state.prefabRoot.reset();
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
  // An edit still going on is saved as it is now, so it ends here.
  state.history.commit(*state.scene);
  if (!AssetManager::instance().saveAsset(asset)) {
    CH_LOG_ERROR(EditorSceneLog, "Failed to save scene {0}.", asset->getName());
    return false;
  }
  state.history.markSaved();
  setDirty(state, false);
  rememberStartupScene(*asset);
  CH_LOG_INFO(EditorSceneLog, "Saved scene {0}.", asset->getName());
  return true;
}

/*
 */
bool
savePrefab(EditorSceneState& state)
{
  PrefabAsset& prefab = *state.prefab;
  state.history.commit(*state.scene);
  if (!prefab.setContent(*state.prefabRoot) ||
      !AssetManager::instance().saveAsset(state.prefab)) {
    CH_LOG_ERROR(EditorSceneLog, "Failed to save prefab {0}.", prefab.getName());
    return false;
  }
  state.history.markSaved();
  setDirty(state, false);
  CH_LOG_INFO(EditorSceneLog, "Saved prefab {0}.", prefab.getName());
  return true;
}

/*
 * The prefab in prefab mode, else the scene's asset. False when there is nothing to save
 * into yet (a scene never saved) or saving failed.
 */
bool
saveCurrent(EditorSceneState& state)
{
  if (state.prefab) {
    return savePrefab(state);
  }
  return state.asset && saveInto(state, state.asset);
}

/*
 */
void
openSceneOrNew(EditorSceneState& state, const SPtr<SceneAsset>& asset)
{
  const SPtr<Scene> scene = asset ? SceneManager::instance().loadScene(asset).lock() : nullptr;
  if (scene) {
    setCurrent(state, scene, asset);
    return;
  }
  if (asset) {
    CH_LOG_ERROR(EditorSceneLog, "Could not open scene {0}; starting a new one.",
                 asset->getName());
  }
  setCurrent(state, makeNewScene(), nullptr);
}

/*
 * The prefab's content goes into a plain object, so it can be edited, in a scene of its own.
 * The scene also gets a sun that only lights the prefab: it is not under the root, so it is
 * never saved into the prefab.
 */
void
openPrefabMode(EditorSceneState& state, const SPtr<PrefabAsset>& prefab)
{
  const SPtr<SceneAsset> returnAsset = state.prefab ? state.returnAsset : state.asset;
  releaseCurrent(state);

  const String name(prefab->getName());
  const SPtr<Scene> scene = SceneManager::instance().createAndLoadScene(name).lock();
  if (!scene) {
    CH_LOG_ERROR(EditorSceneLog, "Could not open prefab {0}.", name);
    openSceneOrNew(state, returnAsset);
    return;
  }
  addSun(*scene);
  const SPtr<GameObject> root = scene->createGameObject(name);
  if (!GameObjectSerializer::readPrefabContent(*prefab, *root)) {
    CH_LOG_WARNING(EditorSceneLog, "Could not read all of prefab {0}.", name);
  }

  setCurrent(state, scene, nullptr);
  state.prefab = prefab;
  state.prefabRoot = root;
  state.returnAsset = returnAsset;
  ++state.revision;

  scene->updateTransforms();
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (state.camera && root->getHierarchyWorldBounds(bounds)) {
    state.camera->focus(bounds);
  }
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
requestAction(PendingAction action,
              const SPtr<SceneAsset>& asset,
              const SPtr<PrefabAsset>& prefab = nullptr)
{
  EditorSceneState& state = getState();
  state.action = action;
  state.actionAsset = asset;
  state.actionPrefab = prefab;
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
  case PendingAction::EditPrefab:
    return "editing the prefab";
  case PendingAction::BackToScene:
    return "going back to the scene";
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
    if (!state.asset && !state.prefab) {
      // The action waits for the name.
      state.bOpenSaveAsPopup = true;
    }
    else if (saveCurrent(state)) {
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
    state.actionPrefab.reset();
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

/*
 * The scene stays dirty when undo or redo did not land where it was saved.
 */
void
applyHistory(EditorSceneState& state, bool bEditing)
{
  Scene& scene = *state.scene;
  UUID focus = UUID::null();
  bool bApplied = false;
  if (state.bUndoRequested) {
    bApplied = state.history.undo(scene, focus);
  }
  else if (state.bRedoRequested) {
    bApplied = state.history.redo(scene, focus);
  }
  state.bUndoRequested = false;
  state.bRedoRequested = false;

  if (bApplied) {
    // Null when the step removed the object.
    EditorSelection::setSelectedGameObject(scene.findGameObject(focus));
    setDirty(state, !state.history.isAtSavedState());
  }
  else if (!bEditing && state.history.isEntryOpen()) {
    // An edit that changed nothing (a value set back) is dropped and leaves no "*".
    state.history.commit(scene);
    setDirty(state, !state.history.isAtSavedState());
  }
}

/*
 * "Lamp" copies as "Lamp (1)" and "Lamp (1)" as "Lamp (2)", not "Lamp (1) (1)", with the
 * first number no sibling uses.
 */
String
makeDuplicateName(const GameObject& source)
{
  StringView base = source.getName();
  const SIZE_T open = base.rfind(" (");
  if (open != StringView::npos && base.size() > open + 3 && base.back() == ')') {
    bool bNumber = true;
    for (SIZE_T i = open + 2; i + 1 < base.size(); ++i) {
      bNumber = bNumber && base[i] >= '0' && base[i] <= '9';
    }
    if (bNumber) {
      base = base.substr(0, open);
    }
  }

  const GameObject* parent = source.getParent();
  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->getChildren() : source.getScene()->getRootGameObjects();
  for (uint32 number = 1;; ++number) {
    String name = StringUtils::format("{0} ({1})", base, number);
    const bool bTaken =
        Algorithm::containsIf(siblings, [&name](const SPtr<GameObject>& sibling) {
          return sibling->getName() == name;
        });
    if (!bTaken) {
      return name;
    }
  }
}

/*
 * The top-level selected objects of the open scene that test accepts, into state.targets.
 */
template <typename Test>
void
gatherTargets(EditorSceneState& state, Test test)
{
  EditorSelection::getTopLevel(state.targets);
  Algorithm::removeAllIf(state.targets, [&state, &test](const SPtr<GameObject>& object) {
    return object->getScene() != state.scene.get() || !test(*object);
  });
}

/*
 * One object keeps the "<action> <name>" the history gives it.
 */
void
nameGroupEntry(EditorSceneState& state, StringView action, SIZE_T count)
{
  if (count > 1) {
    state.history.setEntryName(StringUtils::format("{0} {1} objects", action, count));
  }
}

/*
 * Moves gameObject right before or after sibling, which shares its parent.
 */
void
placeNextTo(Scene& scene, GameObject& gameObject, const GameObject& sibling, bool bAfter)
{
  const uint32 index = scene.getSiblingIndex(gameObject);
  const uint32 target = scene.getSiblingIndex(sibling);
  // Taking gameObject out first shifts the places after it by one.
  const uint32 shift = index < target ? 1 : 0;
  scene.setSiblingIndex(gameObject, bAfter ? target + 1 - shift : target - shift);
}

/*
 * The copies are selected, so the gizmo moves them off their sources at once.
 */
void
applyDuplicate(EditorSceneState& state)
{
  gatherTargets(state, [](GameObject& object) {
    return EditorScene::canDuplicateOrDelete(object);
  });
  EditorSelection::clear();
  for (const SPtr<GameObject>& source : state.targets) {
    const SPtr<GameObject> copy = GameObjectSerializer::duplicate(*source);
    if (!copy) {
      CH_LOG_ERROR(EditorSceneLog, "Could not duplicate {0}.", source->getName());
      continue;
    }
    copy->setName(makeDuplicateName(*source));
    EditorSelection::add(copy);
    EditorScene::recordCreated(*copy, "Duplicate");
  }
  nameGroupEntry(state, "Duplicate", state.targets.size());
}

/*
 */
void
applyDelete(EditorSceneState& state)
{
  gatherTargets(state, [](GameObject& object) {
    return EditorScene::canDuplicateOrDelete(object);
  });
  for (const SPtr<GameObject>& object : state.targets) {
    EditorScene::recordChange(*object, "Delete");
    state.scene->destroyGameObject(*object);
  }
  nameGroupEntry(state, "Delete", state.targets.size());
  EditorSelection::removeDetached();
}

/*
 * The world placement is kept, so objects stay where they are seen.
 */
void
applyReparent(EditorSceneState& state)
{
  const SPtr<GameObject> newParent = std::move(state.reparentParent);
  const SPtr<GameObject> sibling = std::move(state.reparentSibling);
  Scene& scene = *state.scene;
  if ((newParent && (newParent->getScene() != &scene ||
                     newParent->getOutermostPrefabInstance())) ||
      (sibling && sibling->getParent() != newParent.get())) {
    return;
  }

  gatherTargets(state, [&newParent, &sibling](GameObject& object) {
    if (!EditorScene::canDuplicateOrDelete(object) || &object == sibling.get()) {
      return false;
    }
    // Under itself or one of its children it would leave the tree.
    for (const GameObject* parent = newParent.get(); parent; parent = parent->getParent()) {
      if (parent == &object) {
        return false;
      }
    }
    return true;
  });
  if (state.targets.empty()) {
    return;
  }

  scene.updateTransforms();
  const GameObject* anchor = sibling.get();
  bool bAfter = state.bReparentAfterSibling;
  for (const SPtr<GameObject>& object : state.targets) {
    EditorScene::recordChange(*object, "Reparent");
    if (!scene.setParent(*object, newParent.get())) {
      continue;
    }
    if (anchor) {
      placeNextTo(scene, *object, *anchor, bAfter);
      // The next one goes after this one, so their order stays as it was selected.
      anchor = object.get();
      bAfter = true;
    }
  }
  nameGroupEntry(state, "Reparent", state.targets.size());
}

/*
 */
void
applyCreatePrefabs(EditorSceneState& state)
{
  gatherTargets(state, [](GameObject& object) {
    GameObject* instance = object.getOutermostPrefabInstance();
    return !instance || instance == &object;
  });
  for (const SPtr<GameObject>& object : state.targets) {
    CH_PARAMETER_UNUSED(EditorScene::createPrefab(*object, state.prefabFolder));
  }
}

/*
 * True when gameObject or something below it is an instance of prefab or of a prefab that
 * holds it, which would put prefab inside itself.
 */
bool
holdsPrefab(const GameObject& gameObject, const PrefabAsset& prefab)
{
  if (!gameObject.getPrefab().isNull()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(gameObject.getPrefab());
    const SPtr<PrefabAsset> instanced = asset ? asset->as<PrefabAsset>() : nullptr;
    if (instanced && instanced->dependsOn(prefab.getUUID())) {
      return true;
    }
  }
  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    if (holdsPrefab(*child, prefab)) {
      return true;
    }
  }
  return false;
}

/*
 * As siblings right after the active object, as in Unity. The prefab's root has no
 * siblings in prefab mode, so what is pasted on it goes inside it.
 */
void
applyPaste(EditorSceneState& state)
{
  if (state.clipboard.empty()) {
    return;
  }
  Scene& scene = *state.scene;
  GameObject* parent = state.prefabRoot.get();
  const GameObject* after = nullptr;
  const SPtr<GameObject>& active = EditorSelection::getSelectedGameObject();
  if (active && active->getScene() == &scene && active != state.prefabRoot) {
    GameObject* activeParent = active->getParent();
    const bool bInsidePrefab = !state.prefabRoot || activeParent;
    if (bInsidePrefab && (!activeParent || !activeParent->getOutermostPrefabInstance())) {
      parent = activeParent;
      after = active.get();
    }
  }

  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->getChildren() : scene.getRootGameObjects();
  const SIZE_T firstNew = siblings.size();
  // Only read, so the bytes are never changed through the stream.
  MemoryDataStream stream(state.clipboard.data(), state.clipboard.size(), false);
  const bool bRead = GameObjectSerializer::readCopies(stream, scene, parent);
  state.targets.assign(siblings.begin() + firstNew, siblings.end());

  bool bRefused = false;
  if (state.prefab) {
    for (const SPtr<GameObject>& object : state.targets) {
      bRefused = bRefused || holdsPrefab(*object, *state.prefab);
    }
  }
  if (!bRead || bRefused) {
    for (const SPtr<GameObject>& object : state.targets) {
      scene.destroyGameObject(*object);
    }
    if (bRefused) {
      CH_LOG_WARNING(EditorSceneLog, "A prefab cannot be pasted inside itself.");
    }
    else {
      CH_LOG_ERROR(EditorSceneLog, "Could not paste the copied objects.");
    }
    return;
  }

  EditorSelection::clear();
  for (const SPtr<GameObject>& object : state.targets) {
    if (after) {
      placeNextTo(scene, *object, *after, true);
      after = object.get();
    }
    EditorSelection::add(object);
    EditorScene::recordCreated(*object, "Paste");
  }
  nameGroupEntry(state, "Paste", state.targets.size());
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
  state.prefab.reset();
  state.prefabRoot.reset();
  state.returnAsset.reset();
  state.actionAsset.reset();
  state.actionPrefab.reset();
  state.reparentParent.reset();
  state.reparentSibling.reset();
  state.targets.clear();
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
const SPtr<PrefabAsset>&
EditorScene::getPrefab()
{
  return getState().prefab;
}

/*
 */
const SPtr<GameObject>&
EditorScene::getPrefabRoot()
{
  return getState().prefabRoot;
}

/*
 */
GameObject*
EditorScene::getDefaultParent()
{
  return getState().prefabRoot.get();
}

/*
 */
void
EditorScene::recordChange(GameObject& gameObject, StringView action)
{
  EditorSceneState& state = getState();
  state.history.recordChange(gameObject, action);
  setDirty(state, true);
}

/*
 */
void
EditorScene::recordCreated(GameObject& gameObject, StringView action)
{
  EditorSceneState& state = getState();
  state.history.recordCreated(gameObject, action);
  setDirty(state, true);
}

/*
 */
bool
EditorScene::canUndo()
{
  return getState().history.canUndo();
}

/*
 */
bool
EditorScene::canRedo()
{
  return getState().history.canRedo();
}

/*
 */
StringView
EditorScene::getUndoName()
{
  return getState().history.getUndoName();
}

/*
 */
StringView
EditorScene::getRedoName()
{
  return getState().history.getRedoName();
}

/*
 */
void
EditorScene::requestUndo()
{
  getState().bUndoRequested = true;
}

/*
 */
void
EditorScene::requestRedo()
{
  getState().bRedoRequested = true;
}

/*
 * The prefab's root holds the whole prefab, and what is inside an instance comes from its
 * prefab, so neither has a place for a copy.
 */
bool
EditorScene::canDuplicateOrDelete(GameObject& gameObject)
{
  const GameObject* instance = gameObject.getOutermostPrefabInstance();
  return &gameObject != getState().prefabRoot.get() && (!instance || instance == &gameObject);
}

/*
 */
void
EditorScene::requestDuplicate()
{
  getState().bDuplicateRequested = true;
}

/*
 */
void
EditorScene::requestDelete()
{
  getState().bDeleteRequested = true;
}

/*
 */
void
EditorScene::requestCreatePrefabs(const Path& folder)
{
  EditorSceneState& state = getState();
  state.bCreatePrefabsRequested = true;
  state.prefabFolder = folder;
}

/*
 */
void
EditorScene::requestReparent(const SPtr<GameObject>& newParent,
                             const SPtr<GameObject>& sibling,
                             bool bAfterSibling)
{
  EditorSceneState& state = getState();
  state.bReparentRequested = true;
  state.reparentParent = newParent;
  state.reparentSibling = sibling;
  state.bReparentAfterSibling = bAfterSibling;
}

/*
 */
void
EditorScene::copySelection()
{
  EditorSceneState& state = getState();
  EditorSelection::getTopLevel(state.targets);
  if (state.targets.empty()) {
    return;
  }
  MemoryDataStream stream;
  GameObjectSerializer::write(stream, state.targets);
  const uint8* bytes = stream.getStartPtr();
  state.clipboard.assign(bytes, bytes + stream.size());
  CH_LOG_INFO(EditorSceneLog, "Copied {0} object(s).", state.targets.size());
}

/*
 */
bool
EditorScene::canPaste()
{
  return !getState().clipboard.empty();
}

/*
 */
void
EditorScene::requestPaste()
{
  getState().bPasteRequested = true;
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
  String name = state.prefab ? String("Prefab: ") : String();
  if (state.scene) {
    name += state.scene->getName();
  }
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
  if (state.asset || state.prefab) {
    saveCurrent(state);
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
  EditorSceneState& state = getState();
  if (state.prefab) {
    CH_LOG_WARNING(EditorSceneLog, "A prefab is saved with Save; go back to the scene first.");
    return;
  }
  state.bOpenSaveAsPopup = true;
}

/*
 */
void
EditorScene::requestEditPrefab(const SPtr<PrefabAsset>& prefab)
{
  if (!prefab || (prefab == getState().prefab && !getState().bDirty)) {
    return;
  }
  requestAction(PendingAction::EditPrefab, nullptr, prefab);
}

/*
 */
void
EditorScene::requestBackToScene()
{
  if (getState().prefab) {
    requestAction(PendingAction::BackToScene, nullptr);
  }
}

/*
 */
SPtr<PrefabAsset>
EditorScene::createPrefab(GameObject& gameObject, const Path& folder)
{
  GameObject* instance = gameObject.getOutermostPrefabInstance();
  if (instance && instance != &gameObject) {
    CH_LOG_ERROR(EditorSceneLog, "{0} is part of a prefab instance.", gameObject.getName());
    return nullptr;
  }

  AssetManager& assetManager = AssetManager::instance();
  const String name = assetManager.makeUniqueAssetName(folder, gameObject.getName());
  const SPtr<PrefabAsset> prefab = assetManager.createAsset<PrefabAsset>(name, folder).lock();
  if (!prefab || !prefab->setContent(gameObject) || !assetManager.saveAsset(prefab)) {
    CH_LOG_ERROR(EditorSceneLog, "Could not make prefab {0} in {1}.", name, folder);
    return nullptr;
  }

  // Its content is already what the prefab holds, so nothing has to be read again. Undo only
  // removes the link; the prefab asset stays.
  recordChange(gameObject, "Create Prefab");
  gameObject.setPrefab(prefab->getUUID());
  getState().onAssetCreated(prefab);
  CH_LOG_INFO(EditorSceneLog, "Made prefab {0}.", name);
  return prefab;
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
EditorScene::applyPending(bool bEditing)
{
  EditorSceneState& state = getState();
  if (state.scene) {
    if (state.bReparentRequested) {
      applyReparent(state);
    }
    if (state.bDuplicateRequested) {
      applyDuplicate(state);
    }
    if (state.bPasteRequested) {
      applyPaste(state);
    }
    if (state.bCreatePrefabsRequested) {
      applyCreatePrefabs(state);
    }
    if (state.bDeleteRequested) {
      applyDelete(state);
    }
    state.bReparentRequested = false;
    state.bDuplicateRequested = false;
    state.bPasteRequested = false;
    state.bCreatePrefabsRequested = false;
    state.bDeleteRequested = false;
    // Clear keeps the capacity; holding the objects would keep deleted ones alive.
    state.targets.clear();
    applyHistory(state, bEditing);
    // An undo may have replaced selected objects with new ones.
    EditorSelection::removeDetached();
  }
  if (!state.bActionReady) {
    return;
  }
  state.bActionReady = false;
  const PendingAction action = state.action;
  const SPtr<SceneAsset> asset = std::move(state.actionAsset);
  const SPtr<PrefabAsset> prefab = std::move(state.actionPrefab);
  state.action = PendingAction::None;

  switch (action) {
  case PendingAction::New:
    releaseCurrent(state);
    state.returnAsset.reset();
    setCurrent(state, makeNewScene(), nullptr);
    break;

  case PendingAction::Open:
    // Released first, so opening the scene that was open reads its file again.
    releaseCurrent(state);
    state.returnAsset.reset();
    openSceneOrNew(state, asset);
    break;

  case PendingAction::EditPrefab:
    openPrefabMode(state, prefab);
    break;

  case PendingAction::BackToScene: {
    // Read from its file again, so its instances show the prefab as it was just saved.
    const SPtr<SceneAsset> returnAsset = std::move(state.returnAsset);
    releaseCurrent(state);
    openSceneOrNew(state, returnAsset);
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
