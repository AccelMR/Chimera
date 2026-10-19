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

#include "chDebugDraw.h"
#include "chGameObject.h"
#include "chObject.h"
#include "chLightItem.h"
#include "chSkyLightItem.h"
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
   * Walks the whole hierarchy: meant for rare lookups (undo), not for every frame. Null when
   * no object has the id.
   */
  NODISCARD SPtr<GameObject>
  findGameObject(const UUID& id) const;

  /**
   * Place of the object among its parent's children, or among the root objects.
   */
  NODISCARD uint32
  getSiblingIndex(const GameObject& gameObject) const;

  /**
   * Moves the object to index among its siblings; past the end puts it last.
   */
  void
  setSiblingIndex(GameObject& gameObject, uint32 index);

  /**
   * Moves the object, with its children, under newParent (a root object when null), last
   * among its new siblings, and keeps where it stands in the world. Reads the world matrices
   * of the last updateTransforms. False, with nothing changed, when newParent is the object,
   * one of its children or in another scene.
   */
  NODISCARD bool
  setParent(GameObject& gameObject, GameObject* newParent);

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
   * Every slot, free ones included (their mesh is null). For tools that look at all items;
   * the renderer uses gatherRenderItems.
   */
  NODISCARD FORCEINLINE Span<const RenderItem>
  getRenderItemSlots() const { return m_renderItems; }

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

  /**
   * Returns an id that stays valid until removeSkyLight.
   */
  NODISCARD uint32
  addSkyLight(const SkyLightItem& skyLight);

  NODISCARD FORCEINLINE SkyLightItem&
  getSkyLight(uint32 id) { return m_skyLights[id]; }

  void
  removeSkyLight(uint32 id);

  /**
   * The sky light that lights the scene: the last one registered, null when there is none.
   * A scene holds one or two, so the search is cheap enough for every frame.
   */
  NODISCARD const SkyLightItem*
  getActiveSkyLight() const;

  /**
   * Lines the scene's components draw. Cleared when update() starts, so lines drawn during
   * an update show on every frame until the next one.
   */
  NODISCARD FORCEINLINE DebugDraw&
  getDebugDraw() { return m_debugDraw; }

  NODISCARD FORCEINLINE const DebugDraw&
  getDebugDraw() const { return m_debugDraw; }

 private:
  Vector<SPtr<GameObject>> m_rootGameObjects;
  Vector<RenderItem> m_renderItems;
  Vector<uint32> m_freeRenderItems;
  uint32 m_renderItemCount = 0;
  Vector<LightItem> m_lights;
  Vector<uint32> m_freeLights;
  uint32 m_lightCount = 0;
  Vector<SkyLightItem> m_skyLights;
  Vector<uint32> m_freeSkyLights;
  uint32 m_nextSkyLightOrder = 0;
  DebugDraw m_debugDraw;
};

} // namespace chEngineSDK
