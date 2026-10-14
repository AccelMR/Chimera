/************************************************************************/
/**
 * @file chEditorScene.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  The scene open in the editor and the requests to make, open and save scenes.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chEventSystem.h"

namespace chEngineSDK {
class EditorCamera;
class PrefabAsset;
class SceneAsset;

/**
 * Exists to keep track of what the editor shows: a scene, or a prefab being edited (prefab
 * mode), with the asset it is saved in and whether it changed since it was saved. Requests
 * that would lose changes ask first, and the scene only changes in applyPending, after the
 * frame's UI, so no panel sees its scene go away while it draws.
 */
class EditorScene
{
 public:
  /**
   * Opens the Editor.StartupScene asset, or a new scene with a sun when there is none.
   */
  static void
  initialize(EditorCamera& camera);

  static void
  shutDown();

  NODISCARD static const SPtr<Scene>&
  getScene();

  /**
   * Null while the scene was never saved, and while a prefab is edited.
   */
  NODISCARD static const SPtr<SceneAsset>&
  getAsset();

  /**
   * The prefab being edited, or null outside prefab mode.
   */
  NODISCARD static const SPtr<PrefabAsset>&
  getPrefab();

  /**
   * In prefab mode, the object that holds the prefab's content; what is saved into it.
   */
  NODISCARD static const SPtr<GameObject>&
  getPrefabRoot();

  /**
   * Where objects made without a parent go: the prefab's root in prefab mode, else null
   * (the scene's root).
   */
  NODISCARD static GameObject*
  getDefaultParent();

  /**
   * Call after every change to the scene's objects or components.
   */
  static void
  markDirty();

  NODISCARD static bool
  isDirty();

  /**
   * Changes whenever the scene, its name or its dirty flag changes, so a title can be built
   * again only then.
   */
  NODISCARD static uint32
  getRevision();

  /**
   * The scene's name, with "*" when it has unsaved changes.
   */
  NODISCARD static String
  getDisplayName();

  static void
  requestNew();

  static void
  requestOpen(const SPtr<SceneAsset>& asset);

  /**
   * Saves at once into the scene's asset; a scene never saved asks for a name first.
   */
  static void
  requestSave();

  static void
  requestSaveAs();

  /**
   * Opens the prefab in prefab mode; requestBackToScene goes back to the scene that was
   * open.
   */
  static void
  requestEditPrefab(const SPtr<PrefabAsset>& prefab);

  static void
  requestBackToScene();

  /**
   * Saves gameObject and its children as a new prefab in folder and makes the object an
   * instance of it. Null on failure.
   */
  static SPtr<PrefabAsset>
  createPrefab(GameObject& gameObject, const Path& folder);

  /**
   * True when the editor can close now. Otherwise it asks about the unsaved changes first,
   * and shouldQuit becomes true once they are saved or discarded.
   */
  NODISCARD static bool
  requestQuit();

  NODISCARD static bool
  shouldQuit();

  /**
   * Draws the dialogs; Save As puts a new scene into folder. Call once per frame, after the
   * panels.
   */
  static void
  render(const Path& folder);

  /**
   * Makes or opens the scene asked for during the frame. Call after the UI is drawn.
   */
  static void
  applyPending();

  /**
   * The callback runs on the main thread with each scene asset Save As makes.
   */
  NODISCARD static HEvent
  onAssetCreated(const Function<void(const SPtr<IAsset>&)>& callback);
};

} // namespace chEngineSDK
