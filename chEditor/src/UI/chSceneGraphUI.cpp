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

#include "chEditorSelection.h"
#include "chGameObject.h"
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
  const bool bOpen = ImGui::TreeNodeEx("##node", flags, "%s", gameObject->getName().c_str());
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    EditorSelection::setSelectedGameObject(gameObject);
  }

  // Inside the PushID of this object, so a fixed popup name is unique.
  if (ImGui::BeginPopupContextItem("GameObjectContext")) {
    if (ImGui::MenuItem("Create Child")) {
      scene.createGameObject("GameObject", gameObject.get());
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
    ImGui::EndPopup();
  }
}

} // namespace chEngineSDK
