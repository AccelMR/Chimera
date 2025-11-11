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
  const String windowTitle = m_currentScene
                                 ? "Scene Graph - " + m_currentScene->getName()
                                 : "Scene Graph - No Active Scene";
  // Rendering code for the scene graph UI
  ImGui::Begin(windowTitle.c_str(), &m_isVisible);
  for (const auto& nodeData : m_sceneGraphData) {
    if (ImGui::TreeNode(nodeData.gameObject->getName().c_str())) {
      ImGui::TreePop();
    }
    if (ImGui::IsItemClicked()) {
      EditorSelection::setSelectedGameObject(nodeData.gameObject);
    }

    if(nodeData.gameObject->getName() == "Root") {
      continue; // Skip context menu for root node
    }

    //Right click management
    handleContextMenuForGameObject(nodeData.gameObject);
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
  bool itemRightClicked =
      ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  String popupId = chString::format("GameObjectContext_{0}", gameObject->getUUID().toString());
  if (itemRightClicked) {
    ImGui::OpenPopup(popupId.c_str());
  }
  if (ImGui::BeginPopup(popupId.c_str())) {
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
