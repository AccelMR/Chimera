/************************************************************************/
/**
 * @file chAssetDragDrop.h
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Assets dragged from the Content Browser and GameObjects dragged from the Scene Graph.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class AnimationClipAsset;
class MaterialAsset;
class ModelAsset;
class PrefabAsset;
class ScriptAsset;
class TextureAsset;

/**
 * Exists so every window that takes assets from the Content Browser, or GameObjects from the
 * Scene Graph, agrees on the payload: the UUID, under one payload name per asset type (and
 * one for GameObjects), so a window only highlights for the types it accepts.
 */
class AssetDragDrop
{
 public:
  /**
   * Makes the last ImGui item a drag source for the asset. Types that only a folder takes
   * (scenes, skeletons) share one payload name. With a count above one the drag shows how
   * many assets the selection holds; the payload is still this asset.
   */
  static void
  source(const IAsset& asset, uint32 selectedCount = 1);

  /**
   * Same as acceptModel, for an asset of any type, and not loaded. Used to move assets
   * between folders.
   */
  NODISCARD static SPtr<IAsset>
  acceptAnyAsset();

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
   * Same as acceptModel, for animation clips.
   */
  NODISCARD static SPtr<AnimationClipAsset>
  acceptAnimation();

  /**
   * Same as acceptModel, for scripts.
   */
  NODISCARD static SPtr<ScriptAsset>
  acceptScript();

  /**
   * Makes the last ImGui item a drag source for gameObject, an object of the open scene. When
   * it is selected, the drop acts on the whole selection.
   */
  static void
  source(const GameObject& gameObject);

  /**
   * The object of the open scene being dragged, or null when the payload is something else.
   * Lets a target refuse a drop before it highlights.
   */
  NODISCARD static SPtr<GameObject>
  getDraggedGameObject();

  /**
   * Same as acceptModel, for an object of the open scene. imguiFlags are ImGuiDragDropFlags,
   * so a target can take the payload before the drop to draw where it would land.
   */
  NODISCARD static SPtr<GameObject>
  acceptGameObject(int32 imguiFlags = 0);

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
