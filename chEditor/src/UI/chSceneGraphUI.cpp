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
#include "chEditorCamera.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLightComponent.h"
#include "chMath.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
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
    for (const SPtr<GameObject>& root : scene->getRootGameObjects()) {
      renderGameObject(*scene, root);
    }
    renderEmptyAreaContextMenu(*scene);
    renderEmptyAreaDropTarget();

    if (m_pendingModel) {
      addPendingModel(*scene);
    }
    if (m_pendingExpand) {
      if (ModelExpand::expand(*scene, *m_pendingExpand, m_pendingExpandMode)) {
        m_openOnNextDraw = m_pendingExpand.get();
      }
      m_pendingExpand.reset();
    }
    if (m_pendingDelete) {
      const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
      if (isInSubtree(selected.get(), *m_pendingDelete)) {
        EditorSelection::setSelectedGameObject(nullptr);
      }
      scene->destroyGameObject(*m_pendingDelete);
      m_pendingDelete.reset();
    }
  }
  ImGui::End();
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

  // Objects can share a name, so the pointer keeps their tree nodes apart.
  ImGui::PushID(gameObject.get());
  if (gameObject.get() == m_openOnNextDraw) {
    // A model just split into parts opens, so the parts show at once.
    ImGui::SetNextItemOpen(true);
    m_openOnNextDraw = nullptr;
  }
  const bool bOpen = ImGui::TreeNodeEx("##node", flags, "%s", gameObject->getName().c_str());
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    EditorSelection::setSelectedGameObject(gameObject);
  }

  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
      m_pendingModel = std::move(model);
      m_pendingModelParent = gameObject;
    }
    ImGui::EndDragDropTarget();
  }

  // Inside the PushID of this object, so a fixed popup name is unique.
  if (ImGui::BeginPopupContextItem("GameObjectContext")) {
    if (ImGui::MenuItem("Create Child")) {
      scene.createGameObject("GameObject", gameObject.get());
    }
    renderCreateLightItems(scene, gameObject.get());
    const ModelComponent* model = gameObject->getComponent<ModelComponent>();
    if (model && model->isWholeModel() && model->getModel() &&
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
    if (ImGui::MenuItem("Delete")) {
      m_pendingDelete = gameObject;
    }
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
SceneGraphUI::renderEmptyAreaContextMenu(Scene& scene)
{
  if (ImGui::BeginPopupContextWindow("EmptyAreaContextMenu_SceneGraph",
                                     ImGuiPopupFlags_MouseButtonRight |
                                         ImGuiPopupFlags_NoOpenOverItems)) {
    if (ImGui::MenuItem("Create Empty")) {
      scene.createGameObject("GameObject");
    }
    renderCreateLightItems(scene, nullptr);
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
      m_pendingModelParent.reset();
    }
    ImGui::EndDragDropTarget();
  }
}

/*
 */
void
SceneGraphUI::addPendingModel(Scene& scene)
{
  const SPtr<GameObject> gameObject =
      AssetDragDrop::createModelObject(scene, *m_pendingModel, m_pendingModelParent.get());
  m_pendingModel.reset();
  m_pendingModelParent.reset();
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
