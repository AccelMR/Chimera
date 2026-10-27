/************************************************************************/
/**
 * @file chSceneHistory.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Undo and redo of changes to the objects of a scene.
 */
/************************************************************************/
#include "chSceneHistory.h"

#include "chAlgorithm.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chGameObjectSerializer.h"
#include "chLogger.h"
#include "chScene.h"
#include "chSceneManager.h"

#include <cstring>

namespace chEngineSDK {
namespace {
constexpr int64 kNoSavedPosition = -1;

/*
 */
void
saveObject(const GameObject& gameObject, Vector<uint8>& outBytes)
{
  MemoryDataStream stream;
  GameObjectSerializer::write(stream, gameObject);
  outBytes.resize(stream.size());
  std::memcpy(outBytes.data(), stream.getStartPtr(), outBytes.size());
}

/*
 */
const UUID&
getParentId(const GameObject& gameObject)
{
  const GameObject* parent = gameObject.getParent();
  return parent ? parent->getUUID() : UUID::null();
}
} // namespace

/*
 */
SceneHistory::SceneHistory(uint32 maxEntries)
 : m_maxEntries(maxEntries > 0 ? maxEntries : 1)
{}

/*
 */
void
SceneHistory::recordChange(GameObject& gameObject, StringView action)
{
  if (m_bEntryOpen && isRecorded(gameObject)) {
    return;
  }
  if (!m_bEntryOpen) {
    openEntry(gameObject, action);
  }

  ObjectRecord& record = m_openEntry.records.emplace_back();
  record.object = gameObject.getUUID();
  record.parentBefore = getParentId(gameObject);
  record.indexBefore = gameObject.getScene()->getSiblingIndex(gameObject);
  saveObject(gameObject, record.before);
}

/*
 */
void
SceneHistory::recordCreated(GameObject& gameObject, StringView action)
{
  if (m_bEntryOpen && isRecorded(gameObject)) {
    return;
  }
  if (!m_bEntryOpen) {
    openEntry(gameObject, action);
  }
  // What it holds is saved on commit, once it is complete.
  m_openEntry.records.emplace_back().object = gameObject.getUUID();
}

/*
 */
void
SceneHistory::recordSubLevels(const Scene& scene, StringView action)
{
  if (!m_bEntryOpen) {
    m_bEntryOpen = true;
    m_openEntry.name.assign(action);
  }
  if (!m_openEntry.bSubLevels) {
    m_openEntry.bSubLevels = true;
    m_openEntry.subLevelsBefore = scene.getSubLevels();
  }
}

/*
 */
void
SceneHistory::setEntryName(StringView name)
{
  if (m_bEntryOpen) {
    m_openEntry.name.assign(name);
  }
}

/*
 */
void
SceneHistory::commit(Scene& scene)
{
  if (!m_bEntryOpen) {
    return;
  }
  m_bEntryOpen = false;

  for (ObjectRecord& record : m_openEntry.records) {
    const SPtr<GameObject> gameObject = scene.findGameObject(record.object);
    if (gameObject) {
      record.parentAfter = getParentId(*gameObject);
      record.indexAfter = scene.getSiblingIndex(*gameObject);
      saveObject(*gameObject, record.after);
    }
  }
  // Set a value and back, or made and destroyed in the same entry.
  Algorithm::removeAllIf(m_openEntry.records, [](const ObjectRecord& record) {
    return record.before == record.after && record.parentBefore == record.parentAfter &&
           record.indexBefore == record.indexAfter;
  });
  if (m_openEntry.bSubLevels) {
    m_openEntry.subLevelsAfter = scene.getSubLevels();
    m_openEntry.bSubLevels = m_openEntry.subLevelsBefore != m_openEntry.subLevelsAfter;
  }
  if (m_openEntry.records.empty() && !m_openEntry.bSubLevels) {
    m_openEntry = Entry{};
    return;
  }

  // What could be redone is gone, the saved state with it if it was there.
  m_entries.resize(m_position);
  if (m_savedPosition > static_cast<int64>(m_position)) {
    m_savedPosition = kNoSavedPosition;
  }
  m_entries.push_back(std::move(m_openEntry));
  m_openEntry = Entry{};
  ++m_position;

  if (m_entries.size() > m_maxEntries) {
    m_entries.erase(m_entries.begin());
    --m_position;
    if (m_savedPosition != kNoSavedPosition) {
      --m_savedPosition;
    }
  }
}

/*
 */
bool
SceneHistory::canUndo() const
{
  return m_bEntryOpen || m_position > 0;
}

/*
 */
bool
SceneHistory::canRedo() const
{
  return !m_bEntryOpen && m_position < m_entries.size();
}

/*
 */
StringView
SceneHistory::getUndoName() const
{
  if (m_bEntryOpen) {
    return m_openEntry.name;
  }
  return m_position > 0 ? StringView(m_entries[m_position - 1].name) : StringView();
}

/*
 */
StringView
SceneHistory::getRedoName() const
{
  return canRedo() ? StringView(m_entries[m_position].name) : StringView();
}

/*
 * The records go back in the opposite order they were made, so an object recorded inside
 * one recorded later ends as it was before both.
 */
bool
SceneHistory::undo(Scene& scene, UUID& outFocus)
{
  commit(scene);
  if (m_position == 0) {
    return false;
  }
  --m_position;
  const Entry& entry = m_entries[m_position];
  for (SIZE_T i = entry.records.size(); i > 0; --i) {
    applyRecord(scene, entry.records[i - 1], true);
  }
  if (entry.bSubLevels) {
    scene.setSubLevels(entry.subLevelsBefore);
  }
  outFocus = entry.records.empty() ? UUID::null() : entry.records.front().object;
  return true;
}

/*
 */
bool
SceneHistory::redo(Scene& scene, UUID& outFocus)
{
  commit(scene);
  if (m_position >= m_entries.size()) {
    return false;
  }
  const Entry& entry = m_entries[m_position];
  ++m_position;
  for (const ObjectRecord& record : entry.records) {
    applyRecord(scene, record, false);
  }
  if (entry.bSubLevels) {
    scene.setSubLevels(entry.subLevelsAfter);
  }

  // The places after the change were all saved at commit, so they are set again from the
  // lowest up; set in record order, a later move would shift one already placed.
  if (entry.records.size() > 1) {
    Vector<const ObjectRecord*> byPlace;
    byPlace.reserve(entry.records.size());
    for (const ObjectRecord& record : entry.records) {
      byPlace.push_back(&record);
    }
    Algorithm::sort(byPlace, [](const ObjectRecord* a, const ObjectRecord* b) {
      return a->indexAfter < b->indexAfter;
    });
    for (const ObjectRecord* record : byPlace) {
      const SPtr<GameObject> gameObject = scene.findGameObject(record->object);
      if (gameObject && !record->after.empty() &&
          getParentId(*gameObject) == record->parentAfter) {
        scene.setSiblingIndex(*gameObject, record->indexAfter);
      }
    }
  }
  outFocus = entry.records.empty() ? UUID::null() : entry.records.front().object;
  return true;
}

/*
 */
void
SceneHistory::clear()
{
  m_entries.clear();
  m_openEntry = Entry{};
  m_bEntryOpen = false;
  m_position = 0;
  m_savedPosition = 0;
}

/*
 */
void
SceneHistory::markSaved()
{
  m_savedPosition = m_position;
}

/*
 */
bool
SceneHistory::isAtSavedState() const
{
  return !m_bEntryOpen && m_savedPosition == static_cast<int64>(m_position);
}

/*
 * A recorded parent already holds the object in its bytes.
 */
bool
SceneHistory::isRecorded(const GameObject& gameObject) const
{
  for (const GameObject* current = &gameObject; current; current = current->getParent()) {
    for (const ObjectRecord& record : m_openEntry.records) {
      if (record.object == current->getUUID()) {
        return true;
      }
    }
  }
  return false;
}

/*
 */
void
SceneHistory::openEntry(const GameObject& gameObject, StringView action)
{
  m_bEntryOpen = true;
  m_openEntry.name.assign(action);
  m_openEntry.name += ' ';
  m_openEntry.name += gameObject.getName();
}

/*
 * An object that exists under the same parent is read again in place, so whatever holds it
 * (the selection, the editor) keeps a live object. Otherwise it is made again with its saved
 * UUID at its saved place.
 */
void
SceneHistory::applyRecord(Scene& scene, const ObjectRecord& record, bool bBefore)
{
  const Vector<uint8>& bytes = bBefore ? record.before : record.after;
  const UUID& parentId = bBefore ? record.parentBefore : record.parentAfter;
  const uint32 index = bBefore ? record.indexBefore : record.indexAfter;

  SPtr<GameObject> existing = scene.findGameObject(record.object);
  if (existing && (bytes.empty() || getParentId(*existing) != parentId)) {
    scene.destroyGameObject(*existing);
    existing.reset();
  }
  if (bytes.empty()) {
    return;
  }

  // Only read, so the bytes are never changed through the stream.
  MemoryDataStream stream(const_cast<uint8*>(bytes.data()), bytes.size(), false);
  if (existing) {
    if (!GameObjectSerializer::readInto(stream, *existing)) {
      CH_LOG_ERROR(SceneManagerLog, "Could not restore {0}.", existing->getName());
    }
    // A reorder among the same siblings only changed the place.
    scene.setSiblingIndex(*existing, index);
    return;
  }

  SPtr<GameObject> parent;
  if (!parentId.isNull()) {
    parent = scene.findGameObject(parentId);
    if (!parent) {
      CH_LOG_ERROR(SceneManagerLog, "Could not restore an object: its parent is gone.");
      return;
    }
  }
  if (!GameObjectSerializer::read(stream, scene, parent.get())) {
    CH_LOG_ERROR(SceneManagerLog, "Could not restore an object.");
  }
  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->getChildren() : scene.getRootGameObjects();
  if (!siblings.empty() && siblings.back()->getUUID() == record.object) {
    scene.setSiblingIndex(*siblings.back(), index);
  }
}

} // namespace chEngineSDK
