/************************************************************************/
/**
 * @file chModel.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Model made of a hierarchy of nodes that draw meshes.
 */
/************************************************************************/
#include "chModel.h"

#include "chMatrixHelpers.h"
#include "chSphereBoxBounds.h"
#include "chVector4.h"

namespace chEngineSDK {

/*
 */
uint32
Model::addMesh(SPtr<Mesh> mesh)
{
  m_meshes.push_back(std::move(mesh));
  return static_cast<uint32>(m_meshes.size() - 1);
}

/*
 */
uint32
Model::addNode(StringView name,
               uint32 parent,
               const Vector3& position,
               const Quaternion& rotation,
               const Vector3& scale,
               Span<const uint32> meshIndices)
{
  const uint32 index = static_cast<uint32>(m_nodes.size());
  CH_ASSERT(parent == ModelNode::NO_PARENT || parent < index);
  if (parent != ModelNode::NO_PARENT && parent >= index) {
    parent = ModelNode::NO_PARENT;
  }

  ModelNode& node = m_nodes.emplace_back();
  node.name = String(name);
  node.parent = parent;
  node.position = position;
  node.rotation = rotation;
  node.scale = scale;
  node.firstMesh = static_cast<uint32>(m_nodeMeshes.size());
  for (const uint32 meshIndex : meshIndices) {
    CH_ASSERT(meshIndex < m_meshes.size());
    if (meshIndex < m_meshes.size()) {
      m_nodeMeshes.push_back(meshIndex);
    }
  }
  node.meshCount = static_cast<uint32>(m_nodeMeshes.size()) - node.firstMesh;

  // Row vectors: the node's own transform applies first, then its parent's.
  const Matrix4 localMatrix = ScaleRotationTranslationMatrix(scale, rotation, position);
  m_restMatrices.push_back(parent == ModelNode::NO_PARENT
                               ? localMatrix
                               : localMatrix * m_restMatrices[parent]);
  return index;
}

/*
 */
uint32
Model::addNode(StringView name,
               uint32 parent,
               const Matrix4& localMatrix,
               Span<const uint32> meshIndices)
{
  Vector3 scale = Vector3::UNIT;
  Quaternion rotation = Quaternion::IDENTITY;
  Vector3 position = Vector3::ZERO;
  if (!localMatrix.decompose(scale, rotation, position)) {
    // A zero scale hides what the node draws, as the source matrix did.
    scale = Vector3::ZERO;
    const Vector4 origin = localMatrix.transformPosition(Vector3::ZERO);
    position = Vector3(origin.x, origin.y, origin.z);
  }
  return addNode(name, parent, position, rotation, scale, meshIndices);
}

/*
 */
uint32
Model::addMaterialSlot(ModelMaterialSlot slot)
{
  m_materialSlots.push_back(std::move(slot));
  return static_cast<uint32>(m_materialSlots.size() - 1);
}

/*
 */
void
Model::setSlotMaterial(uint32 slot, const SPtr<MaterialAsset>& material)
{
  if (slot < m_materialSlots.size()) {
    m_materialSlots[slot].material = material;
  }
}

/*
 */
void
Model::setSkeleton(const UUID& skeletonId, const SPtr<SkeletonAsset>& skeleton)
{
  m_skeletonId = skeletonId;
  m_skeleton = skeleton;
}

/*
 */
void
Model::setNodeBone(uint32 nodeIndex, uint32 bone)
{
  if (nodeIndex < m_nodes.size()) {
    m_nodes[nodeIndex].bone = bone;
  }
}

/*
 */
SphereBoxBounds
Model::calculateBounds() const
{
  bool bFirst = true;
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  for (uint32 nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
    for (const uint32 meshIndex : getNodeMeshes(nodeIndex)) {
      const AABox meshBounds =
          m_meshes[meshIndex]->getBounds().getBox().getTransformed(m_restMatrices[nodeIndex]);
      if (bFirst) {
        bounds = meshBounds;
        bFirst = false;
      }
      else {
        bounds += meshBounds;
      }
    }
  }
  return SphereBoxBounds(bounds);
}

} // namespace chEngineSDK
