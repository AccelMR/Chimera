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

#include "chSceneManager.h"
#include "chGameObject.h"

#include "imgui.h"

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
      // Display GameObject details or children here
      ImGui::Text("UUID: %s", nodeData.gameObject->getUUID().toString().c_str());
      ImGui::TreePop();
    }
  }
  ImGui::End();
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
