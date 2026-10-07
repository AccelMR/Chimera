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
class EditorCamera;
class ModelAsset;

/**
 * Editor window that shows the GameObject hierarchy of the active scene, selects objects
 * and creates or deletes them. It reads the scene every frame, so it never shows a stale
 * tree. A model dropped on an object becomes its child, and one dropped on the empty area
 * becomes a root object.
 */
class SceneGraphUI
{
 public:
  SceneGraphUI() = default;

  void
  renderSceneGraphUI();

  /**
   * A dropped model is framed with this camera. Not owned; it outlives this window.
   */
  FORCEINLINE void
  setEditorCamera(EditorCamera* editorCamera) { m_editorCamera = editorCamera; }

 private:
  void
  renderGameObject(Scene& scene, const SPtr<GameObject>& gameObject);

  void
  renderEmptyAreaContextMenu(Scene& scene);

  void
  renderEmptyAreaDropTarget();

  void
  addPendingModel(Scene& scene);

  bool m_isVisible = true;
  EditorCamera* m_editorCamera = nullptr;

  // Deleting or adding while the tree is drawn would change the vectors being iterated, so
  // both wait until the tree is done.
  SPtr<GameObject> m_pendingDelete;
  SPtr<ModelAsset> m_pendingModel;
  SPtr<GameObject> m_pendingModelParent;

  // Rebuilt only when the scene name changes.
  String m_windowTitle;
  String m_titleSceneName;
};
} // namespace chEngineSDK
