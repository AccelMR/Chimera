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

namespace chEngineSDK {

/**
 * Editor window that shows the GameObject hierarchy of the active scene, selects objects
 * and creates or deletes them. It reads the scene every frame, so it never shows a stale
 * tree.
 */
class SceneGraphUI
{
 public:
  SceneGraphUI() = default;

  void
  renderSceneGraphUI();

 private:
  void
  renderGameObject(Scene& scene, const SPtr<GameObject>& gameObject);

  void
  renderEmptyAreaContextMenu(Scene& scene);

  bool m_isVisible = true;

  // Deleting while the tree is drawn would change the vectors being iterated, so it waits
  // until the tree is done.
  SPtr<GameObject> m_pendingDelete;

  // Rebuilt only when the scene name changes.
  String m_windowTitle;
  String m_titleSceneName;
};
} // namespace chEngineSDK
