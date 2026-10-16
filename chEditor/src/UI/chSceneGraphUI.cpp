/************************************************************************/
/**
 * @file chSceneGraphUI.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Scene graph UI class for the Chimera Editor.
 */
/************************************************************************/

#include "chSceneGraphUI.h"

#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chContentAssetUI.h"
#include "chEditorScene.h"
#include "chEditorCamera.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLightComponent.h"
#include "chSkyLightComponent.h"
#include "chMath.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
#include "chPrefabAsset.h"
#include "chScene.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {

namespace {
/**
 * True when ancestor is gameObject or one of its parents, so deleting ancestor removes
 * gameObject too.
 */
bool
isInSubtree(const GameObject* gameObject, const GameObject& ancestor)
{
  for (; gameObject; gameObject = gameObject->getParent()) {
    if (gameObject == &ancestor) {
      return true;
    }
  }
  return false;
}

/*
 * The new light goes under parent (a root object when null) and is selected, so the
 * Inspector edits it at once.
 */
template <typename LightComponentType>
void
createLight(Scene& scene, const ANSICHAR* name, GameObject* parent)
{
  const SPtr<GameObject> gameObject = scene.createGameObject(name, parent);
  gameObject->addComponent<LightComponentType>();
  EditorSelection::setSelectedGameObject(gameObject);
  EditorScene::recordCreated(*gameObject, "Create");
}

/*
 */
void
renderCreateLightItems(Scene& scene, GameObject* parent)
{
  if (!ImGui::BeginMenu("Create Light")) {
    return;
  }
  if (ImGui::MenuItem("Directional Light")) {
    createLight<DirectionalLightComponent>(scene, "Directional Light", parent);
  }
  if (ImGui::MenuItem("Point Light")) {
    createLight<PointLightComponent>(scene, "Point Light", parent);
  }
  if (ImGui::MenuItem("Spot Light")) {
    createLight<SpotLightComponent>(scene, "Spot Light", parent);
  }
  if (ImGui::MenuItem("Sky Light")) {
    createLight<SkyLightComponent>(scene, "Sky Light", parent);
  }
  ImGui::EndMenu();
}
} // namespace

/*
 */
void
SceneGraphUI::renderSceneGraphUI()
{
  const SPtr<Scene> scene = SceneManager::instance().getActiveScene().lock();
  const ANSICHAR* sceneName = scene ? scene->getName().c_str() : "No Active Scene";
  if (m_windowTitle.empty() || m_titleSceneName != sceneName) {
    m_titleSceneName = sceneName;
    // "###" keeps the window ID fixed, so ImGui keeps its position and docking when the
    // scene changes.
    m_windowTitle = StringUtils::format("Scene Graph - {0}###SceneGraph", m_titleSceneName);
  }

  ImGui::Begin(m_windowTitle.c_str(), &m_isVisible);
  if (scene) {
    // In prefab mode only the prefab is shown; the light that shows it is not part of it.
    if (const SPtr<GameObject>& prefabRoot = EditorScene::getPrefabRoot()) {
      ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
      renderGameObject(*scene, prefabRoot);
    }
    else {
      for (const SPtr<GameObject>& root : scene->getRootGameObjects()) {
        renderGameObject(*scene, root);
      }
    }
    renderEmptyAreaContextMenu(*scene);
    renderEmptyAreaDropTarget();
    applyPendingChanges(*scene);
  }
  ImGui::End();
}

/*
 */
void
SceneGraphUI::applyPendingChanges(Scene& scene)
{
  if (m_pendingModel || m_pendingPrefab) {
    addPendingObject(scene);
  }
  if (m_pendingExpand) {
    EditorScene::recordChange(*m_pendingExpand, "Expand");
    if (ModelExpand::expand(scene, *m_pendingExpand, m_pendingExpandMode)) {
      m_openOnNextDraw = m_pendingExpand.get();
    }
    m_pendingExpand.reset();
  }
  if (m_pendingCreatePrefab && m_contentBrowser) {
    CH_PARAMETER_UNUSED(EditorScene::createPrefab(*m_pendingCreatePrefab,
                                                  m_contentBrowser->getCurrentFolderPath()));
    m_pendingCreatePrefab.reset();
  }
  if (m_pendingDelete) {
    const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
    if (isInSubtree(selected.get(), *m_pendingDelete)) {
      EditorSelection::setSelectedGameObject(nullptr);
    }
    EditorScene::recordChange(*m_pendingDelete, "Delete");
    scene.destroyGameObject(*m_pendingDelete);
    m_pendingDelete.reset();
  }
}

/*
 */
void
SceneGraphUI::renderGameObject(Scene& scene, const SPtr<GameObject>& gameObject)
{
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth;
  if (gameObject->getChildren().empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (EditorSelection::getSelectedGameObject() == gameObject) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  // What is below an instance comes from its prefab, so it cannot be changed here: the
  // instance shows in blue and its content in gray, as in Unity.
  const GameObject* instance = gameObject->getOutermostPrefabInstance();
  const bool bInstanceRoot = instance == gameObject.get();
  const bool bInsideInstance = instance && !bInstanceRoot;
  const bool bLocked = instance != nullptr;
  const bool bPrefabRoot = gameObject == EditorScene::getPrefabRoot();

  // Objects can share a name, so the pointer keeps their tree nodes apart.
  ImGui::PushID(gameObject.get());
  if (gameObject.get() == m_openOnNextDraw) {
    // A model just split into parts opens, so the parts show at once.
    ImGui::SetNextItemOpen(true);
    m_openOnNextDraw = nullptr;
  }
  if (bLocked) {
    ImGui::PushStyleColor(ImGuiCol_Text, bInstanceRoot ? ImVec4(0.45f, 0.7f, 1.0f, 1.0f)
                                                       : ImVec4(0.55f, 0.55f, 0.6f, 1.0f));
  }
  const bool bOpen = ImGui::TreeNodeEx("##node", flags, "%s", gameObject->getName().c_str());
  if (bLocked) {
    ImGui::PopStyleColor();
  }
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    EditorSelection::setSelectedGameObject(gameObject);
  }

  if (!bLocked && ImGui::BeginDragDropTarget()) {
    if (SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
      m_pendingModel = std::move(model);
      m_pendingParent = gameObject;
    }
    if (SPtr<PrefabAsset> prefab = AssetDragDrop::acceptPrefab()) {
      m_pendingPrefab = std::move(prefab);
      m_pendingParent = gameObject;
    }
    ImGui::EndDragDropTarget();
  }

  // Inside the PushID of this object, so a fixed popup name is unique.
  if (ImGui::BeginPopupContextItem("GameObjectContext")) {
    renderObjectContextMenu(scene, gameObject, bLocked, bInsideInstance, bPrefabRoot);
    ImGui::EndPopup();
  }

  if (bOpen) {
    for (const SPtr<GameObject>& child : gameObject->getChildren()) {
      renderGameObject(scene, child);
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
}

/*
 */
void
SceneGraphUI::renderObjectContextMenu(Scene& scene,
                                      const SPtr<GameObject>& gameObject,
                                      bool bLocked,
                                      bool bInsideInstance,
                                      bool bPrefabRoot)
{
  if (ImGui::MenuItem("Create Child", nullptr, false, !bLocked)) {
    EditorScene::recordCreated(*scene.createGameObject("GameObject", gameObject.get()),
                               "Create");
  }
  if (!bLocked) {
    renderCreateLightItems(scene, gameObject.get());
  }
  const ModelComponent* model = gameObject->getComponent<ModelComponent>();
  if (!bLocked && model && model->isWholeModel() && model->getModel() &&
      ImGui::BeginMenu("Expand Model")) {
    if (ImGui::MenuItem("By Node")) {
      m_pendingExpand = gameObject;
      m_pendingExpandMode = ModelExpandMode::PerNode;
    }
    if (ImGui::MenuItem("By Mesh")) {
      m_pendingExpand = gameObject;
      m_pendingExpandMode = ModelExpandMode::PerMesh;
    }
    ImGui::EndMenu();
  }

  ImGui::Separator();
  if (ImGui::MenuItem("Create Prefab", nullptr, false, !bInsideInstance)) {
    m_pendingCreatePrefab = gameObject;
  }
  if (!gameObject->getPrefab().isNull()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(gameObject->getPrefab());
    const SPtr<PrefabAsset> prefab = asset ? asset->as<PrefabAsset>() : nullptr;
    if (ImGui::MenuItem("Edit Prefab", nullptr, false, prefab != nullptr)) {
      EditorScene::requestEditPrefab(prefab);
    }
    if (ImGui::MenuItem("Unpack Prefab", nullptr, false, !bInsideInstance)) {
      // The content stays; it is saved with the scene from now on.
      EditorScene::recordChange(*gameObject, "Unpack");
      gameObject->setPrefab(UUID::null());
    }
  }

  ImGui::Separator();
  if (ImGui::MenuItem("Delete", nullptr, false, !bInsideInstance && !bPrefabRoot)) {
    m_pendingDelete = gameObject;
  }
}

/*
 */
void
SceneGraphUI::renderEmptyAreaContextMenu(Scene& scene)
{
  if (ImGui::BeginPopupContextWindow("EmptyAreaContextMenu_SceneGraph",
                                     ImGuiPopupFlags_MouseButtonRight |
                                         ImGuiPopupFlags_NoOpenOverItems)) {
    GameObject* parent = EditorScene::getDefaultParent();
    if (ImGui::MenuItem("Create Empty")) {
      EditorScene::recordCreated(*scene.createGameObject("GameObject", parent), "Create");
    }
    renderCreateLightItems(scene, parent);
    ImGui::EndPopup();
  }
}

/*
 */
void
SceneGraphUI::renderEmptyAreaDropTarget()
{
  // Only while dragging: at other times the button would cover the empty area and keep
  // its context menu from opening.
  if (!ImGui::GetDragDropPayload()) {
    return;
  }

  const ImVec2 available = ImGui::GetContentRegionAvail();
  ImGui::InvisibleButton("##emptyAreaDrop", ImVec2(Math::max(available.x, 1.0f),
                                                   Math::max(available.y,
                                                             ImGui::GetFrameHeight())));
  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
      m_pendingModel = std::move(model);
      m_pendingParent.reset();
    }
    if (SPtr<PrefabAsset> prefab = AssetDragDrop::acceptPrefab()) {
      m_pendingPrefab = std::move(prefab);
      m_pendingParent.reset();
    }
    ImGui::EndDragDropTarget();
  }
}

/*
 */
void
SceneGraphUI::addPendingObject(Scene& scene)
{
  GameObject* parent =
      m_pendingParent ? m_pendingParent.get() : EditorScene::getDefaultParent();
  const SPtr<GameObject> gameObject =
      m_pendingModel ? AssetDragDrop::createModelObject(scene, m_pendingModel, parent)
                     : AssetDragDrop::createPrefabObject(scene, m_pendingPrefab, parent);
  m_pendingModel.reset();
  m_pendingPrefab.reset();
  m_pendingParent.reset();
  if (!gameObject || !m_editorCamera) {
    return;
  }

  // Under a moved parent the render items are only in place after the transform update.
  scene.updateTransforms();
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (ModelComponent::getHierarchyWorldBounds(*gameObject, bounds)) {
    m_editorCamera->focus(bounds);
  }
}

} // namespace chEngineSDK
