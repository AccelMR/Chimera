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
#include "chComponentRegister.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chLogger.h"
#include "chScene.h"
#include "chSceneManager.h"

namespace chEngineSDK {
namespace {
// Above any real file; only a broken one asks for more.
constexpr uint32 kMaxObjectsPerList = 1u << 20;
constexpr uint32 kMaxComponents = 1024;
constexpr uint32 kMaxNameLength = 1024;
// Keeps a broken file from recursing until the stack runs out.
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
 */
void
writeObject(DataStream& stream, const GameObject& gameObject)
{
  const Transform& transform = gameObject.getTransform();
  const Vector3& position = transform.getLocalPosition();
  const Quaternion& rotation = transform.getLocalRotation();
  const Vector3& scale = transform.getLocalScale();
  const GameObjectFileData data{
      .uuid = gameObject.getUUID(),
      .position = {position.x, position.y, position.z},
      .rotation = {rotation.x, rotation.y, rotation.z, rotation.w},
      .scale = {scale.x, scale.y, scale.z},
      .componentCount = static_cast<uint32>(gameObject.getComponents().size()),
      .childCount = static_cast<uint32>(gameObject.getChildren().size())};
  stream.write(&data, sizeof(data));
  AssetFile::writeString(stream, gameObject.getName());

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
collectObjectReferences(const GameObject& gameObject,
                        Vector<UUID>& scratch,
                        Vector<UUID>& outReferences)
{
  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    scratch.clear();
    component->collectReferences(scratch);
    for (const UUID& reference : scratch) {
      if (!reference.isNull() && !Algorithm::contains(outReferences, reference)) {
        outReferences.push_back(reference);
      }
    }
  }
  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    collectObjectReferences(*child, scratch, outReferences);
  }
}
} // namespace

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
  return GameObjectSerializer::readList(stream, scene, parent, count, 0);
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
bool
GameObjectSerializer::readList(DataStream& stream,
                               Scene& scene,
                               GameObject* parent,
                               uint32 count,
                               uint32 depth)
{
  if (count > kMaxObjectsPerList || depth > kMaxDepth) {
    return false;
  }
  for (uint32 i = 0; i < count; ++i) {
    if (!readObject(stream, scene, parent, depth)) {
      return false;
    }
  }
  return true;
}

/*
 */
bool
GameObjectSerializer::readObject(DataStream& stream,
                                 Scene& scene,
                                 GameObject* parent,
                                 uint32 depth)
{
  GameObjectFileData data;
  String name;
  if (stream.read(&data, sizeof(data)) != sizeof(data) ||
      !AssetFile::readString(stream, name, kMaxNameLength) ||
      data.componentCount > kMaxComponents) {
    return false;
  }

  const SPtr<GameObject> gameObject = scene.createGameObject(name, parent);
  // The saved UUID, so whatever points at the object by its id finds it again.
  gameObject->m_uuid = data.uuid;
  Transform& transform = gameObject->getTransform();
  transform.setLocalPosition(Vector3(data.position[0], data.position[1], data.position[2]));
  transform.setLocalRotation(
      Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]));
  transform.setLocalScale(Vector3(data.scale[0], data.scale[1], data.scale[2]));

  for (uint32 i = 0; i < data.componentCount; ++i) {
    if (!readComponent(stream, *gameObject)) {
      return false;
    }
  }
  return readList(stream, scene, gameObject.get(), data.childCount, depth + 1);
}

/*
 */
bool
GameObjectSerializer::readComponent(DataStream& stream, GameObject& gameObject)
{
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

} // namespace chEngineSDK
