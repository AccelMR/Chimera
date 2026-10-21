/************************************************************************/
/**
 * @file chShapeComponent.cpp
 * @author AccelMR
 * @date 2026/10/18
 * @brief
 *  Component that draws a simple shape (plane, box, sphere...) at its GameObject.
 */
/************************************************************************/
#include "chShapeComponent.h"

#include "chAssetManager.h"
#include "chBox.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chMaterialAsset.h"
#include "chMesh.h"
#include "chScene.h"
#include "chSceneManager.h"

namespace chEngineSDK {
namespace {
struct ShapeFileData
{
  UUID material;
  uint32 type;
  float size[3];
  float radius;
  float height;
  uint32 segments;
  uint32 rings;
  uint32 subdivisions;
};
static_assert(sizeof(ShapeFileData) == 52, "ShapeFileData must have no padding");

struct CachedMesh
{
  ShapeDesc shape;
  WeakPtr<Mesh> mesh;
};

/*
 * Meshes of the shapes in use, by the hash of their canonical description. Held weakly, so
 * a mesh goes away with the last component that draws it. Main thread only, like every
 * component change.
 */
UnorderedMap<uint64, CachedMesh>&
getMeshCache()
{
  static UnorderedMap<uint64, CachedMesh> cache;
  return cache;
}

NODISCARD SPtr<Mesh>
findOrCreateMesh(const ShapeDesc& shape)
{
  const ShapeDesc canonical = MeshGenerator::getCanonical(shape);
  const uint64 key = canonical.getHash();
  UnorderedMap<uint64, CachedMesh>& cache = getMeshCache();
  const auto found = cache.find(key);
  if (found != cache.end() && found->second.shape == canonical) {
    if (SPtr<Mesh> mesh = found->second.mesh.lock()) {
      return mesh;
    }
  }

  // Entries of shapes nobody draws any more are dropped here, which only runs when a new
  // shape is made, so the cache stays as small as the shapes in use.
  for (auto it = cache.begin(); it != cache.end();) {
    it = it->second.mesh.expired() ? cache.erase(it) : std::next(it);
  }

  SPtr<Mesh> mesh = chMakeShared<Mesh>();
  MeshGenerator::generate(canonical, *mesh);
  cache[key] = CachedMesh{.shape = canonical, .mesh = mesh};
  return mesh;
}

/*
 * Null when there is no such asset or it is of another type. Without an AssetManager
 * (tests) nothing is found.
 */
NODISCARD SPtr<MaterialAsset>
findMaterial(const UUID& id)
{
  if (id.isNull() || !AssetManager::isStarted()) {
    return nullptr;
  }
  const SPtr<IAsset> asset = AssetManager::instance().getAsset(id);
  return asset ? asset->as<MaterialAsset>() : nullptr;
}
} // namespace

/*
 */
ShapeComponent::ShapeComponent(const ShapeDesc& shape)
  : m_shape(MeshGenerator::getClamped(shape))
{}

/*
 */
void
ShapeComponent::setShape(const ShapeDesc& shape)
{
  m_shape = MeshGenerator::getClamped(shape);
  m_mesh = findOrCreateMesh(m_shape);
  updateRenderItem();
}

/*
 */
const SPtr<Mesh>&
ShapeComponent::getMesh()
{
  if (!m_mesh) {
    m_mesh = findOrCreateMesh(m_shape);
  }
  return m_mesh;
}

/*
 */
void
ShapeComponent::setMaterial(const SPtr<MaterialAsset>& material)
{
  m_material = material;
  m_materialId = material ? material->getUUID() : UUID::null();
  updateRenderItem();
}

/*
 */
bool
ShapeComponent::getWorldBounds(AABox& outBounds) const
{
  if (!isRegistered()) {
    return false;
  }
  outBounds = getOwner()->getScene()->getRenderItem(m_renderItemId).worldBounds.getBox();
  return true;
}

/*
 */
void
ShapeComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  const ShapeFileData data{.material = m_materialId,
                           .type = static_cast<uint32>(m_shape.type),
                           .size = {m_shape.size.x, m_shape.size.y, m_shape.size.z},
                           .radius = m_shape.radius,
                           .height = m_shape.height,
                           .segments = m_shape.segments,
                           .rings = m_shape.rings,
                           .subdivisions = m_shape.subdivisions};
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
}

/*
 */
bool
ShapeComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  ShapeFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data) ||
      data.type >= static_cast<uint32>(ShapeType::COUNT)) {
    return false;
  }

  setShape({.type = static_cast<ShapeType>(data.type),
            .size = Vector3(data.size[0], data.size[1], data.size[2]),
            .radius = data.radius,
            .height = data.height,
            .segments = data.segments,
            .rings = data.rings,
            .subdivisions = data.subdivisions});

  const SPtr<MaterialAsset> material = findMaterial(data.material);
  setMaterial(material);
  m_materialId = data.material;
  if (!material && !data.material.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "A shape lost its material {0}.", data.material);
  }
  return true;
}

/*
 */
void
ShapeComponent::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_materialId.isNull()) {
    outReferences.push_back(m_materialId);
  }
}

/*
 */
void
ShapeComponent::onRegister()
{
  m_renderItemId = getOwner()->getScene()->addRenderItem(
      RenderItem{.mesh = getMesh().get(), .owner = getOwner()});
  updateRenderItem();
}

/*
 */
void
ShapeComponent::onUnregister()
{
  getOwner()->getScene()->removeRenderItem(m_renderItemId);
}

/*
 */
void
ShapeComponent::onTransformChanged()
{
  updateRenderItem();
}

/*
 */
void
ShapeComponent::updateRenderItem()
{
  if (!isRegistered()) {
    return;
  }

  RenderItem& item = getOwner()->getScene()->getRenderItem(m_renderItemId);
  item.worldMatrix = getOwner()->getTransform().getWorldMatrix();
  item.worldBounds = m_mesh->getBounds().getTransformed(item.worldMatrix);
  item.mesh = m_mesh.get();
  item.material = m_material ? &m_material->getMaterial() : nullptr;
}

} // namespace chEngineSDK
