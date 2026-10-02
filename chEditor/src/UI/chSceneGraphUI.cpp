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
#include "chSceneManager.h"
#include "chGameObject.h"

#include "imgui.h"

CH_LOG_DECLARE_STATIC(SceneGraphUILog, All);

namespace chEngineSDK {

SceneGraphUI::SceneGraphUI() {
  SceneManager& sceneManager = SceneManager::instance();
  WeakPtr<Scene> weakScene = sceneManager.getActiveScene();
  m_currentScene = weakScene.lock();
  if (m_currentScene) {
    buildSceneGraphData(m_currentScene->getRootGameObjects());
  }
}

SceneGraphUI::~SceneGraphUI() {
  // Destructor implementation
}

void
SceneGraphUI::renderSceneGraphUI() {
  const ANSICHAR* sceneName =
      m_currentScene ? m_currentScene->getName().c_str() : "No Active Scene";
  if (m_windowTitle.empty() || m_titleSceneName != sceneName) {
    m_titleSceneName = sceneName;
    // "###" keeps the window ID fixed, so ImGui keeps its position and docking when the
    // scene changes.
    m_windowTitle = chString::format("Scene Graph - {0}###SceneGraph", m_titleSceneName);
  }

  ImGui::Begin(m_windowTitle.c_str(), &m_isVisible);
  for (const auto& nodeData : m_sceneGraphData) {
    // Objects can share a name, so the pointer keeps their tree nodes apart.
    ImGui::PushID(nodeData.gameObject.get());
    if (ImGui::TreeNode(nodeData.gameObject->getName().c_str())) {
      ImGui::TreePop();
    }
    if (ImGui::IsItemClicked()) {
      EditorSelection::setSelectedGameObject(nodeData.gameObject);
    }

    if (nodeData.gameObject->getName() != "Root") {
      handleContextMenuForGameObject(nodeData.gameObject);
    }
    ImGui::PopID();
  }

  handleEmptyAreaContextMenu();
  renderEmptyAreaContextMenu();

  ImGui::End();
}

/*
*/
void
SceneGraphUI::handleEmptyAreaContextMenu() {
    // Early return if any item is hovered
  if (ImGui::IsAnyItemHovered()) {
    return;
  }

  // Early return if not hovering window
  if (!ImGui::IsWindowHovered()) {
    return;
  }

  // Check for right click in empty area
  bool emptyAreaRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

  if (!emptyAreaRightClicked) {
    return;
  }

  ImGui::OpenPopup("EmptyAreaContextMenu_SceneGraph");
}

/*
*/
void
SceneGraphUI::renderEmptyAreaContextMenu() {
  if (!ImGui::BeginPopup("EmptyAreaContextMenu_SceneGraph")) {
    return;
  }

  ImGui::EndPopup();
}

/*
*/
void
SceneGraphUI::handleContextMenuForGameObject(const SPtr<GameObject>& gameObject) {
  CH_PARAMETER_UNUSED(gameObject);

  // Called inside the PushID of this object, so a fixed popup name is unique.
  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImGui::OpenPopup("GameObjectContext");
  }
  if (ImGui::BeginPopup("GameObjectContext")) {
    // Context menu items for the GameObject
    if (ImGui::MenuItem("Delete")) {
      // Handle delete action
    }
    ImGui::EndPopup();
  }
}
/*
*/
void
SceneGraphUI::buildSceneGraphData(const Vector<SPtr<GameObject>>& rootGameObjects) {
  for (const auto& gameObject : rootGameObjects) {
    // Process each root GameObject and build UI data
    SceneNodeUIData nodeData;
    nodeData.gameObject = gameObject;
    m_sceneGraphData.push_back(nodeData);
  }
}

} // namespace chEngineSDK
