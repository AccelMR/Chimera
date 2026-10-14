/************************************************************************/
/**
 * @file chScene.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  Scene class containing a hierarchy of GameObjects.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGameObject.h"
#include "chObject.h"
#include "chLightItem.h"
#include "chRenderItem.h"

namespace chEngineSDK {
class Frustum;

/**
 * What a ray hit in a scene: the nearest render item along it and the object it draws for.
 */
struct SceneRayHit
{
  GameObject* gameObject = nullptr;
  const RenderItem* item = nullptr;
  // Along the ray, in world units.
  float distance = 0.0f;
};

/**
 * Exists to own a hierarchy of GameObjects and the flat list of what they draw. Components
 * add and update render items here, so the renderer only asks for the items a camera sees
 * and never walks the hierarchy.
 */
class CH_CORE_EXPORT Scene : public Object
{
 public:
  Scene(const String& name, UUID id);

  ~Scene() override;

  Scene(const Scene&) = delete;

  Scene&
  operator=(const Scene&) = delete;

  /**
   * parent null makes a root object. The object stays alive while it is in the scene.
   */
  SPtr<GameObject>
  createGameObject(const String& name, GameObject* parent = nullptr);

  /**
   * Removes the object and its children from the scene and unregisters their components.
   * Objects still held elsewhere (an SPtr) stay alive, out of any scene.
   */
  void
  destroyGameObject(GameObject& gameObject);

  NODISCARD FORCEINLINE const Vector<SPtr<GameObject>>&
  getRootGameObjects() const { return m_rootGameObjects; }

  /**
   * Updates the components of every object.
   */
  void
  update(float deltaTime);

  /**
   * Rebuilds the world matrix of every object whose transform or parent changed and tells
   * their components. Call it once per frame before rendering.
   */
  void
  updateTransforms();

  /**
   * Returns an id that stays valid until removeRenderItem. item.mesh must not be null.
   */
  NODISCARD uint32
  addRenderItem(const RenderItem& item);

  NODISCARD FORCEINLINE RenderItem&
  getRenderItem(uint32 id) { return m_renderItems[id]; }

  void
  removeRenderItem(uint32 id);

  NODISCARD FORCEINLINE uint32
  getRenderItemCount() const { return m_renderItemCount; }

  /**
   * Replaces the contents of outItems with the items whose bounds touch the frustum. The
   * pointers are valid until the next add or remove. Reusing outItems every frame keeps it
   * from allocating.
   */
  void
  gatherRenderItems(const Frustum& frustum, Vector<const RenderItem*>& outItems) const;

  /**
   * Nearest render item the ray hits, by its triangles. Tests every item's bounds first,
   * then the triangles of those the ray reaches, all on the CPU: meant for picking, not
   * for every frame. ray.direction must be unit length.
   */
  NODISCARD bool
  raycast(const Ray& ray, SceneRayHit& outHit) const;

  /**
   * Returns an id that stays valid until removeLight.
   */
  NODISCARD uint32
  addLight(const LightItem& light);

  NODISCARD FORCEINLINE LightItem&
  getLight(uint32 id) { return m_lights[id]; }

  void
  removeLight(uint32 id);

  NODISCARD FORCEINLINE uint32
  getLightCount() const { return m_lightCount; }

  /**
   * Replaces the contents of outLights with every directional light and the point and spot
   * lights whose range touches the frustum. Same lifetime and reuse rules as
   * gatherRenderItems.
   */
  void
  gatherLights(const Frustum& frustum, Vector<const LightItem*>& outLights) const;

 private:
  Vector<SPtr<GameObject>> m_rootGameObjects;
  Vector<RenderItem> m_renderItems;
  Vector<uint32> m_freeRenderItems;
  uint32 m_renderItemCount = 0;
  Vector<LightItem> m_lights;
  Vector<uint32> m_freeLights;
  uint32 m_lightCount = 0;
};

} // namespace chEngineSDK
