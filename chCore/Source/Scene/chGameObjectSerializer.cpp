/************************************************************************/
/**
 * @file chGameObjectSerializer.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Saves and loads GameObject hierarchies with their components.
 */
/************************************************************************/
#include "chGameObjectSerializer.h"

#include "chAlgorithm.h"
#include "chAssetFile.h"
#include "chAssetManager.h"
#include "chComponentRegister.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chLogger.h"
#include "chPrefabAsset.h"
#include "chScene.h"
#include "chSceneManager.h"

namespace chEngineSDK {
namespace {
// Above any real file; only a broken one asks for more.
constexpr uint32 kMaxObjectsPerList = 1u << 20;
constexpr uint32 kMaxComponents = 1024;
constexpr uint32 kMaxNameLength = 1024;
// Keeps a broken file, or prefabs that hold each other, from recursing until the stack runs
// out.
constexpr uint32 kMaxDepth = 1024;

struct GameObjectFileData
{
  UUID uuid;
  float position[3];
  float rotation[4];
  float scale[3];
  uint32 componentCount;
  uint32 childCount;
};
static_assert(sizeof(GameObjectFileData) == 64, "GameObjectFileData must have no padding");

struct ComponentFileData
{
  UUID typeId;
  uint32 enabled;
  // Bytes of the component's own data, which follow.
  uint32 size;
};
static_assert(sizeof(ComponentFileData) == 24, "ComponentFileData must have no padding");

/*
 */
void
writeComponent(DataStream& stream, const Component& component)
{
  ComponentFileData header{.typeId = component.getTypeId(),
                           .enabled = component.isEnabled() ? 1u : 0u,
                           .size = 0};
  const SIZE_T headerStart = stream.tell();
  stream.write(&header, sizeof(header));
  const SIZE_T dataStart = stream.tell();
  component.serialize(stream);
  const SIZE_T dataEnd = stream.tell();

  header.size = static_cast<uint32>(dataEnd - dataStart);
  stream.seek(headerStart);
  stream.write(&header, sizeof(header));
  stream.seek(dataEnd);
}

/*
 * An instance writes its link and nothing below it: its content belongs to the prefab.
 */
void
writeObject(DataStream& stream, const GameObject& gameObject)
{
  const bool bInstance = !gameObject.getPrefab().isNull();
  const Transform& transform = gameObject.getTransform();
  const Vector3& position = transform.getLocalPosition();
  const Quaternion& rotation = transform.getLocalRotation();
  const Vector3& scale = transform.getLocalScale();
  const GameObjectFileData data{
      .uuid = gameObject.getUUID(),
      .position = {position.x, position.y, position.z},
      .rotation = {rotation.x, rotation.y, rotation.z, rotation.w},
      .scale = {scale.x, scale.y, scale.z},
      .componentCount =
          bInstance ? 0 : static_cast<uint32>(gameObject.getComponents().size()),
      .childCount = bInstance ? 0 : static_cast<uint32>(gameObject.getChildren().size())};
  stream.write(&data, sizeof(data));
  const UUID& prefab = gameObject.getPrefab();
  stream.write(&prefab, sizeof(prefab));
  AssetFile::writeString(stream, gameObject.getName());
  if (bInstance) {
    return;
  }

  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    writeComponent(stream, *component);
  }
  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    writeObject(stream, *child);
  }
}

/*
 */
void
addReference(const UUID& reference, Vector<UUID>& outReferences)
{
  if (!reference.isNull() && !Algorithm::contains(outReferences, reference)) {
    outReferences.push_back(reference);
  }
}

/*
 */
void
collectObjectReferences(const GameObject& gameObject,
                        Vector<UUID>& scratch,
                        Vector<UUID>& outReferences)
{
  // The prefab brings what its content uses.
  if (!gameObject.getPrefab().isNull()) {
    addReference(gameObject.getPrefab(), outReferences);
    return;
  }

  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    scratch.clear();
    component->collectReferences(scratch);
    for (const UUID& reference : scratch) {
      addReference(reference, outReferences);
    }
  }
  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    collectObjectReferences(*child, scratch, outReferences);
  }
}

/*
 * Points the references the components of gameObject and its children hold at the copies
 * read with them.
 */
void
remapReferences(GameObject& gameObject, const UnorderedMap<UUID, UUID>& newIds)
{
  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    component->remapObjectReferences(newIds);
  }
  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    remapReferences(*child, newIds);
  }
}

/*
 * Null when there is no such prefab or it cannot load. Without an AssetManager (tests)
 * nothing is found.
 */
SPtr<PrefabAsset>
findPrefab(const UUID& id)
{
  if (id.isNull() || !AssetManager::isStarted()) {
    return nullptr;
  }
  AssetManager& assetManager = AssetManager::instance();
  const SPtr<IAsset> asset = assetManager.getAsset(id);
  SPtr<PrefabAsset> prefab = asset ? asset->as<PrefabAsset>() : nullptr;
  if (prefab && !prefab->isLoaded() && !assetManager.syncLoadAsset(prefab)) {
    return nullptr;
  }
  return prefab;
}
} // namespace

struct GameObjectSerializer::ReadContext
{
  DataStream& stream;
  Scene& scene;
  // Objects read from a prefab get new UUIDs, so instances do not share them.
  bool bNewUuids;
  // With bNewUuids, the saved UUID of each object read to its new one.
  UnorderedMap<UUID, UUID>* newIds = nullptr;
};

struct GameObjectSerializer::ObjectHeader
{
  GameObjectFileData data;
  UUID prefab;
  String name;
};

/*
 */
void
GameObjectSerializer::write(DataStream& stream, Span<const SPtr<GameObject>> gameObjects)
{
  const uint32 version = SERIALIZATION_VERSION;
  const uint32 count = static_cast<uint32>(gameObjects.size());
  stream.write(&version, sizeof(version));
  stream.write(&count, sizeof(count));
  for (const SPtr<GameObject>& gameObject : gameObjects) {
    writeObject(stream, *gameObject);
  }
}

/*
 */
void
GameObjectSerializer::write(DataStream& stream, const GameObject& gameObject)
{
  const uint32 version = SERIALIZATION_VERSION;
  const uint32 count = 1;
  stream.write(&version, sizeof(version));
  stream.write(&count, sizeof(count));
  writeObject(stream, gameObject);
}

/*
 */
bool
GameObjectSerializer::read(DataStream& stream, Scene& scene, GameObject* parent)
{
  return readStream(stream, scene, parent, false);
}

/*
 */
bool
GameObjectSerializer::readCopies(DataStream& stream, Scene& scene, GameObject* parent)
{
  return readStream(stream, scene, parent, true);
}

/*
 */
bool
GameObjectSerializer::copyScene(const Scene& source, Scene& target)
{
  target.setMapLevel(source.getLevels()[0].name, source.getLevels()[0].asset);
  target.setSubLevels(source.getSubLevels());
  MemoryDataStream written;
  write(written, source.getRootGameObjects());
  // Only read, so the bytes are never changed through the stream.
  MemoryDataStream reader(written.getStartPtr(), written.size(), false);
  return read(reader, target, nullptr);
}

/*
 */
bool
GameObjectSerializer::readStream(DataStream& stream,
                                 Scene& scene,
                                 GameObject* parent,
                                 bool bNewUuids)
{
  uint32 version = 0;
  uint32 count = 0;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION ||
      stream.read(&count, sizeof(count)) != sizeof(count)) {
    return false;
  }
  UnorderedMap<UUID, UUID> newIds;
  ReadContext context{.stream = stream,
                      .scene = scene,
                      .bNewUuids = bNewUuids,
                      .newIds = bNewUuids ? &newIds : nullptr};
  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->getChildren() : scene.getRootGameObjects();
  const SIZE_T firstNew = siblings.size();
  const bool bRead = readList(context, parent, count, 0);
  if (bNewUuids) {
    for (SIZE_T i = firstNew; i < siblings.size(); ++i) {
      remapReferences(*siblings[i], newIds);
    }
  }
  return bRead;
}

/*
 */
bool
GameObjectSerializer::readInto(DataStream& stream, GameObject& target)
{
  CH_ASSERT(target.getScene() && "The target must be in a scene.");
  Scene& scene = *target.getScene();
  uint32 version = 0;
  uint32 count = 0;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION ||
      stream.read(&count, sizeof(count)) != sizeof(count) || count != 1) {
    return false;
  }
  ReadContext context{.stream = stream, .scene = scene, .bNewUuids = false};
  ObjectHeader header;
  if (!readObjectHeader(context, header)) {
    return false;
  }

  // From the back, so nothing is shifted while removing.
  while (!target.m_components.empty()) {
    target.removeComponent(*target.m_components.back());
  }
  while (!target.m_children.empty()) {
    scene.destroyGameObject(*target.m_children.back());
  }
  target.setName(header.name);
  target.m_prefab = UUID::null();
  return fillObject(context, target, header, 0);
}

/*
 */
SPtr<GameObject>
GameObjectSerializer::duplicate(const GameObject& source)
{
  SPtr<GameObject> copy = copyTo(source, source.m_parent);
  if (copy) {
    source.getScene()->setSiblingIndex(*copy, source.getScene()->getSiblingIndex(source) + 1);
  }
  return copy;
}

/*
 */
SPtr<GameObject>
GameObjectSerializer::copyTo(const GameObject& source, GameObject* parent)
{
  CH_ASSERT(source.getScene() && "The source must be in a scene.");
  Scene& scene = *source.getScene();
  CH_ASSERT((!parent || parent->getScene() == &scene) && "The parent must be in that scene.");
  MemoryDataStream written;
  write(written, source);

  // Only read, so the bytes are never changed through the stream.
  MemoryDataStream stream(written.getStartPtr(), written.size(), false);
  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->m_children : scene.getRootGameObjects();
  const SIZE_T siblingCount = siblings.size();
  const bool bRead = readCopies(stream, scene, parent);
  if (siblings.size() == siblingCount) {
    return nullptr;
  }

  SPtr<GameObject> copy = siblings.back();
  if (!bRead) {
    // What was read is half an object, not a copy.
    scene.destroyGameObject(*copy);
    return nullptr;
  }
  return copy;
}

/*
 */
SPtr<GameObject>
GameObjectSerializer::instantiate(const PrefabAsset& prefab, Scene& scene, GameObject* parent)
{
  const SPtr<GameObject> gameObject = scene.createGameObject(prefab.getName(), parent);
  gameObject->m_prefab = prefab.getUUID();
  if (!readPrefabInto(prefab, *gameObject, 0, true)) {
    CH_LOG_WARNING(SceneManagerLog, "Could not read the content of prefab {0}.",
                   prefab.getName());
  }
  return gameObject;
}

/*
 */
bool
GameObjectSerializer::readPrefabContent(const PrefabAsset& prefab, GameObject& target)
{
  return readPrefabInto(prefab, target, 0, true);
}

/*
 */
void
GameObjectSerializer::collectReferences(Span<const SPtr<GameObject>> gameObjects,
                                        Vector<UUID>& outReferences)
{
  Vector<UUID> scratch;
  for (const SPtr<GameObject>& gameObject : gameObjects) {
    collectObjectReferences(*gameObject, scratch, outReferences);
  }
}

/*
 */
void
GameObjectSerializer::collectReferences(const GameObject& gameObject,
                                        Vector<UUID>& outReferences)
{
  Vector<UUID> scratch;
  collectObjectReferences(gameObject, scratch, outReferences);
}

/*
 */
bool
GameObjectSerializer::readList(ReadContext& context,
                               GameObject* parent,
                               uint32 count,
                               uint32 depth)
{
  if (count > kMaxObjectsPerList || depth > kMaxDepth) {
    return false;
  }
  for (uint32 i = 0; i < count; ++i) {
    if (!readObject(context, parent, depth)) {
      return false;
    }
  }
  return true;
}

/*
 */
bool
GameObjectSerializer::readObject(ReadContext& context, GameObject* parent, uint32 depth)
{
  ObjectHeader header;
  if (!readObjectHeader(context, header)) {
    return false;
  }

  const SPtr<GameObject> gameObject = context.scene.createGameObject(header.name, parent);
  if (!context.bNewUuids) {
    // The saved UUID, so whatever points at the object by its id finds it again.
    gameObject->m_uuid = header.data.uuid;
  }
  else if (context.newIds) {
    (*context.newIds)[header.data.uuid] = gameObject->getUUID();
  }
  return fillObject(context, *gameObject, header, depth);
}

/*
 */
bool
GameObjectSerializer::readObjectHeader(ReadContext& context, ObjectHeader& outHeader)
{
  DataStream& stream = context.stream;
  outHeader.prefab = UUID::null();
  if (stream.read(&outHeader.data, sizeof(outHeader.data)) != sizeof(outHeader.data) ||
      stream.read(&outHeader.prefab, sizeof(outHeader.prefab)) != sizeof(outHeader.prefab) ||
      !AssetFile::readString(stream, outHeader.name, kMaxNameLength) ||
      outHeader.data.componentCount > kMaxComponents) {
    return false;
  }
  // An instance stores nothing below it.
  return outHeader.prefab.isNull() ||
         (outHeader.data.componentCount == 0 && outHeader.data.childCount == 0);
}

/*
 */
bool
GameObjectSerializer::fillObject(ReadContext& context,
                                 GameObject& gameObject,
                                 const ObjectHeader& header,
                                 uint32 depth)
{
  const GameObjectFileData& data = header.data;
  Transform& transform = gameObject.getTransform();
  transform.setLocalPosition(Vector3(data.position[0], data.position[1], data.position[2]));
  transform.setLocalRotation(
      Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]));
  transform.setLocalScale(Vector3(data.scale[0], data.scale[1], data.scale[2]));

  if (!header.prefab.isNull()) {
    // Kept even when the prefab is missing, so saving again keeps the link.
    gameObject.m_prefab = header.prefab;
    const SPtr<PrefabAsset> prefab = findPrefab(header.prefab);
    if (!prefab) {
      CH_LOG_WARNING(SceneManagerLog, "{0} lost its prefab {1}.", header.name, header.prefab);
    }
    else if (!readPrefabInto(*prefab, gameObject, depth + 1, false)) {
      // A broken prefab leaves its instances empty instead of failing the whole file.
      CH_LOG_WARNING(SceneManagerLog, "{0} could not read its prefab {1}.", header.name,
                     prefab->getName());
    }
    return true;
  }

  for (uint32 i = 0; i < data.componentCount; ++i) {
    if (!readComponent(context, gameObject)) {
      return false;
    }
  }
  return readList(context, &gameObject, data.childCount, depth + 1);
}

/*
 */
bool
GameObjectSerializer::readComponent(ReadContext& context, GameObject& gameObject)
{
  DataStream& stream = context.stream;
  ComponentFileData header;
  if (stream.read(&header, sizeof(header)) != sizeof(header)) {
    return false;
  }
  const SIZE_T dataStart = stream.tell();
  const SIZE_T dataEnd = dataStart + header.size;
  if (dataEnd > stream.size()) {
    return false;
  }

  UniquePtr<Component> component = ComponentRegister::create(header.typeId);
  if (!component) {
    CH_LOG_WARNING(SceneManagerLog,
                   "{0} has a component of an unknown type ({1}); it is left out.",
                   gameObject.getName(), header.typeId);
  }
  else if (!component->deserialize(stream)) {
    CH_LOG_WARNING(SceneManagerLog, "{0} has a {1} that could not be read; it is left out.",
                   gameObject.getName(), ComponentRegister::getTypeName(header.typeId));
    component.reset();
  }
  // From the stored size, so a component that reads less (or nothing) leaves the stream
  // where the next one starts.
  stream.seek(dataEnd);

  if (component) {
    // Before attaching, so a disabled component never registers.
    component->setEnabled(header.enabled != 0);
    gameObject.attachComponent(std::move(component), header.typeId);
  }
  return true;
}

/*
 * The content is one object written by write(); its components and children go to target,
 * its UUID, name and position are left out. Its rotation and scale only start a new
 * instance: an instance read from a scene keeps the ones the scene saved.
 */
bool
GameObjectSerializer::readPrefabInto(const PrefabAsset& prefab,
                                     GameObject& target,
                                     uint32 depth,
                                     bool bApplyRootTransform)
{
  CH_ASSERT(target.getScene() && "The target must be in a scene.");
  if (depth > kMaxDepth) {
    return false;
  }
  const Span<const uint8> content = prefab.getContent();
  if (content.empty()) {
    return true;
  }

  // Only read, so the content is never changed through the stream.
  MemoryDataStream stream(const_cast<uint8*>(content.data()), content.size(), false);
  uint32 version = 0;
  uint32 count = 0;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION ||
      stream.read(&count, sizeof(count)) != sizeof(count) || count != 1) {
    return false;
  }

  GameObjectFileData data;
  UUID nestedPrefab = UUID::null();
  String name;
  if (stream.read(&data, sizeof(data)) != sizeof(data) ||
      stream.read(&nestedPrefab, sizeof(nestedPrefab)) != sizeof(nestedPrefab) ||
      !AssetFile::readString(stream, name, kMaxNameLength) ||
      data.componentCount > kMaxComponents) {
    return false;
  }

  if (bApplyRootTransform) {
    Transform& transform = target.getTransform();
    transform.setLocalRotation(
        Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]));
    transform.setLocalScale(Vector3(data.scale[0], data.scale[1], data.scale[2]));
  }

  // A prefab whose root is an instance of another takes that one's content.
  if (!nestedPrefab.isNull()) {
    const SPtr<PrefabAsset> nested = findPrefab(nestedPrefab);
    return nested ? readPrefabInto(*nested, target, depth + 1, false) : true;
  }

  // The root's components land on target, so references to the root point at it.
  UnorderedMap<UUID, UUID> newIds;
  newIds[data.uuid] = target.getUUID();
  ReadContext context{.stream = stream,
                      .scene = *target.getScene(),
                      .bNewUuids = true,
                      .newIds = &newIds};
  bool bRead = true;
  for (uint32 i = 0; i < data.componentCount && bRead; ++i) {
    bRead = readComponent(context, target);
  }
  bRead = bRead && readList(context, &target, data.childCount, depth + 1);
  remapReferences(target, newIds);
  return bRead;
}

} // namespace chEngineSDK
