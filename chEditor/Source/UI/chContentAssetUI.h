/************************************************************************/
/**
 * @file chContentAssetUI.h
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chEventSystem.h"
#include "chPath.h"
#include "chUUID.h"

namespace chEngineSDK {

/**
 * Editor window that lists the project assets in a grid or a table, with search and
 * type filters. The filtered list is only rebuilt when the assets or the filters
 * change, and only the visible rows are drawn. Models are dragged from here onto the
 * scene, and assets and folders onto folders to move them.
 */
class ContentAssetUI
{
 public:
  ContentAssetUI();
  ~ContentAssetUI() = default;

  void
  renderContentAssetUI();

  /**
   * Reloads the asset list from the AssetManager.
   */
  void
  refreshAssets();

  /**
   * The folder the browser shows, where new assets go.
   */
  NODISCARD const Path&
  getCurrentFolderPath() const;

 private:
  void
  renderDeleteConfirmationPopup();

  void
  renderAssetContextMenu(const SPtr<IAsset>& asset);

  void
  startInlineRename(const SPtr<IAsset>& asset);

  void
  finishInlineRename();

  void
  cancelInlineRename();

  /**
   * Draws the rename field if this asset is being renamed. Returns false otherwise, so
   * the caller draws the name as usual.
   */
  bool
  renderInlineRename(const SPtr<IAsset>& asset);

  void
  startFolderRename(uint32 folderIndex);

  /**
   * Same as renderInlineRename, for a folder.
   */
  bool
  renderFolderRename(uint32 folderIndex);

  void
  renderRenameField();

  /**
   * Position of the item whose rename field waits for focus, counting the visible
   * subfolders first, or -1. The clippers draw it even when it is out of view.
   */
  NODISCARD int32
  findRenameFocusItem() const;

  /**
   * F2 renames the selected folder, else the selected asset.
   */
  void
  handleRenameShortcut();

  /**
   * Makes the last ImGui item a drag source for the folder, so it can be moved.
   */
  void
  folderDragSource(uint32 folderIndex);

  /**
   * Makes the last ImGui item a target for assets and folders, which move into folder, and
   * for objects dragged from the Scene Graph, which become prefabs there.
   */
  void
  acceptFolderDrop(const Path& folder);

  /**
   * The move runs after the frame's lists are drawn, because it reads the folders from
   * disk again. When a setting names a scene that moves by its path, it asks first. A
   * dragged asset that is part of a selection moves the whole selection.
   */
  void
  requestAssetMove(const SPtr<IAsset>& asset, const Path& folder);

  /**
   * Keeps the browser's selection in step with the asset EditorSelection holds, which
   * other windows may change; that one is the active asset of the selection.
   */
  void
  syncAssetSelection();

  NODISCARD bool
  isAssetSelected(const SPtr<IAsset>& asset) const;

  /**
   * Call after an asset's item. A click selects it, Ctrl+click toggles it, Shift+click
   * selects the drawn range from the last plain or Ctrl click. A plain click on part of a
   * selection waits for the release without a drag, so the selection can be dragged.
   */
  void
  handleAssetClick(const SPtr<IAsset>& asset);

  void
  selectOnlyAsset(const SPtr<IAsset>& asset);

  void
  renderDeleteFolderPopup();

  void
  requestFolderMove(const Path& folder, const Path& newFolder);

  void
  applyPendingMove();

  void
  renderMoveConfirmationPopup();

  /**
   * The settings that name, by path, a scene the pending move takes elsewhere. Assets
   * reference each other by UUID, so only text paths in config files break.
   */
  void
  findBrokenScenePaths();

  void
  renderSearchBar();

  void
  renderAssetTypeFilters();

  void
  renderViewModeControls();

  void
  renderAssetDisplayArea();

  void
  rebuildVisibleAssets();

  NODISCARD bool
  shouldShowAsset(const SPtr<IAsset>& asset) const;

  NODISCARD bool
  passesFolderFilter(const SPtr<IAsset>& asset) const;

  NODISCARD bool
  passesSearchFilter(const SPtr<IAsset>& asset) const;

  NODISCARD bool
  passesTypeFilter(const SPtr<IAsset>& asset) const;

  void
  renderGridView();

  void
  renderGridAssetItem(const SPtr<IAsset>& asset, int32 column);

  void
  renderAssetIconButton(const SPtr<IAsset>& asset);

  void
  renderAssetNameInGrid(const SPtr<IAsset>& asset);

  void
  renderListView();

  void
  setupTableColumns();

  void
  renderListAssetItem(const SPtr<IAsset>& asset);

  void
  renderSelectableAssetName(const SPtr<IAsset>& asset);

  void
  renderAssetTooltip(const SPtr<IAsset>& asset);

  void
  handleAssetContextMenu(const SPtr<IAsset>& asset);

  void
  handleEmptyAreaContextMenu();

  void
  renderEmptyAreaContextMenu();

  /**
   * Reads the folders under /Game from disk again, keeping the current folder when it
   * still exists.
   */
  void
  rebuildFolderTree();

  void
  scanFolder(uint32 folderIndex);

  NODISCARD uint32
  findFolder(const Path& path) const;

  void
  renderFolderTree();

  void
  renderFolderTreeNode(uint32 folderIndex);

  void
  renderBreadcrumb();

  /**
   * Takes effect after the frame's folder and asset lists are drawn, so the lists being
   * drawn do not change under them.
   */
  void
  requestFolder(uint32 folderIndex);

  void
  renderGridFolderItem(uint32 folderIndex, int32 column);

  void
  renderListFolderItem(uint32 folderIndex);

  void
  handleFolderContextMenu(uint32 folderIndex);

  /**
   * Makes a folder in the current one and starts renaming it.
   */
  void
  createFolder();

  /**
   * Writes a new script from a template into the current folder. Null on failure.
   */
  SPtr<IAsset>
  createScript();

  /**
   * Refreshes the list and starts renaming an asset just made.
   */
  void
  onAssetCreated(const SPtr<IAsset>& asset);

  /**
   * Opens what can be opened: scenes, prefabs in prefab mode, scripts in the program the
   * system uses for .lua files; the rest goes to the Asset Inspector.
   */
  void
  openAsset(const SPtr<IAsset>& asset);

  /**
   * Empty while searching, because a search lists the assets of every subfolder.
   */
  NODISCARD const Vector<uint32>&
  getVisibleSubfolders() const;

 private:
  static constexpr uint32 INVALID_FOLDER = ~0u;

  struct FolderNode
  {
    // Virtual, like /Game/Skeleton.
    Path path;
    String name;
    uint32 parent = INVALID_FOLDER;
    Vector<uint32> children;
  };

  Vector<SPtr<IAsset>> m_assets;

  // Read from disk on refresh only; [0] is /Game.
  Vector<FolderNode> m_folders;
  uint32 m_currentFolder = 0;
  uint32 m_requestedFolder = INVALID_FOLDER;
  // Opens the tree down to the current folder once, after it was picked outside the tree.
  bool m_bRevealCurrentFolder = false;

  // Indices in m_assets of the assets that pass the filters.
  Vector<uint32> m_visibleAssets;
  bool m_needsFilterUpdate = true;

  SPtr<IAsset> m_assetToDelete;
  bool m_showDeleteConfirmation = false;

  Path m_folderToDelete;
  uint32 m_folderToDeleteAssets = 0;
  bool m_bOpenDeleteFolder = false;

  // The last one is EditorSelection's asset.
  Vector<SPtr<IAsset>> m_selectedAssets;
  SPtr<IAsset> m_selectionAnchor;

  // Picked by a click on a folder tile or row; an asset click clears it. Used by F2.
  uint32 m_selectedFolder = INVALID_FOLDER;

  bool m_isRenaming = false;
  SPtr<IAsset> m_renamingAsset = nullptr;
  uint32 m_renamingFolder = INVALID_FOLDER;
  ANSICHAR m_renameBuffer[256] = {0};
  // Also keeps the renamed row drawn and scrolls to it, for a new asset out of view.
  bool m_renameFocusRequested = false;
  // Set for a script just made from the template, whose class is named after the file.
  bool m_bRenameRewritesScript = false;

  // Either assets or a folder (m_moveFolder not empty) move into m_moveTarget.
  bool m_bMoveRequested = false;
  bool m_bMoveConfirmed = false;
  Vector<SPtr<IAsset>> m_moveAssets;
  Path m_moveFolder;
  Path m_moveTarget;
  Vector<String> m_moveWarnings;
  bool m_bOpenMoveConfirmation = false;

  bool showAllTypes = true;
  bool showModels = true;
  bool showTextures = true;
  bool showMaterials = true;
  bool showOther = true;
  bool gridView = true;
  float gridSize = 120.0f;
  ANSICHAR searchBuffer[256] = "";

  HEvent m_onAssetImported;
  HEvent m_onSceneCreated;
};
} // namespace chEngineSDK
