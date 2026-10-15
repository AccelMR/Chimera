/************************************************************************/
/**
 * @file chGameObjectSerializer.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Saves and loads GameObject hierarchies with their components.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {
class PrefabAsset;

/**
 * Exists so every file that holds GameObjects (scenes and prefabs) writes them the same way:
 * each object with its UUID, name, transform, components and children. Each component is
 * stored with its type and size, so a type nobody registered (a plugin that is not loaded)
 * is skipped with a warning and the rest of the file still loads. A prefab instance is
 * stored as its link alone and gets its content from the prefab when it is read.
 */
class CH_CORE_EXPORT GameObjectSerializer
{
 public:
  /**
   * The stream must support seek and tell, because each component's size is written after
   * its data.
   */
  static void
  write(DataStream& stream, Span<const SPtr<GameObject>> gameObjects);

  /**
   * A list of one object, as a prefab stores it.
   */
  static void
  write(DataStream& stream, const GameObject& gameObject);

  /**
   * Makes the objects in scene, under parent (null for root objects), with their saved
   * UUIDs. False for a broken file; the objects read before the error stay in the scene.
   */
  NODISCARD static bool
  read(DataStream& stream, Scene& scene, GameObject* parent);

  /**
   * Reads one object written by write() into target (which must be in a scene), replacing its
   * name, transform, prefab link, components and children; target keeps its UUID and place.
   * The children get their saved UUIDs. False for broken data.
   */
  NODISCARD static bool
  readInto(DataStream& stream, GameObject& target);

  /**
   * Makes an instance of prefab under parent: an object named after the prefab, linked to it,
   * with the prefab's content and the rotation and scale of the prefab's root. Its position
   * stays at zero; the caller places it.
   */
  static SPtr<GameObject>
  instantiate(const PrefabAsset& prefab, Scene& scene, GameObject* parent);

  /**
   * Gives target (which must be in a scene) the components and children of the prefab's
   * root, with new UUIDs, and the root's rotation and scale; its name, position and link stay
   * as they are. False when the content cannot be read.
   */
  NODISCARD static bool
  readPrefabContent(const PrefabAsset& prefab, GameObject& target);

  /**
   * Adds the UUIDs of the assets the objects and their children use, each once. For an
   * instance that is its prefab, which brings the rest.
   */
  static void
  collectReferences(Span<const SPtr<GameObject>> gameObjects, Vector<UUID>& outReferences);

  static void
  collectReferences(const GameObject& gameObject, Vector<UUID>& outReferences);

  // Version 2 added the prefab link of every object.
  static constexpr uint32 SERIALIZATION_VERSION = 2;

 private:
  struct ReadContext;
  struct ObjectHeader;

  NODISCARD static bool
  readList(ReadContext& context, GameObject* parent, uint32 count, uint32 depth);

  NODISCARD static bool
  readObject(ReadContext& context, GameObject* parent, uint32 depth);

  NODISCARD static bool
  readObjectHeader(ReadContext& context, ObjectHeader& outHeader);

  /**
   * Gives gameObject the transform and prefab link of header, then reads its components and
   * children from the stream (or its prefab's content).
   */
  NODISCARD static bool
  fillObject(ReadContext& context,
             GameObject& gameObject,
             const ObjectHeader& header,
             uint32 depth);

  NODISCARD static bool
  readComponent(ReadContext& context, GameObject& gameObject);

  NODISCARD static bool
  readPrefabInto(const PrefabAsset& prefab,
                 GameObject& target,
                 uint32 depth,
                 bool bApplyRootTransform);
};

} // namespace chEngineSDK
