/************************************************************************/
/**
 * @file chScene.cpp
 * @author AccelMR
 * @date 2025/10/28
 * @brief
 *  Scene class containing a hierarchy of GameObjects.
 */
/************************************************************************/
#include "chScene.h"

#include "chAlgorithm.h"
#include "chFrustum.h"
#include "chShapeOverlap.h"

namespace chEngineSDK {

/*
 */
Scene::Scene(const String& name, UUID id)
 : Object(name, id)
{}

/*
 */
Scene::~Scene()
{
  // Components remove their render items while the scene still exists.
  for (const SPtr<GameObject>& root : m_rootGameObjects) {
    root->setRegisteredRecursive(false);
  }
}

/*
 */
SPtr<GameObject>
Scene::createGameObject(const String& name, GameObject* parent)
{
  SPtr<GameObject> gameObject = chMakeShared<GameObject>(name, this);
  if (parent) {
    CH_ASSERT(parent->getScene() == this);
    gameObject->m_parent = parent;
    parent->m_children.push_back(gameObject);
  }
  else {
    m_rootGameObjects.push_back(gameObject);
  }
  return gameObject;
}

/*
 */
void
Scene::destroyGameObject(GameObject& gameObject)
{
  if (gameObject.getScene() != this) {
    return;
  }

  gameObject.setRegisteredRecursive(false);

  Vector<SPtr<GameObject>>& siblings =
      gameObject.m_parent ? gameObject.m_parent->m_children : m_rootGameObjects;
  gameObject.m_parent = nullptr;
  // Last, because it may release the last reference to the object.
  Algorithm::removeFirstIf(siblings, [&gameObject](const SPtr<GameObject>& sibling) {
    return sibling.get() == &gameObject;
  });
}

/*
 */
void
Scene::update(float deltaTime)
{
  for (const SPtr<GameObject>& root : m_rootGameObjects) {
    root->update(deltaTime);
  }
}

/*
 */
void
Scene::updateTransforms()
{
  for (const SPtr<GameObject>& root : m_rootGameObjects) {
    root->updateWorldMatrices(Matrix4::IDENTITY, false);
  }
}

/*
 */
uint32
Scene::addRenderItem(const RenderItem& item)
{
  CH_ASSERT(item.mesh != nullptr);
  ++m_renderItemCount;

  if (!m_freeRenderItems.empty()) {
    const uint32 id = m_freeRenderItems.back();
    m_freeRenderItems.pop_back();
    m_renderItems[id] = item;
    return id;
  }

  m_renderItems.push_back(item);
  return static_cast<uint32>(m_renderItems.size() - 1);
}

/*
 */
void
Scene::removeRenderItem(uint32 id)
{
  CH_ASSERT(id < m_renderItems.size() && m_renderItems[id].mesh != nullptr);
  m_renderItems[id] = RenderItem{};
  m_freeRenderItems.push_back(id);
  --m_renderItemCount;
}

/*
 */
void
Scene::gatherRenderItems(const Frustum& frustum, Vector<const RenderItem*>& outItems) const
{
  outItems.clear();
  for (const RenderItem& item : m_renderItems) {
    if (item.mesh && ShapeOverlap::frustumBounds(frustum, item.worldBounds)) {
      outItems.push_back(&item);
    }
  }
}

} // namespace chEngineSDK
