/************************************************************************/
/**
 * @file chAssetThumbnails.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Pictures of assets for the Content Browser.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class IAsset;
class ICommandList;

/**
 * Exists so the Content Browser shows what an asset looks like instead of its type icon:
 * models, materials (on a sphere) and prefabs are drawn alone, by a SceneRenderer of its
 * own, into a texture kept for the session; textures show their own image. A thumbnail is
 * made when a tile first asks for it, one asset per frame, and nothing is saved to disk, so
 * they are made again in every session. A regenerated thumbnail keeps the old image until
 * the new one is drawn.
 *
 * A frame is the UI asking with get(), update() once the UI is done, then recordFrame() in
 * the render, before the pass that draws the UI.
 */
class AssetThumbnails
{
 public:
  /**
   * ImGui texture id of the asset's thumbnail, or 0 while there is none (the caller shows
   * the icon). The first call asks for it.
   */
  NODISCARD static uint64
  get(const SPtr<IAsset>& asset);

  NODISCARD static bool
  canHaveThumbnail(const IAsset& asset);

  /**
   * Draws the thumbnail again; does nothing when no tile asked for it yet.
   */
  static void
  regenerate(const IAsset& asset);

  static void
  regenerateAll();

  /**
   * Forgets the asset's thumbnail, before the asset is deleted.
   */
  static void
  forget(const IAsset& asset);

  /**
   * Builds the scene of the next asset in the queue and copies it for the renderer.
   */
  static void
  update();

  /**
   * Draws the asset update() prepared into its thumbnail.
   */
  static void
  recordFrame(ICommandList& commandList);

  /**
   * Controls for the thumbnail settings, for a popup.
   */
  static void
  renderSettings();

  /**
   * Releases the thumbnails, the scene and the renderer. Call after the GPU is idle, before
   * the asset manager and the graphics API shut down.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
