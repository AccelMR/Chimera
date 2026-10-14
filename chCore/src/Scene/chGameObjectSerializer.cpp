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
constexpr uint32 kFirstPrefabVersion = 2;

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
  uint32 version;
  // Objects read from a prefab get new UUIDs, so instances do not share them.
  bool bNewUuids;
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
  uint32 version = 0;
  uint32 count = 0;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION ||
      stream.read(&count, sizeof(count)) != sizeof(count)) {
    return false;
  }
  ReadContext context{
      .stream = stream, .scene = scene, .version = version, .bNewUuids = false};
  return readList(context, parent, count, 0);
}

/*
 */
SPtr<GameObject>
GameObjectSerializer::instantiate(const PrefabAsset& prefab, Scene& scene, GameObject* parent)
{
  const SPtr<GameObject> gameObject = scene.createGameObject(prefab.getName(), parent);
  gameObject->m_prefab = prefab.getUUID();
  if (!readPrefabInto(prefab, *gameObject, 0)) {
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
  return readPrefabInto(prefab, target, 0);
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
  DataStream& stream = context.stream;
  GameObjectFileData data;
  UUID prefabId = UUID::null();
  String name;
  if (stream.read(&data, sizeof(data)) != sizeof(data) ||
      (context.version >= kFirstPrefabVersion &&
       stream.read(&prefabId, sizeof(prefabId)) != sizeof(prefabId)) ||
      !AssetFile::readString(stream, name, kMaxNameLength) ||
      data.componentCount > kMaxComponents) {
    return false;
  }
  // An instance stores nothing below it.
  if (!prefabId.isNull() && (data.componentCount != 0 || data.childCount != 0)) {
    return false;
  }

  const SPtr<GameObject> gameObject = context.scene.createGameObject(name, parent);
  if (!context.bNewUuids) {
    // The saved UUID, so whatever points at the object by its id finds it again.
    gameObject->m_uuid = data.uuid;
  }
  Transform& transform = gameObject->getTransform();
  transform.setLocalPosition(Vector3(data.position[0], data.position[1], data.position[2]));
  transform.setLocalRotation(
      Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]));
  transform.setLocalScale(Vector3(data.scale[0], data.scale[1], data.scale[2]));

  if (!prefabId.isNull()) {
    // Kept even when the prefab is missing, so saving again keeps the link.
    gameObject->m_prefab = prefabId;
    const SPtr<PrefabAsset> prefab = findPrefab(prefabId);
    if (!prefab) {
      CH_LOG_WARNING(SceneManagerLog, "{0} lost its prefab {1}.", name, prefabId);
    }
    else if (!readPrefabInto(*prefab, *gameObject, depth + 1)) {
      // A broken prefab leaves its instances empty instead of failing the whole file.
      CH_LOG_WARNING(SceneManagerLog, "{0} could not read its prefab {1}.", name,
                     prefab->getName());
    }
    return true;
  }

  for (uint32 i = 0; i < data.componentCount; ++i) {
    if (!readComponent(context, *gameObject)) {
      return false;
    }
  }
  return readList(context, gameObject.get(), data.childCount, depth + 1);
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
 * its UUID, name and transform are left out.
 */
bool
GameObjectSerializer::readPrefabInto(const PrefabAsset& prefab,
                                     GameObject& target,
                                     uint32 depth)
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
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION ||
      stream.read(&count, sizeof(count)) != sizeof(count) || count != 1) {
    return false;
  }

  GameObjectFileData data;
  UUID nestedPrefab = UUID::null();
  String name;
  if (stream.read(&data, sizeof(data)) != sizeof(data) ||
      (version >= kFirstPrefabVersion &&
       stream.read(&nestedPrefab, sizeof(nestedPrefab)) != sizeof(nestedPrefab)) ||
      !AssetFile::readString(stream, name, kMaxNameLength) ||
      data.componentCount > kMaxComponents) {
    return false;
  }

  // A prefab whose root is an instance of another takes that one's content.
  if (!nestedPrefab.isNull()) {
    const SPtr<PrefabAsset> nested = findPrefab(nestedPrefab);
    return nested ? readPrefabInto(*nested, target, depth + 1) : true;
  }

  ReadContext context{.stream = stream,
                      .scene = *target.getScene(),
                      .version = version,
                      .bNewUuids = true};
  for (uint32 i = 0; i < data.componentCount; ++i) {
    if (!readComponent(context, target)) {
      return false;
    }
  }
  return readList(context, &target, data.childCount, depth + 1);
}

} // namespace chEngineSDK
