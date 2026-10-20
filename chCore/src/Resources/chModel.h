/************************************************************************/
/**
 * @file chModel.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Model made of a hierarchy of nodes that draw meshes.
 */
/************************************************************************/
#pragma once

#include "chMatrix4.h"
#include "chMesh.h"
#include "chPrerequisitesCore.h"
#include "chQuaternion.h"
#include "chUUID.h"
#include "chVector3.h"

namespace chEngineSDK {
class MaterialAsset;
class SkeletonAsset;

/**
 * One material the meshes of a model can use; each Mesh stores the index of its slot.
 */
struct ModelMaterialSlot
{
  String name;
  // Kept even when the asset is missing, so saving the model again does not lose it.
  UUID materialId = UUID::null();
  SPtr<MaterialAsset> material;
};

/**
 * One node of a Model: its place in the hierarchy, its rest transform relative to its
 * parent, the range of meshes it draws and the bone it is, if any.
 */
struct ModelNode
{
  static constexpr uint32 NO_PARENT = 0xFFFFFFFFu;
  static constexpr uint32 NO_BONE = 0xFFFFFFFFu;

  String name;
  // Lower than the node's own index, so a parent always comes before its children.
  uint32 parent = NO_PARENT;
  Vector3 position = Vector3::ZERO;
  Quaternion rotation = Quaternion::IDENTITY;
  Vector3 scale = Vector3::UNIT;
  // Range in Model::getNodeMeshes.
  uint32 firstMesh = 0;
  uint32 meshCount = 0;
  // The bone of the model's skeleton this node is, or NO_BONE.
  uint32 bone = NO_BONE;
};

/**
 * Exists to hold what a source file describes, read only once built, so every object that
 * draws it shares one copy. Nodes are a flat array with parents before children, which lets
 * the transforms of the whole hierarchy be computed in one pass; meshes are a separate
 * array, so nodes can share them.
 */
class CH_CORE_EXPORT Model
{
 public:
  /**
   * Returns the index of the mesh in getMeshes.
   */
  uint32
  addMesh(SPtr<Mesh> mesh);

  /**
   * The parent must be NO_PARENT or a node added before; meshIndices are indices of
   * getMeshes. Returns the index of the node.
   */
  uint32
  addNode(StringView name,
          uint32 parent,
          const Vector3& position,
          const Quaternion& rotation,
          const Vector3& scale,
          Span<const uint32> meshIndices);

  /**
   * Same as above with the local transform as a matrix, split into its parts. Shear is lost.
   */
  uint32
  addNode(StringView name,
          uint32 parent,
          const Matrix4& localMatrix,
          Span<const uint32> meshIndices);

  /**
   * Returns the index of the new slot.
   */
  uint32
  addMaterialSlot(ModelMaterialSlot slot);

  /**
   * Sets the loaded asset of a slot whose UUID is already known.
   */
  void
  setSlotMaterial(uint32 slot, const SPtr<MaterialAsset>& material);

  /**
   * The skeleton whose bones the nodes name with setNodeBone. The UUID is kept even when
   * the asset is missing, so saving the model again does not lose it.
   */
  void
  setSkeleton(const UUID& skeletonId, const SPtr<SkeletonAsset>& skeleton);

  void
  setNodeBone(uint32 nodeIndex, uint32 bone);

  NODISCARD FORCEINLINE bool
  hasSkeleton() const noexcept { return !m_skeletonId.isNull(); }

  NODISCARD FORCEINLINE const UUID&
  getSkeletonId() const noexcept { return m_skeletonId; }

  /**
   * Null when the model has no skeleton or its asset is missing.
   */
  NODISCARD FORCEINLINE const SPtr<SkeletonAsset>&
  getSkeleton() const noexcept { return m_skeleton; }

  NODISCARD FORCEINLINE const Vector<ModelNode>&
  getNodes() const noexcept { return m_nodes; }

  NODISCARD FORCEINLINE uint32
  getNodeCount() const noexcept { return static_cast<uint32>(m_nodes.size()); }

  NODISCARD FORCEINLINE const Vector<SPtr<Mesh>>&
  getMeshes() const noexcept { return m_meshes; }

  NODISCARD FORCEINLINE uint32
  getMeshCount() const noexcept { return static_cast<uint32>(m_meshes.size()); }

  /**
   * Indices into getMeshes of the meshes the node draws.
   */
  NODISCARD FORCEINLINE Span<const uint32>
  getNodeMeshes(uint32 nodeIndex) const
  {
    const ModelNode& node = m_nodes[nodeIndex];
    return Span<const uint32>(m_nodeMeshes).subspan(node.firstMesh, node.meshCount);
  }

  /**
   * The node's rest transform in the model's space.
   */
  NODISCARD FORCEINLINE const Matrix4&
  getNodeRestMatrix(uint32 nodeIndex) const { return m_restMatrices[nodeIndex]; }

  NODISCARD FORCEINLINE const Vector<ModelMaterialSlot>&
  getMaterialSlots() const noexcept { return m_materialSlots; }

  /**
   * Bounds of every mesh of every node, in the model's space, in the rest pose.
   */
  NODISCARD SphereBoxBounds
  calculateBounds() const;

 private:
  Vector<ModelNode> m_nodes;
  // One per node.
  Vector<Matrix4> m_restMatrices;
  // Every node's mesh indices one after the other; each node keeps its range.
  Vector<uint32> m_nodeMeshes;
  Vector<SPtr<Mesh>> m_meshes;
  Vector<ModelMaterialSlot> m_materialSlots;
  UUID m_skeletonId = UUID::null();
  SPtr<SkeletonAsset> m_skeleton;
};

} // namespace chEngineSDK
