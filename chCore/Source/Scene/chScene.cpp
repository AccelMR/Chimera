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
#include "chCameraComponent.h"
#include "chFrustum.h"
#include "chMath.h"
#include "chMesh.h"
#include "chRay.h"
#include "chRayCast.h"
#include "chRenderCulling.h"
#include "chSceneManager.h"

namespace chEngineSDK {

namespace {
NODISCARD FORCEINLINE Vector3
toVector3(const Vector4& vector)
{
  return Vector3(vector.x, vector.y, vector.z);
}

/*
 * True when gameObject is ancestor or anywhere below it.
 */
NODISCARD bool
isUnder(const GameObject* gameObject, const GameObject* ancestor)
{
  for (const GameObject* current = gameObject; current; current = current->getParent()) {
    if (current == ancestor) {
      return true;
    }
  }
  return false;
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

/*
 * Drops the null slots and tells each component its new place, through slot (a member of
 * Component the scene may name).
 */
void
closeGaps(Vector<Component*>& components, uint32 Component::*slot)
{
  SIZE_T kept = 0;
  for (Component* component : components) {
    if (component) {
      component->*slot = static_cast<uint32>(kept);
      components[kept++] = component;
    }
  }
  components.resize(kept);
}
} // namespace

/*
 */
Scene::Scene(const String& name, UUID id)
 : Object(name, id)
{
  m_levels.push_back({.name = name, .asset = UUID::null(), .id = MAP_LEVEL});
}

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
  // A new transform starts dirty, so it never tells the scene by itself.
  addDirtyTransform(*gameObject);
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
  // Read before the object leaves its root.
  const SceneLevelId level = gameObject.getLevel();

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
  if (!newParent) {
    gameObject.m_level = level;
  }
  Vector<SPtr<GameObject>>& newSiblings = newParent ? newParent->m_children : m_rootGameObjects;
  newSiblings.push_back(std::move(moved));
  return true;
}

/*
 */
void
Scene::fixedUpdate(float deltaTime)
{
  m_fixedDebugDraw.clear();
  // By index with the count taken first: components registered meanwhile (objects a script
  // made) start from the next step, and those that leave only null their slot.
  const SIZE_T count = m_fixedUpdates.size();
  for (SIZE_T i = 0; i < count; ++i) {
    Component* component = m_fixedUpdates[i];
    if (component && (m_bPlaying || component->updatesWhileEditing())) {
      component->fixedUpdate(deltaTime);
    }
  }
  applyDestroyRequests();
  compactUpdateLists();
}

/*
 */
void
Scene::update(float deltaTime)
{
  m_debugDraw.clear();
  const SIZE_T count = m_frameUpdates.size();
  for (SIZE_T i = 0; i < count; ++i) {
    Component* component = m_frameUpdates[i];
    if (component && (m_bPlaying || component->updatesWhileEditing())) {
      component->update(deltaTime);
    }
  }
  applyDestroyRequests();
  compactUpdateLists();
}

/*
 */
void
Scene::requestDestroy(GameObject& gameObject)
{
  if (gameObject.getScene() != this || gameObject.m_bDestroyRequested) {
    return;
  }
  // Always queued, also outside an update: onDestroy of a script may ask while the scene is
  // unregistering a subtree, and the tree must not change under it.
  gameObject.m_bDestroyRequested = true;
  m_destroyRequests.push_back(gameObject.shared_from_this());
}

/*
 */
void
Scene::applyDestroyRequests()
{
  if (m_destroyRequests.empty()) {
    return;
  }
  // onDestroy of a script may ask for more while a subtree is being unregistered, so those
  // are queued too and destroyed in the next round. A chain that never ends waits for the
  // next update.
  constexpr uint32 kMaxRounds = 8;
  for (uint32 round = 0; round < kMaxRounds && !m_destroyRequests.empty(); ++round) {
    m_destroyBatch.swap(m_destroyRequests);
    bool bRootLeft = false;
    for (const SPtr<GameObject>& gameObject : m_destroyBatch) {
      // An object whose parent went first already left the scene.
      if (gameObject->m_scene != this) {
        continue;
      }
      gameObject->setRegisteredRecursive(false);
      if (GameObject* parent = gameObject->m_parent) {
        parent->m_bChildLeft = true;
      }
      else {
        bRootLeft = true;
      }
    }

    // One pass per list of siblings, instead of a search and an erase per object, which cost
    // as much as the list is long.
    for (const SPtr<GameObject>& gameObject : m_destroyBatch) {
      GameObject* parent = gameObject->m_parent;
      if (parent && parent->m_bChildLeft) {
        parent->m_bChildLeft = false;
        removeDestroyedObjects(parent->m_children);
      }
    }
    if (bRootLeft) {
      removeDestroyedObjects(m_rootGameObjects);
    }
    m_destroyBatch.clear();
  }
}

/*
 */
void
Scene::removeDestroyedObjects(Vector<SPtr<GameObject>>& siblings)
{
  const auto hasLeft = [](const SPtr<GameObject>& sibling) {
    return sibling->m_bDestroyRequested && sibling->m_scene == nullptr;
  };
  for (const SPtr<GameObject>& sibling : siblings) {
    if (hasLeft(sibling)) {
      sibling->m_parent = nullptr;
    }
  }
  // Last, because it may release the last reference to the objects.
  Algorithm::removeAllIf(siblings, hasLeft);
}

/*
 */
void
Scene::addUpdatingComponent(Component& component)
{
  const ComponentUpdateFlags flags = component.m_updateFlags;
  if (flags.isSet(ComponentUpdate::Frame)) {
    component.m_frameUpdateSlot = static_cast<uint32>(m_frameUpdates.size());
    m_frameUpdates.push_back(&component);
  }
  if (flags.isSet(ComponentUpdate::Fixed)) {
    component.m_fixedUpdateSlot = static_cast<uint32>(m_fixedUpdates.size());
    m_fixedUpdates.push_back(&component);
  }
}

/*
 */
void
Scene::removeUpdatingComponent(Component& component)
{
  if (component.m_frameUpdateSlot != Component::NO_SLOT) {
    m_frameUpdates[component.m_frameUpdateSlot] = nullptr;
    component.m_frameUpdateSlot = Component::NO_SLOT;
    m_bUpdateListsHaveGaps = true;
  }
  if (component.m_fixedUpdateSlot != Component::NO_SLOT) {
    m_fixedUpdates[component.m_fixedUpdateSlot] = nullptr;
    component.m_fixedUpdateSlot = Component::NO_SLOT;
    m_bUpdateListsHaveGaps = true;
  }
}

/*
 */
void
Scene::compactUpdateLists()
{
  if (!m_bUpdateListsHaveGaps) {
    return;
  }
  closeGaps(m_frameUpdates, &Component::m_frameUpdateSlot);
  closeGaps(m_fixedUpdates, &Component::m_fixedUpdateSlot);
  m_bUpdateListsHaveGaps = false;
}

/*
 */
void
Scene::addDirtyTransform(GameObject& gameObject)
{
  gameObject.m_dirtyTransformSlot = static_cast<uint32>(m_dirtyTransforms.size());
  m_dirtyTransforms.push_back(&gameObject);
}

/*
 */
void
Scene::removeDirtyTransform(GameObject& gameObject)
{
  m_dirtyTransforms[gameObject.m_dirtyTransformSlot] = nullptr;
  gameObject.m_dirtyTransformSlot = GameObject::NO_SLOT;
}

/*
 */
void
Scene::updateTransforms()
{
  // The size is read every time: a component told about a new matrix may move another
  // object, which is then rebuilt in this same pass.
  for (SIZE_T i = 0; i < m_dirtyTransforms.size(); ++i) {
    GameObject* gameObject = m_dirtyTransforms[i];
    if (!gameObject) {
      continue;
    }
    // Listed again after an earlier rebuild in this pass; the later slot is its place.
    if (gameObject->m_dirtyTransformSlot == i) {
      gameObject->m_dirtyTransformSlot = GameObject::NO_SLOT;
    }
    // Already rebuilt with a parent that had changed too.
    if (!gameObject->m_transform.isDirty()) {
      continue;
    }

    // The highest changed object above rebuilds everything under it, this one included.
    GameObject* top = gameObject;
    for (GameObject* parent = gameObject->m_parent; parent; parent = parent->m_parent) {
      if (parent->m_transform.isDirty()) {
        top = parent;
      }
    }
    top->updateWorldMatrices(top->m_parent ? top->m_parent->m_transform.getWorldMatrix()
                                           : Matrix4::IDENTITY);
  }
  m_dirtyTransforms.clear();
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
  RenderCulling::gatherItems(m_renderItems, frustum, outItems);
}

/*
 */
bool
Scene::raycast(const Ray& ray,
               SceneRayHit& outHit,
               const float maxDistance,
               const GameObject* ignore) const
{
  bool bHit = false;
  for (const RenderItem& item : m_renderItems) {
    if (!item.mesh) {
      continue;
    }

    const AABox worldBox(item.worldBounds.center - item.worldBounds.boxExtent,
                         item.worldBounds.center + item.worldBounds.boxExtent);
    float boxDistance = 0.0f;
    if (!RayCast::box(ray, worldBox, boxDistance) || boxDistance > maxDistance ||
        (bHit && boxDistance > outHit.distance)) {
      continue;
    }
    // Walked only for items the ray reaches, so it costs nothing for the rest.
    if (ignore && isUnder(item.owner, ignore)) {
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

    // A skinned item's matrix only places the model; its triangles are where the pose put
    // them.
    float localDistance = 0.0f;
    Vector3 localNormal;
    const bool bLocalHit =
        item.skinMatrices
            ? item.mesh->raycastSkinned(
                  localRay,
                  Span<const Matrix4>(item.skinMatrices, item.mesh->getSkin().boneNodes.size()),
                  localDistance,
                  &localNormal)
            : item.mesh->raycast(localRay, localDistance, &localNormal);
    if (!bLocalHit) {
      continue;
    }
    const Vector3 worldPoint =
        toVector3(item.worldMatrix.transformPosition(localRay.getPoint(localDistance)));
    const float distance = worldPoint.distance(ray.origin);
    if (distance > maxDistance || (bHit && distance >= outHit.distance)) {
      continue;
    }

    // Normals go through the inverse transpose, so non-uniform scale keeps them square to
    // the surface.
    Vector3 worldNormal = toVector3(worldToLocal.getTransposed().transformVector(localNormal));
    if (!worldNormal.normalize()) {
      worldNormal = -ray.direction;
    }
    else if (worldNormal.dot(ray.direction) > 0.0f) {
      worldNormal = -worldNormal;
    }
    outHit = {.gameObject = item.owner,
              .item = &item,
              .distance = distance,
              .point = worldPoint,
              .normal = worldNormal};
    bHit = true;
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
  RenderCulling::gatherLights(m_lights, frustum, outLights);
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

  // Within a level the last one wins on purpose (a script swapping skies); across levels it
  // is usually two files that each brought their own.
  const SceneLevelId level = skyLight.owner ? skyLight.owner->getLevel() : MAP_LEVEL;
  for (const SkyLightItem& other : m_skyLights) {
    if (other.bInUse && &other != &m_skyLights[id] && other.owner &&
        other.owner->getLevel() != level) {
      CH_LOG_WARNING(SceneManagerLog,
                     "Scene {0} has sky lights in several levels; the one of the first level "
                     "is used.",
                     m_name);
      break;
    }
  }
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
  uint32 activeRank = MAX_LEVEL_RANK;
  for (const SkyLightItem& skyLight : m_skyLights) {
    if (!skyLight.bInUse) {
      continue;
    }
    const uint32 rank =
        skyLight.owner ? getLevelRank(skyLight.owner->getLevel()) : MAP_LEVEL;
    if (!active || rank < activeRank ||
        (rank == activeRank && skyLight.order > active->order)) {
      active = &skyLight;
      activeRank = rank;
    }
  }
  return active;
}

/*
 */
void
Scene::addCamera(CameraComponent& camera)
{
  m_cameras.push_back(&camera);
}

/*
 */
void
Scene::removeCamera(CameraComponent& camera)
{
  Algorithm::removeFirst(m_cameras, &camera);
  if (m_chosenCamera == &camera) {
    m_chosenCamera = nullptr;
  }
}

/*
 */
const CameraComponent*
Scene::getActiveCamera() const
{
  if (m_chosenCamera) {
    return m_chosenCamera;
  }
  const CameraComponent* active = nullptr;
  uint32 activeRank = MAX_LEVEL_RANK;
  for (const CameraComponent* camera : m_cameras) {
    const uint32 rank = getLevelRank(camera->getOwner()->getLevel());
    if (!active || camera->getPriority() > active->getPriority() ||
        (camera->getPriority() == active->getPriority() && rank <= activeRank)) {
      active = camera;
      activeRank = rank;
    }
  }
  return active;
}

/*
 */
void
Scene::setMapLevel(const String& name, const UUID& asset)
{
  m_levels[0].name = name;
  m_levels[0].asset = asset;
}

/*
 */
const SceneLevel*
Scene::findLevel(const UUID& asset) const
{
  for (const SceneLevel& level : m_levels) {
    if (level.asset == asset && !asset.isNull()) {
      return &level;
    }
  }
  return nullptr;
}

/*
 */
const SceneLevel*
Scene::findLevelById(const SceneLevelId id) const
{
  for (const SceneLevel& level : m_levels) {
    if (level.id == id) {
      return &level;
    }
  }
  return nullptr;
}

/*
 */
uint32
Scene::getLevelRank(const SceneLevelId id) const
{
  // The map is always first, and most scenes have no other level.
  if (id == MAP_LEVEL) {
    return 0;
  }
  for (SIZE_T i = 1; i < m_levels.size(); ++i) {
    if (m_levels[i].id == id) {
      return static_cast<uint32>(i);
    }
  }
  return MAX_LEVEL_RANK;
}

/*
 */
SceneLevelId
Scene::addLevel(const String& name, const UUID& asset)
{
  // The counter wraps after enough loads (a game streaming levels for hours), so ids still
  // in use and the map's are skipped; a level keeps its id until it is unloaded.
  CH_ASSERT(m_levels.size() < MAX_LEVEL_COUNT);
  SceneLevelId id = m_nextLevelId;
  while (id == MAP_LEVEL || getLevelRank(id) != MAX_LEVEL_RANK) {
    ++id;
  }
  m_nextLevelId = static_cast<SceneLevelId>(id + 1);
  m_levels.push_back({.name = name, .asset = asset, .id = id});
  return id;
}

/*
 */
void
Scene::setSceneRecursive(GameObject& gameObject)
{
  gameObject.m_scene = this;
  for (const SPtr<GameObject>& child : gameObject.m_children) {
    setSceneRecursive(*child);
  }
}

/*
 */
void
Scene::moveRootObjectsFrom(Scene& source, const SceneLevelId level)
{
  CH_ASSERT(&source != this);
  Vector<SPtr<GameObject>> roots = std::move(source.m_rootGameObjects);
  source.m_rootGameObjects.clear();

  m_rootGameObjects.reserve(m_rootGameObjects.size() + roots.size());
  for (const SPtr<GameObject>& root : roots) {
    if (root->m_scene) {
      root->setRegisteredRecursive(false);
    }
    setSceneRecursive(*root);
    root->m_level = level;
    m_rootGameObjects.push_back(root);

    // The world matrices were built in the other scene, or never; built again here.
    Transform& transform = root->m_transform;
    if (transform.isDirty()) {
      addDirtyTransform(*root);
    }
    else {
      transform.setLocalPosition(transform.getLocalPosition());
    }
    root->setRegisteredRecursive(true);
  }
}

/*
 */
bool
Scene::removeLevel(const SceneLevelId id)
{
  if (id == MAP_LEVEL) {
    return false;
  }
  const uint32 rank = getLevelRank(id);
  if (rank == MAX_LEVEL_RANK) {
    return false;
  }

  for (const SPtr<GameObject>& root : m_rootGameObjects) {
    if (root->m_level == id) {
      requestDestroy(*root);
    }
  }
  applyDestroyRequests();
  compactUpdateLists();
  m_levels.erase(m_levels.begin() + rank);
  return true;
}

/*
 */
bool
Scene::moveToLevel(GameObject& gameObject, const SceneLevelId level)
{
  if (gameObject.m_scene != this || getLevelRank(level) == MAX_LEVEL_RANK) {
    return false;
  }
  if (gameObject.m_parent) {
    if (gameObject.m_parent->getLevel() == level) {
      return true;
    }
    if (!setParent(gameObject, nullptr)) {
      return false;
    }
  }
  gameObject.m_level = level;
  return true;
}

/*
 */
bool
Scene::setActiveCamera(CameraComponent* camera)
{
  if (camera && !Algorithm::contains(m_cameras, camera)) {
    return false;
  }
  m_chosenCamera = camera;
  return true;
}

} // namespace chEngineSDK
