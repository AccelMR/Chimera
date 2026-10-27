/************************************************************************/
/**
 * @file chSceneHistory.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Undo and redo of changes to the objects of a scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chScene.h"
#include "chUUID.h"

namespace chEngineSDK {

/**
 * Exists so any change to a scene's objects can be undone without each kind of change
 * knowing how: before a change, the changed object is saved with its children (the same
 * bytes a scene file holds), and when the change ends it is saved again. Undo and redo write
 * those bytes back by UUID, so every component that can be saved can also be undone.
 * Changes recorded between two commits are one entry, so a drag that changes an object every
 * frame is undone at once.
 */
class CH_CORE_EXPORT SceneHistory
{
 public:
  explicit SceneHistory(uint32 maxEntries = DEFAULT_MAX_ENTRIES);

  /**
   * Call before gameObject (or one of its children) changes or is destroyed. The first call
   * after a commit opens an entry named "<action> <object name>". Cheap when the object is
   * already saved in the open entry, so it can be called on every frame of a drag.
   */
  void
  recordChange(GameObject& gameObject, StringView action);

  /**
   * Call after gameObject was made, with its components and children.
   */
  void
  recordCreated(GameObject& gameObject, StringView action);

  /**
   * Call before the list of sublevels of scene changes. Opens an entry named action when
   * none is open.
   */
  void
  recordSubLevels(const Scene& scene, StringView action);

  NODISCARD FORCEINLINE bool
  isEntryOpen() const { return m_bEntryOpen; }

  /**
   * Renames the open entry, for a change to several objects ("Delete 3 objects").
   */
  void
  setEntryName(StringView name);

  /**
   * Saves how the recorded objects are now and closes the open entry. An entry that changed
   * nothing is dropped. Throws away what could be redone.
   */
  void
  commit(Scene& scene);

  NODISCARD bool
  canUndo() const;

  NODISCARD bool
  canRedo() const;

  /**
   * Name of the entry undo or redo would apply; empty when there is none.
   */
  NODISCARD StringView
  getUndoName() const;

  NODISCARD StringView
  getRedoName() const;

  /**
   * Commits the open entry first. outFocus gets the UUID of the entry's first object, so the
   * caller can select it. False when there was nothing to undo.
   */
  bool
  undo(Scene& scene, UUID& outFocus);

  bool
  redo(Scene& scene, UUID& outFocus);

  void
  clear();

  /**
   * The scene was saved as it is now.
   */
  void
  markSaved();

  /**
   * True when undo and redo brought the scene back to how it was saved.
   */
  NODISCARD bool
  isAtSavedState() const;

  NODISCARD FORCEINLINE uint32
  getEntryCount() const { return static_cast<uint32>(m_entries.size()); }

  static constexpr uint32 DEFAULT_MAX_ENTRIES = 100;

 private:
  /**
   * One object as it was before and after the entry. Empty bytes mean it did not exist.
   */
  struct ObjectRecord
  {
    UUID object = UUID::null();
    UUID parentBefore = UUID::null();
    UUID parentAfter = UUID::null();
    uint32 indexBefore = 0;
    uint32 indexAfter = 0;
    Vector<uint8> before;
    Vector<uint8> after;
  };

  struct Entry
  {
    String name;
    Vector<ObjectRecord> records;
    // The list of sublevels of the map, when the entry changed it.
    bool bSubLevels = false;
    Vector<SceneSubLevel> subLevelsBefore;
    Vector<SceneSubLevel> subLevelsAfter;
  };

  NODISCARD bool
  isRecorded(const GameObject& gameObject) const;

  void
  openEntry(const GameObject& gameObject, StringView action);

  void
  applyRecord(Scene& scene, const ObjectRecord& record, bool bBefore);

  Vector<Entry> m_entries;
  // Entries below it are done, those from it on can be redone.
  uint32 m_position = 0;
  // Where the history was when the scene was saved; none once that entry is dropped.
  int64 m_savedPosition = 0;
  uint32 m_maxEntries;
  Entry m_openEntry;
  bool m_bEntryOpen = false;
};

} // namespace chEngineSDK
