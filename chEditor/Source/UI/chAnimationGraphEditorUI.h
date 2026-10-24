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
class IAsset;

/**
 * Exists so an animation graph is built in the editor instead of in code: each layer is a
 * canvas with a node per state and a link per transition, with the layers, parameters and
 * clips listed beside it and the selected state or transition shown in detail. It edits a
 * copy of the graph, so nothing changes on disk (or in the objects that use the graph) until
 * Save; every change is an undo step of its own, kept while the graph stays open. A graph
 * with errors can be opened and saved, and says what is wrong, but does not play until
 * fixed. The window stays open between sessions.
 */
class AnimationGraphEditorUI
{
 public:
  static void
  render();

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
   * Releases the node editor and the open graph. Call before the ImGui context is destroyed.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
