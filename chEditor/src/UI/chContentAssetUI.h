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
#include "chMultiStageRenderer.h"

struct ImVec4;
struct ImVec2;

namespace chEngineSDK {
class NastyRenderer;
class ContentAssetUI
{
 public:
  ContentAssetUI();
  ~ContentAssetUI() = default;

  // Method to render the content asset UI
  void
  renderContentAssetUI();
  void
  refreshAssets();

  // DELETEME
  void
  setMultiStageRenderer(SPtr<MultiStageRenderer> renderer) {
    m_multiStageRenderer = std::move(renderer);
  }

 private:
  // Helper method to render asset deletion confirmation popup
  void
  renderDeleteConfirmationPopup();

  // Helper method to handle asset selection
  void
  handleAssetSelection(const SPtr<IAsset>& asset);

  // Helper method to render context menu for assets
  void
  renderAssetContextMenu(const SPtr<IAsset>& asset);

  // Helper method to get asset state color
  ImVec4
  getAssetStateColor(const SPtr<IAsset>& asset);

  // Helper method to get asset state string
  String
  getAssetStateString(const SPtr<IAsset>& asset);

  // Inline rename functionality
  void
  startInlineRename(const SPtr<IAsset>& asset);
  void
  finishInlineRename();
  void
  cancelInlineRename();
  bool
  renderInlineRename(const SPtr<IAsset>& asset, const String& displayName);

  void
  renderSearchBar();
  void
  renderAssetTypeFilters();
  void
  renderViewModeControls();
  void
  renderAssetDisplayArea();
  void
  renderGridView();
  bool
  passesSearchFilter(const SPtr<IAsset>& asset);
  bool
  passesTypeFilter(const SPtr<IAsset>& asset);
  void
  renderAssetIconButton(const SPtr<IAsset>& asset, float size = 64.0f);
  void
  renderAssetNameGrid(const SPtr<IAsset>& asset, float size = 64.0f);
  void
  renderAssetStateIndicator(const SPtr<IAsset>& asset);
  void
  renderListView();
  void
  renderAssetNameInGrid(const SPtr<IAsset>& asset, float size = 64.0f);
  void
  renderAssetTooltip(const SPtr<IAsset>& asset);
  void
  handleAssetContextMenu(const SPtr<IAsset>& asset);
  void
  renderSelectableAssetName(const SPtr<IAsset>& asset);
  bool
  shouldShowAsset(const SPtr<IAsset>& asset);
  void
  renderGridAssetItem(const SPtr<IAsset>& asset,
                      int32 currentColumn,
                      float gridSize);

  void
  handleEmptyAreaContextMenu();

  void
  renderEmptyAreaContextMenu();

  void
  setupTableColumns();
  void
  renderListAssetItem(const SPtr<IAsset>& asset);

 private:
  Vector<SPtr<IAsset>> m_assets;
  SPtr<IAsset> m_assetToDelete;          ///< Asset to delete, set when delete is requested
  bool m_showDeleteConfirmation = false; ///< Flag to show delete confirmation popup
  //SPtr<NastyRenderer> m_nastyRenderer;   ///< Nasty renderer instance for rendering assets
  SPtr<MultiStageRenderer> m_multiStageRenderer; ///< Multi-stage renderer instance
  SPtr<ISampler> m_defaultSampler; ///< Default sampler for textures
  UnorderedMap<UUID, Pair<SPtr<ITextureView>, SPtr<IDescriptorSet>>> m_assetThumbnails;

  bool m_isRenaming = false;
  SPtr<IAsset> m_renamingAsset = nullptr;
  ANSICHAR m_renameBuffer[256] = {0}; // Buffer for the new name
  bool m_renameFocusRequested = false;

  bool showAllTypes = true;
  bool showModels = true;
  bool showTextures = true;
  bool showMaterials = true;
  bool showOther = true;
  bool gridView = true;
  float gridSize = 80.0f;
  char searchBuffer[256] = "";
  float indicatorRadius = 3.0f;

}; // class ContentAssetUI
} // namespace chEngineSDK
