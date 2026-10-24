/************************************************************************/
/**
 * @file chAnimationGraphEditorUI.h
 * @author AccelMR
 * @date 2026/10/24
 * @brief Window that edits an animation graph as a node canvas.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class GameObject;
class IAsset;
class ICommandList;

/**
 * Exists so an animation graph is built in the editor instead of in code: each layer is a
 * canvas with a node per state and a link per transition, with the layers, parameters and
 * clips listed beside it and the selected state or transition shown in detail. It edits a
 * copy of the graph, so nothing changes on disk (or in the objects that use the graph) until
 * Save; every change is an undo step of its own, kept while the graph stays open. A graph
 * with errors can be opened and saved, and says what is wrong, but does not play until
 * fixed. Under the canvas a model plays the graph, with test values for its parameters,
 * and the canvas marks the state it is in; during Play it can follow an animator of the
 * game instead. The window stays open between sessions.
 */
class AnimationGraphEditorUI
{
 public:
  static void
  render();

  /**
   * Plays the model preview and copies its scene for the renderer.
   */
  static void
  update(float deltaTime);

  /**
   * Draws the model preview into the texture the window showed this frame.
   */
  static void
  recordFrame(ICommandList& commandList);

  NODISCARD static bool
  canOpen(const IAsset& asset);

  /**
   * Shows the graph (loading it first) and brings the window to the front; asks first when
   * the graph already open has unsaved changes.
   */
  static void
  open(const SPtr<IAsset>& asset);

  /**
   * Forgets the asset without asking, before it is deleted.
   */
  static void
  close(const IAsset& asset);

  /**
   * True when no graph has unsaved changes, so the caller can go on at once. Otherwise it
   * asks to save them, returns false, and calls then once the answer is Save or Don't Save
   * (never after Cancel).
   */
  /**
   * During Play, opens the graph the object's Animator plays and shows that animator on
   * the canvas (its state, its live parameters) instead of the preview.
   */
  static void
  debugAnimator(const SPtr<GameObject>& object);

  NODISCARD static bool
  askToClose(Function<void()> then);

  /**
   * True while the window (or anything inside it) has the keyboard focus, so Ctrl+S, Ctrl+Z
   * and Ctrl+Y go to the graph instead of the scene.
   */
  NODISCARD static bool
  hasFocus();

  static void
  save();

  static void
  undo();

  static void
  redo();

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);

  /**
   * Releases the node editor, the open graph and the preview. Call after the GPU is idle,
   * before the ImGui context is destroyed.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
