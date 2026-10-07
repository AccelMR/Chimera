/************************************************************************/
/**
 * @file chAssetDragDrop.h
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Assets dragged from the Content Browser and dropped on other editor windows.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class MaterialAsset;
class ModelAsset;
class TextureAsset;

/**
 * Exists so every window that takes assets from the Content Browser agrees on the payload:
 * the asset UUID, under one payload name per asset type, so a window only highlights for
 * the types it accepts.
 */
class AssetDragDrop
{
 public:
  /**
   * Makes the last ImGui item a drag source for the asset when its type can be dropped
   * somewhere.
   */
  static void
  source(const IAsset& asset);

  /**
   * Call between BeginDragDropTarget and EndDragDropTarget. Returns the model dropped this
   * frame, already loaded, or null.
   */
  NODISCARD static SPtr<ModelAsset>
  acceptModel();

  /**
   * Same as acceptModel, for textures.
   */
  NODISCARD static SPtr<TextureAsset>
  acceptTexture();

  /**
   * Same as acceptModel, for materials.
   */
  NODISCARD static SPtr<MaterialAsset>
  acceptMaterial();

  /**
   * Adds a GameObject with the model under parent (a root object when null) and selects it.
   */
  static SPtr<GameObject>
  createModelObject(Scene& scene, const ModelAsset& modelAsset, GameObject* parent);
};

} // namespace chEngineSDK
