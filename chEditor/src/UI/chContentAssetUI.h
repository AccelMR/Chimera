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
 * scene.
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

  /**
   * Makes the last ImGui item a target for objects dragged from the Scene Graph, which
   * become prefabs in folder.
   */
  void
  acceptPrefabDrop(const Path& folder);

  void
  renderGridFolderItem(uint32 folderIndex, int32 column);

  void
  renderListFolderItem(uint32 folderIndex);

  void
  createFolder();

  /**
   * Opens what can be opened (scenes, and prefabs in prefab mode); does nothing for the rest.
   */
  void
  openAsset(const SPtr<IAsset>& asset);

  /**
   * Empty while searching, because a search lists the assets of every subfolder.
   */
  NODISCARD const Vector<uint32>&
  getVisibleSubfolders() const;

  /**
   * Thumbnail of a texture asset, created the first time it is needed. Returns 0 if
   * it could not be created; that is only tried once.
   */
  NODISCARD uint64
  getThumbnail(const SPtr<IAsset>& asset);

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

  // A texture whose thumbnail failed keeps an empty entry, so it is not tried again.
  UnorderedMap<UUID, Pair<SPtr<ITexture>, uint64>> m_assetThumbnails;


  bool m_isRenaming = false;
  SPtr<IAsset> m_renamingAsset = nullptr;
  ANSICHAR m_renameBuffer[256] = {0};
  bool m_renameFocusRequested = false;

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
