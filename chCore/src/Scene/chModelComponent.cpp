/************************************************************************/
/**
 * @file chModelComponent.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model, or one part of it, at its GameObject.
 */
/************************************************************************/
#include "chModelComponent.h"

#include "chAlgorithm.h"
#include "chGameObject.h"
#include "chMaterialAsset.h"
#include "chMatrixHelpers.h"
#include "chModel.h"
#include "chScene.h"

namespace chEngineSDK {
namespace {
// Returned by reference for slots that do not exist.
const SPtr<MaterialAsset> kNoMaterial;

/*
 * Calls visit(mesh, localMatrix) for every mesh the part draws, localMatrix being where the
 * mesh sits relative to the GameObject. An index past the end draws nothing.
 */
template<typename Visitor>
void
forEachDrawnMesh(const Model& model,
                 uint32 nodeIndex,
                 uint32 meshIndex,
                 const Vector3& pivot,
                 Visitor&& visit)
{
  const Vector<ModelNode*>& nodes = model.getAllNodes();
  if (nodeIndex == ModelComponent::ALL) {
    for (const ModelNode* node : nodes) {
      for (const SPtr<Mesh>& mesh : node->getMeshes()) {
        visit(*mesh, node->getGlobalTransform());
      }
    }
    return;
  }
  if (nodeIndex >= nodes.size()) {
    return;
  }

  // The node's own transform went into the GameObject; only the pivot is left.
  const Matrix4 localMatrix = TranslationMatrix(-pivot);
  const Vector<SPtr<Mesh>>& meshes = nodes[nodeIndex]->getMeshes();
  if (meshIndex == ModelComponent::ALL) {
    for (const SPtr<Mesh>& mesh : meshes) {
      visit(*mesh, localMatrix);
    }
  }
  else if (meshIndex < meshes.size()) {
    visit(*meshes[meshIndex], localMatrix);
  }
}
} // namespace

/*
 */
ModelComponent::ModelComponent(const SPtr<Model>& model)
{
  setModel(model);
}

/*
 */
ModelComponent::ModelComponent(const SPtr<Model>& model,
                               uint32 nodeIndex,
                               uint32 meshIndex,
                               const Vector3& pivot)
{
  setModelPart(model, nodeIndex, meshIndex, pivot);
}

/*
 */
void
ModelComponent::setModel(const SPtr<Model>& model)
{
  setModelPart(model, ALL, ALL, Vector3::ZERO);
}

/*
 */
void
ModelComponent::setModelPart(const SPtr<Model>& model,
                             uint32 nodeIndex,
                             uint32 meshIndex,
                             const Vector3& pivot)
{
  if (isRegistered()) {
    removeRenderItems();
  }

  m_model = model;
  m_nodeIndex = nodeIndex;
  m_meshIndex = meshIndex;
  m_pivot = pivot;
  if (m_model) {
    // The node matrices are read when the items are placed, so they must be current.
    m_model->updateTransforms();
  }
  rebuildUsedSlots();

  if (isRegistered()) {
    addRenderItems();
  }
}

/*
 */
void
ModelComponent::rebuildUsedSlots()
{
  m_usedSlots.clear();
  m_materialOverrides.clear();
  if (!m_model) {
    return;
  }

  const SIZE_T slotCount = m_model->getMaterialSlots().size();
  forEachDrawnMesh(*m_model, m_nodeIndex, m_meshIndex, m_pivot,
                   [this, slotCount](const Mesh& mesh, const Matrix4&) {
                     const uint32 slot = mesh.getMaterialSlot();
                     if (slot < slotCount && !Algorithm::contains(m_usedSlots, slot)) {
                       m_usedSlots.push_back(slot);
                     }
                   });
  Algorithm::sort(m_usedSlots);
  m_materialOverrides.resize(m_usedSlots.size());
}

/*
 */
uint32
ModelComponent::findUsedSlot(uint32 slot) const
{
  const SIZE_T index = Algorithm::lowerBound(m_usedSlots, slot);
  if (index < m_usedSlots.size() && m_usedSlots[index] == slot) {
    return static_cast<uint32>(index);
  }
  return ALL;
}

/*
 */
void
ModelComponent::setMaterial(uint32 slot, const SPtr<MaterialAsset>& material)
{
  const uint32 index = findUsedSlot(slot);
  if (index == ALL) {
    return;
  }

  m_materialOverrides[index] = material;
  if (!isRegistered()) {
    return;
  }

  Scene& scene = *getOwner()->getScene();
  const Material* renderMaterial = getRenderMaterial(slot);
  for (const MeshPart& part : m_parts) {
    if (part.mesh->getMaterialSlot() == slot) {
      scene.getRenderItem(part.renderItemId).material = renderMaterial;
    }
  }
}

/*
 */
const SPtr<MaterialAsset>&
ModelComponent::getMaterialOverride(uint32 slot) const
{
  const uint32 index = findUsedSlot(slot);
  return index != ALL ? m_materialOverrides[index] : kNoMaterial;
}

/*
 */
const SPtr<MaterialAsset>&
ModelComponent::getMaterial(uint32 slot) const
{
  if (!m_model || slot >= m_model->getMaterialSlots().size()) {
    return kNoMaterial;
  }
  const SPtr<MaterialAsset>& materialOverride = getMaterialOverride(slot);
  if (materialOverride) {
    return materialOverride;
  }
  return m_model->getMaterialSlots()[slot].material;
}

/*
 */
const Material*
ModelComponent::getRenderMaterial(uint32 slot) const
{
  const SPtr<MaterialAsset>& material = getMaterial(slot);
  return material ? &material->getMaterial() : nullptr;
}

/*
 */
AABox
ModelComponent::getWorldBounds() const
{
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (!isRegistered() || m_parts.empty()) {
    return bounds;
  }

  Scene& scene = *getOwner()->getScene();
  bounds = scene.getRenderItem(m_parts[0].renderItemId).worldBounds.getBox();
  for (const MeshPart& part : Span<const MeshPart>(m_parts).subspan(1)) {
    bounds += scene.getRenderItem(part.renderItemId).worldBounds.getBox();
  }
  return bounds;
}

/*
 */
bool
ModelComponent::getHierarchyWorldBounds(const GameObject& gameObject, AABox& outBounds)
{
  bool bFound = false;
  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    if (component->getTypeId() != TypeTraits<ModelComponent>::getTypeId()) {
      continue;
    }
    const ModelComponent& model = static_cast<const ModelComponent&>(*component);
    if (!model.isRegistered() || model.m_parts.empty()) {
      continue;
    }
    const AABox modelBounds = model.getWorldBounds();
    if (bFound) {
      outBounds += modelBounds;
    }
    else {
      outBounds = modelBounds;
      bFound = true;
    }
  }

  for (const SPtr<GameObject>& child : gameObject.getChildren()) {
    AABox childBounds(Vector3::ZERO, Vector3::ZERO);
    if (!getHierarchyWorldBounds(*child, childBounds)) {
      continue;
    }
    if (bFound) {
      outBounds += childBounds;
    }
    else {
      outBounds = childBounds;
      bFound = true;
    }
  }
  return bFound;
}

/*
 */
void
ModelComponent::onRegister()
{
  addRenderItems();
}

/*
 */
void
ModelComponent::onUnregister()
{
  removeRenderItems();
}

/*
 */
void
ModelComponent::onTransformChanged()
{
  Scene& scene = *getOwner()->getScene();
  const Matrix4& ownerWorld = getOwner()->getTransform().getWorldMatrix();
  for (const MeshPart& part : m_parts) {
    RenderItem& item = scene.getRenderItem(part.renderItemId);
    item.worldMatrix = part.localMatrix * ownerWorld;
    item.worldBounds = part.mesh->getBounds().getTransformed(item.worldMatrix);
  }
}

/*
 */
void
ModelComponent::addRenderItems()
{
  if (!m_model) {
    return;
  }

  Scene& scene = *getOwner()->getScene();
  const Matrix4& ownerWorld = getOwner()->getTransform().getWorldMatrix();
  forEachDrawnMesh(
      *m_model, m_nodeIndex, m_meshIndex, m_pivot,
      [this, &scene, &ownerWorld](const Mesh& mesh, const Matrix4& localMatrix) {
        // Row vectors: the mesh's place in the object applies first, then the object's.
        const Matrix4 world = localMatrix * ownerWorld;
        const RenderItem item{.worldMatrix = world,
                              .worldBounds = mesh.getBounds().getTransformed(world),
                              .mesh = &mesh,
                              .material = getRenderMaterial(mesh.getMaterialSlot()),
                              .owner = getOwner()};
        const uint32 renderItemId = scene.addRenderItem(item);
        m_parts.push_back(
            {.localMatrix = localMatrix, .mesh = &mesh, .renderItemId = renderItemId});
      });
}

/*
 */
void
ModelComponent::removeRenderItems()
{
  Scene& scene = *getOwner()->getScene();
  for (const MeshPart& part : m_parts) {
    scene.removeRenderItem(part.renderItemId);
  }
  m_parts.clear();
}

} // namespace chEngineSDK
