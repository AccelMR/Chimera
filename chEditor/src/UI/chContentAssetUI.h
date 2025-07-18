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

struct ImVec4;

namespace chEngineSDK {
class NastyRenderer;
class ContentAssetUI
{
 public:
  ContentAssetUI() = default;
  ~ContentAssetUI() = default;

  // Method to render the content asset UI
  void
  renderContentAssetUI();

  void
  refreshAssets();

  //DELETEME
  void
  setNastyRenderer(const SPtr<NastyRenderer>& renderer) {
    m_nastyRenderer = renderer;
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

 private:
  Vector<SPtr<IAsset>> m_assets;
  SPtr<IAsset> m_assetToDelete;          ///< Asset to delete, set when delete is requested
  bool m_showDeleteConfirmation = false; ///< Flag to show delete confirmation popup
  SPtr<NastyRenderer> m_nastyRenderer; ///< Nasty renderer instance for rendering assets
}; // class ContentAssetUI
} // namespace chEngineSDK
