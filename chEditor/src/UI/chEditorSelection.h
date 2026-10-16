/************************************************************************/
/**
 * @file chEditorSelection.h
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Selection management for the Chimera Editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGameObject.h"

namespace chEngineSDK {

/**
 * Exists so every editor window agrees on what is selected: a list of GameObjects of the open
 * scene and, among them, the active one (the last selected), which the Inspector shows and
 * the gizmo stands on. Operations on several objects (delete, duplicate, move) act on the
 * top-level ones, since a selected parent already brings its children.
 */
class EditorSelection
{
 public:
  /**
   * The active object, null when nothing is selected.
   */
  NODISCARD static const SPtr<GameObject>&
  getSelectedGameObject();

  /**
   * Selects only gameObject; null clears the selection.
   */
  static void
  setSelectedGameObject(const SPtr<GameObject>& gameObject);

  /**
   * In the order they were selected; the active one is last.
   */
  NODISCARD static const Vector<SPtr<GameObject>>&
  getSelection();

  NODISCARD static bool
  isSelected(const GameObject& gameObject);

  /**
   * Adds gameObject, or moves it to the end when already selected, and makes it active.
   */
  static void
  add(const SPtr<GameObject>& gameObject);

  /**
   * Removes gameObject when selected, else adds it.
   */
  static void
  toggle(const SPtr<GameObject>& gameObject);

  static void
  clear();

  /**
   * Replaces the contents of outObjects with the selected objects that have no selected
   * parent, in selection order.
   */
  static void
  getTopLevel(Vector<SPtr<GameObject>>& outObjects);

  /**
   * Selects every object below root (the whole scene when null) except the content of
   * prefab instances, which cannot be changed on its own.
   */
  static void
  selectAll(const Scene& scene, const GameObject* root);

  /**
   * Drops the objects that left their scene (deleted, or replaced by an undo).
   */
  static void
  removeDetached();
};

} // namespace chEngineSDK
