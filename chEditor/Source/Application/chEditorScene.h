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
   * Call before every change to gameObject, its components or its children, also before
   * destroying it, so it can be undone. action names the change in the Edit menu ("Move").
   * Changes recorded until the edit ends (applyPending) are undone together.
   */
  static void
  recordChange(GameObject& gameObject, StringView action);

  /**
   * Call after making gameObject, once its components and children are in place.
   */
  static void
  recordCreated(GameObject& gameObject, StringView action);

  NODISCARD static bool
  canUndo();

  NODISCARD static bool
  canRedo();

  NODISCARD static StringView
  getUndoName();

  NODISCARD static StringView
  getRedoName();

  /**
   * Undo and redo run in applyPending and select the object they changed.
   */
  static void
  requestUndo();

  static void
  requestRedo();

  /**
   * False for the prefab's root in prefab mode and for objects inside a prefab instance.
   */
  NODISCARD static bool
  canDuplicateOrDelete(GameObject& gameObject);

  /**
   * The requests below act on the top-level selected objects in applyPending, once no panel
   * is walking the scene or the asset list, and skip the objects they cannot change. Each is
   * one undo step.
   *
   * Duplicate puts each copy right after its source, named "Name (1)", and selects the
   * copies.
   */
  static void
  requestDuplicate();

  static void
  requestDelete();

  /**
   * Makes a prefab in folder from each object (see createPrefab).
   */
  static void
  requestCreatePrefabs(const Path& folder);

  /**
   * Moves the objects under newParent (null for the scene's root): before or after sibling
   * when given (one of newParent's children), else last.
   */
  static void
  requestReparent(const SPtr<GameObject>& newParent,
                  const SPtr<GameObject>& sibling,
                  bool bAfterSibling);

  /**
   * Keeps a copy of the top-level selected objects; it survives opening another scene.
   */
  static void
  copySelection();

  NODISCARD static bool
  canPaste();

  /**
   * Adds the copied objects after the active object, under its parent (under the default
   * parent when nothing is selected), and selects them.
   */
  static void
  requestPaste();

  NODISCARD static bool
  isDirty();

  /**
   * Changes whenever the scene, its name or its dirty flag changes, so a title can be built
   * again only then.
   */
  NODISCARD static uint32
  getRevision();

  /**
   * The scene's name, with "*" when it has unsaved changes, and whether it is playing.
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
   * Play runs a copy of the scene, so nothing done while playing stays: Stop throws the copy
   * away and the scene is shown as it was, with the same objects selected. Both run in
   * applyPending. While playing getScene() is the copy, edits are not recorded for undo and
   * saving waits for Stop; New, Open and the other scene requests stop first. There is no
   * Play in prefab mode.
   */
  NODISCARD static bool
  canPlay();

  NODISCARD static bool
  isPlaying();

  static void
  requestPlay();

  static void
  requestStop();

  /**
   * A paused game does not update; requestStep lets it run one frame.
   */
  NODISCARD static bool
  isPaused();

  static void
  setPaused(bool bPaused);

  static void
  requestStep();

  /**
   * True once after requestStep, for the frame that runs the paused game one step.
   */
  NODISCARD static bool
  takeStep();

  /**
   * While playing the viewport looks through the scene's camera; ejected (F8), the editor
   * camera flies around the game instead and the viewport tools work.
   */
  NODISCARD static bool
  isEjected();

  static void
  setEjected(bool bEjected);

  /**
   * True when the editor can close now. Otherwise it asks about the unsaved changes first,
   * and shouldQuit becomes true once they are saved or discarded.
   */
  NODISCARD static bool
  requestQuit();

  NODISCARD static bool
  shouldQuit();

  /**
   * Closes the editor to open projectFile in a new run, asking about unsaved changes first
   * like a quit. Once shouldQuit is true, getProjectToOpen gives the project.
   */
  static void
  requestOpenProject(const Path& projectFile);

  /**
   * Empty unless the editor closes to open another project.
   */
  NODISCARD static const Path&
  getProjectToOpen();

  /**
   * Draws the dialogs; Save As puts a new scene into folder. Call once per frame, after the
   * panels.
   */
  static void
  render(const Path& folder);

  /**
   * Runs the undo or redo and makes or opens the scene asked for during the frame. Call after
   * the UI is drawn. While bEditing (a drag or a text field still going on) the recorded
   * changes stay one open edit; otherwise they become one undo step.
   */
  static void
  applyPending(bool bEditing);

  /**
   * The callback runs on the main thread with each scene asset Save As makes.
   */
  NODISCARD static HEvent
  onAssetCreated(const Function<void(const SPtr<IAsset>&)>& callback);
};

} // namespace chEngineSDK
