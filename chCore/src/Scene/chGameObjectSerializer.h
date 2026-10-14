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

/**
 * Exists so every file that holds GameObjects (scenes now, prefabs later) writes them the
 * same way: each object with its UUID, name, transform, components and children. Each
 * component is stored with its type and size, so a type nobody registered (a plugin that
 * is not loaded) is skipped with a warning and the rest of the file still loads.
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
   * Makes the objects in scene, under parent (null for root objects). False for a broken
   * file; the objects read before the error stay in the scene.
   */
  NODISCARD static bool
  read(DataStream& stream, Scene& scene, GameObject* parent);

  /**
   * Adds the UUIDs of the assets the objects and their children use, each once.
   */
  static void
  collectReferences(Span<const SPtr<GameObject>> gameObjects, Vector<UUID>& outReferences);

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 private:
  NODISCARD static bool
  readList(DataStream& stream, Scene& scene, GameObject* parent, uint32 count, uint32 depth);

  NODISCARD static bool
  readObject(DataStream& stream, Scene& scene, GameObject* parent, uint32 depth);

  NODISCARD static bool
  readComponent(DataStream& stream, GameObject& gameObject);
};

} // namespace chEngineSDK
