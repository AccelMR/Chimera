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
class PrefabAsset;
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
   * Same as acceptModel, for prefabs.
   */
  NODISCARD static SPtr<PrefabAsset>
  acceptPrefab();

  /**
   * Adds a GameObject with the model under parent (a root object when null) and selects it.
   * The model is split as its import settings say, unless Alt is held.
   */
  static SPtr<GameObject>
  createModelObject(Scene& scene, const SPtr<ModelAsset>& modelAsset, GameObject* parent);

  /**
   * Adds an instance of the prefab under parent (a root object when null) and selects it.
   * Refused, with null, when it would put a prefab inside itself.
   */
  static SPtr<GameObject>
  createPrefabObject(Scene& scene, const SPtr<PrefabAsset>& prefab, GameObject* parent);
};

} // namespace chEngineSDK
