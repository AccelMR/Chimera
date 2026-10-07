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

  void
  saveUnsavedAssets();

 private:
  void
  renderDeleteConfirmationPopup();

  void
  loadAsset(const SPtr<IAsset>& asset);

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
  renderAssetStateIndicator(const SPtr<IAsset>& asset);

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
   * Thumbnail of a texture asset, created the first time it is needed. Returns 0 if
   * it could not be created; that is only tried once.
   */
  NODISCARD uint64
  getThumbnail(const SPtr<IAsset>& asset);

 private:
  Vector<SPtr<IAsset>> m_assets;

  // Indices in m_assets of the assets that pass the filters.
  Vector<uint32> m_visibleAssets;
  bool m_needsFilterUpdate = true;

  SPtr<IAsset> m_assetToDelete;
  bool m_showDeleteConfirmation = false;

  // A texture whose thumbnail failed keeps an empty entry, so it is not tried again.
  UnorderedMap<UUID, Pair<SPtr<ITexture>, uint64>> m_assetThumbnails;

  Vector<WeakPtr<IAsset>> m_unsavedAssets;

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
};
} // namespace chEngineSDK
