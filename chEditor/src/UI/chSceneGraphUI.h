/************************************************************************/
/**
 * @file chSceneGraphUI.h
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Scene graph UI class for the Chimera Editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chScene.h"

namespace chEngineSDK {

struct SceneNodeUIData {
  SPtr<GameObject> gameObject;
  bool isExpanded = false;
};

class SceneGraphUI {
 public:
  SceneGraphUI();
  ~SceneGraphUI();

  /**
   * @brief Main rendering function for the scene graph window
   */
  void
  renderSceneGraphUI();

 private:

  void
  buildSceneGraphData(const Vector<SPtr<GameObject>>& rootGameObjects);

  void
  handleEmptyAreaContextMenu();

  void
  renderEmptyAreaContextMenu();

  void
  handleContextMenuForGameObject(const SPtr<GameObject>& gameObject);

 private:
  // UI state
  bool m_isVisible = true;
  SPtr<Scene> m_currentScene;
  Vector<SceneNodeUIData> m_sceneGraphData;
};
} // namespace chEngineSDK
