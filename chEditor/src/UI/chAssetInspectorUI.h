/************************************************************************/
/**
 * @file chAssetInspectorUI.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Window that shows what an asset holds.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class IAsset;

/**
 * Exists so assets can be looked at and edited without touching the Object Inspector, which
 * stays on the selected GameObject while assets are picked or dragged. Double clicking an
 * asset in the Content Browser opens it here: a model's nodes, meshes, materials and clips,
 * a texture's image, a material's values, a skeleton's bones, a clip's tracks. Only the
 * asset opened last is shown; it loads when opened. The window stays open between sessions.
 */
class AssetInspectorUI
{
 public:
  static void
  render();

  /**
   * Shows the asset (loading it first) and brings the window to the front.
   */
  static void
  open(const SPtr<IAsset>& asset);

  /**
   * Forgets the asset if it is the one shown, before it is deleted.
   */
  static void
  close(const IAsset& asset);

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);

  /**
   * Drops the asset shown, so nothing of it outlives the asset manager.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
