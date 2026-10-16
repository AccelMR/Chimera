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
 * (Ctrl adds, Shift picks a range) and creates, renames, duplicates or deletes them. It reads
 * the scene every frame, so it never shows a stale tree. A model, prefab or the selected
 * objects dragged onto a row go inside it, or before or after it near its edges; dropped on
 * the empty area they become root objects. A search shows the matching objects as a flat
 * list. Prefab instances show in blue and their content, which comes from the prefab, cannot
 * be changed here.
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
  /**
   * parent is null for root objects. bFlat draws the row without its children, for the
   * search results.
   */
  void
  renderGameObject(Scene& scene,
                   const SPtr<GameObject>& gameObject,
                   const SPtr<GameObject>& parent,
                   bool bFlat);

  /**
   * Rows for every object whose name holds the search text, at any depth.
   */
  void
  renderSearchResults(Scene& scene, const Vector<SPtr<GameObject>>& gameObjects);

  void
  handleRowClick(const SPtr<GameObject>& gameObject, bool bSelected);

  /**
   * Takes objects dragged from this tree onto the row: inside it, or before or after it
   * near its top or bottom edge.
   */
  void
  acceptObjectDrop(const SPtr<GameObject>& gameObject,
                   const SPtr<GameObject>& parent,
                   bool bLocked,
                   bool bSiblingsAllowed);

  void
  handleKeys();

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

  void
  beginRename(const SPtr<GameObject>& gameObject);

  /**
   * Draws the name field after the row's icon, in place of its name.
   */
  void
  renderRenameField(GameObject& gameObject);

  bool m_isVisible = true;
  EditorCamera* m_editorCamera = nullptr;
  const ContentAssetUI* m_contentBrowser = nullptr;

  // Adding or expanding while the tree is drawn would change the vectors being iterated, so
  // they wait until the tree is done.
  SPtr<ModelAsset> m_pendingModel;
  SPtr<PrefabAsset> m_pendingPrefab;
  SPtr<GameObject> m_pendingParent;
  SPtr<GameObject> m_pendingExpand;
  ModelExpandMode m_pendingExpandMode = ModelExpandMode::Single;

  // The rows of this frame in the order they are drawn, for Shift ranges. They point into
  // the scene's own lists, which do not change while the tree is drawn.
  Vector<const SPtr<GameObject>*> m_visibleRows;
  // The pointers below are only compared, never followed.
  const GameObject* m_rangeAnchor = nullptr;
  const GameObject* m_pendingRangeEnd = nullptr;
  // A click on a row of a group selection selects it alone only once the button is released
  // without a drag, so the group can be dragged.
  const GameObject* m_pendingSingleSelect = nullptr;
  const GameObject* m_openOnNextDraw = nullptr;
  // Set when another window changed the active object, so its row is opened and scrolled to.
  const GameObject* m_lastSeenActive = nullptr;
  const GameObject* m_revealTarget = nullptr;

  SPtr<GameObject> m_renaming;
  ANSICHAR m_renameBuffer[256] = "";
  bool m_bRenameFocusRequested = false;
  ANSICHAR m_searchBuffer[128] = "";

  // Rebuilt only when the scene name changes.
  String m_windowTitle;
  String m_titleSceneName;
};
} // namespace chEngineSDK
