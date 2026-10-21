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
#include "chSkeleton.h"
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
SPtr<Skeleton>
Model::buildSkeleton(Span<const uint32> jointNodes)
{
  Vector<uint8> isJoint(m_nodes.size(), 0);
  bool bAnyJoint = false;
  for (const uint32 node : jointNodes) {
    if (node < m_nodes.size()) {
      isJoint[node] = 1;
      bAnyJoint = true;
    }
  }
  if (!bAnyJoint) {
    return nullptr;
  }

  Vector<uint32> depths(m_nodes.size(), 0);
  for (SIZE_T i = 0; i < m_nodes.size(); ++i) {
    depths[i] = m_nodes[i].parent == ModelNode::NO_PARENT ? 0 : depths[m_nodes[i].parent] + 1;
  }

  // NO_PARENT when the joints are under different roots: then every node above them is a
  // bone.
  uint32 common = ModelNode::NO_PARENT;
  bool bFirst = true;
  for (uint32 joint = 0; joint < m_nodes.size(); ++joint) {
    if (!isJoint[joint]) {
      continue;
    }
    if (bFirst) {
      common = joint;
      bFirst = false;
      continue;
    }
    uint32 a = common;
    uint32 b = joint;
    while (a != ModelNode::NO_PARENT && b != ModelNode::NO_PARENT && depths[a] > depths[b]) {
      a = m_nodes[a].parent;
    }
    while (a != ModelNode::NO_PARENT && b != ModelNode::NO_PARENT && depths[b] > depths[a]) {
      b = m_nodes[b].parent;
    }
    while (a != b) {
      a = a == ModelNode::NO_PARENT ? a : m_nodes[a].parent;
      b = b == ModelNode::NO_PARENT ? b : m_nodes[b].parent;
    }
    common = a;
  }

  Vector<uint8> isBone(m_nodes.size(), 0);
  for (uint32 joint = 0; joint < m_nodes.size(); ++joint) {
    if (!isJoint[joint]) {
      continue;
    }
    for (uint32 node = joint; node != ModelNode::NO_PARENT && !isBone[node];
         node = m_nodes[node].parent) {
      isBone[node] = 1;
      if (node == common) {
        break;
      }
    }
  }

  SPtr<Skeleton> skeleton = chMakeShared<Skeleton>();
  for (uint32 node = 0; node < m_nodes.size(); ++node) {
    ModelNode& source = m_nodes[node];
    if (!isBone[node]) {
      source.bone = ModelNode::NO_BONE;
      continue;
    }
    const uint32 parent = source.parent;
    const uint32 parentBone = parent != ModelNode::NO_PARENT && isBone[parent]
                                  ? m_nodes[parent].bone
                                  : SkeletonBone::NO_PARENT;
    source.bone = skeleton->addBone({.name = source.name,
                                     .parent = parentBone,
                                     .position = source.position,
                                     .rotation = source.rotation,
                                     .scale = source.scale});
  }
  return skeleton;
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
