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
#include "chRenderItem.h"

namespace chEngineSDK {
class Frustum;

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

 private:
  Vector<SPtr<GameObject>> m_rootGameObjects;
  Vector<RenderItem> m_renderItems;
  Vector<uint32> m_freeRenderItems;
  uint32 m_renderItemCount = 0;
};

} // namespace chEngineSDK
