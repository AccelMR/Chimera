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
#include "chMath.h"
#include "chMesh.h"
#include "chRay.h"
#include "chRayCast.h"
#include "chShapeOverlap.h"

namespace chEngineSDK {

namespace {
NODISCARD FORCEINLINE Vector3
toVector3(const Vector4& vector)
{
  return Vector3(vector.x, vector.y, vector.z);
}

/*
 */
SPtr<GameObject>
findInList(const Vector<SPtr<GameObject>>& gameObjects, const UUID& id)
{
  for (const SPtr<GameObject>& gameObject : gameObjects) {
    if (gameObject->getUUID() == id) {
      return gameObject;
    }
    if (SPtr<GameObject> found = findInList(gameObject->getChildren(), id)) {
      return found;
    }
  }
  return nullptr;
}
} // namespace

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
SPtr<GameObject>
Scene::findGameObject(const UUID& id) const
{
  return findInList(m_rootGameObjects, id);
}

/*
 */
uint32
Scene::getSiblingIndex(const GameObject& gameObject) const
{
  const Vector<SPtr<GameObject>>& siblings =
      gameObject.m_parent ? gameObject.m_parent->m_children : m_rootGameObjects;
  for (SIZE_T i = 0; i < siblings.size(); ++i) {
    if (siblings[i].get() == &gameObject) {
      return static_cast<uint32>(i);
    }
  }
  return 0;
}

/*
 */
void
Scene::setSiblingIndex(GameObject& gameObject, uint32 index)
{
  Vector<SPtr<GameObject>>& siblings =
      gameObject.m_parent ? gameObject.m_parent->m_children : m_rootGameObjects;
  const SIZE_T current = getSiblingIndex(gameObject);
  const SIZE_T target = Math::min(static_cast<SIZE_T>(index), siblings.size() - 1);
  if (current == target) {
    return;
  }
  SPtr<GameObject> moved = std::move(siblings[current]);
  siblings.erase(siblings.begin() + current);
  siblings.insert(siblings.begin() + target, std::move(moved));
}

/*
 */
bool
Scene::setParent(GameObject& gameObject, GameObject* newParent)
{
  if (gameObject.m_scene != this || (newParent && newParent->m_scene != this)) {
    return false;
  }
  for (const GameObject* ancestor = newParent; ancestor; ancestor = ancestor->m_parent) {
    if (ancestor == &gameObject) {
      return false;
    }
  }
  if (gameObject.m_parent == newParent) {
    return true;
  }

  Transform& transform = gameObject.m_transform;
  const Matrix4& world = transform.getWorldMatrix();
  const Matrix4 local =
      newParent ? world * newParent->m_transform.getWorldMatrix().getInverseAffine() : world;
  Vector3 scale;
  Quaternion rotation;
  Vector3 position;
  if (local.decompose(scale, rotation, position)) {
    transform.setLocalScale(scale);
    transform.setLocalRotation(rotation);
    transform.setLocalPosition(position);
  }
  else {
    // A zero scale has no rotation to read; the local values stay, and the world matrix is
    // still built again under the new parent.
    transform.setLocalPosition(transform.getLocalPosition());
  }

  Vector<SPtr<GameObject>>& oldSiblings =
      gameObject.m_parent ? gameObject.m_parent->m_children : m_rootGameObjects;
  const SIZE_T index = getSiblingIndex(gameObject);
  // Kept alive here, since the old list may hold its last reference.
  SPtr<GameObject> moved = std::move(oldSiblings[index]);
  oldSiblings.erase(oldSiblings.begin() + index);

  gameObject.m_parent = newParent;
  Vector<SPtr<GameObject>>& newSiblings = newParent ? newParent->m_children : m_rootGameObjects;
  newSiblings.push_back(std::move(moved));
  return true;
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

/*
 */
bool
Scene::raycast(const Ray& ray, SceneRayHit& outHit) const
{
  bool bHit = false;
  for (const RenderItem& item : m_renderItems) {
    if (!item.mesh) {
      continue;
    }

    const AABox worldBox(item.worldBounds.center - item.worldBounds.boxExtent,
                         item.worldBounds.center + item.worldBounds.boxExtent);
    float boxDistance = 0.0f;
    if (!RayCast::box(ray, worldBox, boxDistance) || (bHit && boxDistance > outHit.distance)) {
      continue;
    }

    // The triangles stay in the mesh's own space; the ray goes there instead, and its
    // direction is made unit again because the matrix may scale.
    const Matrix4 worldToLocal = item.worldMatrix.getInverseAffine();
    Vector3 localDirection = toVector3(worldToLocal.transformVector(ray.direction));
    if (!localDirection.normalize()) {
      continue;
    }
    const Ray localRay(toVector3(worldToLocal.transformPosition(ray.origin)),
                       localDirection);

    float localDistance = 0.0f;
    if (!item.mesh->raycast(localRay, localDistance)) {
      continue;
    }
    const Vector3 worldPoint =
        toVector3(item.worldMatrix.transformPosition(localRay.getPoint(localDistance)));
    const float distance = worldPoint.distance(ray.origin);
    if (!bHit || distance < outHit.distance) {
      outHit = {.gameObject = item.owner, .item = &item, .distance = distance};
      bHit = true;
    }
  }
  return bHit;
}

/*
 */
uint32
Scene::addLight(const LightItem& light)
{
  ++m_lightCount;

  uint32 id = 0;
  if (!m_freeLights.empty()) {
    id = m_freeLights.back();
    m_freeLights.pop_back();
    m_lights[id] = light;
  }
  else {
    id = static_cast<uint32>(m_lights.size());
    m_lights.push_back(light);
  }
  m_lights[id].bInUse = true;
  return id;
}

/*
 */
void
Scene::removeLight(uint32 id)
{
  CH_ASSERT(id < m_lights.size() && m_lights[id].bInUse);
  m_lights[id] = LightItem{};
  m_freeLights.push_back(id);
  --m_lightCount;
}

/*
 */
void
Scene::gatherLights(const Frustum& frustum, Vector<const LightItem*>& outLights) const
{
  outLights.clear();
  for (const LightItem& light : m_lights) {
    if (!light.bInUse) {
      continue;
    }
    // A spot is tested with the sphere of its range too: a cone test would save little for
    // what it costs.
    if (light.type == LightType::Directional ||
        ShapeOverlap::frustumSphere(frustum, Sphere(light.position, light.range))) {
      outLights.push_back(&light);
    }
  }
}

/*
 */
uint32
Scene::addSkyLight(const SkyLightItem& skyLight)
{
  uint32 id = 0;
  if (!m_freeSkyLights.empty()) {
    id = m_freeSkyLights.back();
    m_freeSkyLights.pop_back();
    m_skyLights[id] = skyLight;
  }
  else {
    id = static_cast<uint32>(m_skyLights.size());
    m_skyLights.push_back(skyLight);
  }
  m_skyLights[id].bInUse = true;
  m_skyLights[id].order = m_nextSkyLightOrder++;
  return id;
}

/*
 */
void
Scene::removeSkyLight(uint32 id)
{
  CH_ASSERT(id < m_skyLights.size() && m_skyLights[id].bInUse);
  m_skyLights[id] = SkyLightItem{};
  m_freeSkyLights.push_back(id);
}

/*
 */
const SkyLightItem*
Scene::getActiveSkyLight() const
{
  const SkyLightItem* active = nullptr;
  for (const SkyLightItem& skyLight : m_skyLights) {
    if (skyLight.bInUse && (active == nullptr || skyLight.order > active->order)) {
      active = &skyLight;
    }
  }
  return active;
}

} // namespace chEngineSDK
