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

#include "chModelExpand.h"

namespace chEngineSDK {
class ContentAssetUI;
class EditorCamera;
class ModelAsset;
class PrefabAsset;

/**
 * Editor window that shows the GameObject hierarchy of the active scene, selects objects
 * and creates or deletes them. It reads the scene every frame, so it never shows a stale
 * tree. A model or prefab dropped on an object becomes its child, and one dropped on the
 * empty area becomes a root object. Prefab instances show in blue and their content, which
 * comes from the prefab, cannot be changed here.
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

  /**
   * Create Prefab puts new prefabs in this browser's folder. Not owned; it outlives this
   * window.
   */
  FORCEINLINE void
  setContentBrowser(const ContentAssetUI* contentBrowser)
  {
    m_contentBrowser = contentBrowser;
  }

 private:
  void
  renderGameObject(Scene& scene, const SPtr<GameObject>& gameObject);

  void
  applyPendingChanges(Scene& scene);

  void
  renderObjectContextMenu(Scene& scene,
                          const SPtr<GameObject>& gameObject,
                          bool bLocked,
                          bool bInsideInstance,
                          bool bPrefabRoot);

  void
  renderEmptyAreaContextMenu(Scene& scene);

  void
  renderEmptyAreaDropTarget();

  void
  addPendingObject(Scene& scene);

  bool m_isVisible = true;
  EditorCamera* m_editorCamera = nullptr;
  const ContentAssetUI* m_contentBrowser = nullptr;

  // Deleting, adding or expanding while the tree is drawn would change the vectors being
  // iterated, so they wait until the tree is done.
  SPtr<GameObject> m_pendingDelete;
  SPtr<ModelAsset> m_pendingModel;
  SPtr<PrefabAsset> m_pendingPrefab;
  SPtr<GameObject> m_pendingParent;
  SPtr<GameObject> m_pendingCreatePrefab;
  SPtr<GameObject> m_pendingExpand;
  ModelExpandMode m_pendingExpandMode = ModelExpandMode::Single;
  // Only compared, never followed.
  const GameObject* m_openOnNextDraw = nullptr;

  // Rebuilt only when the scene name changes.
  String m_windowTitle;
  String m_titleSceneName;
};
} // namespace chEngineSDK
