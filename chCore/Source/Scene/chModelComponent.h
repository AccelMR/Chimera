/************************************************************************/
/**
 * @file chModelComponent.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model, or one part of it, at its GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chBox.h"
#include "chComponent.h"
#include "chMatrix4.h"
#include "chTypeTraits.h"
#include "chVector3.h"

namespace chEngineSDK {
class Material;
class MaterialAsset;
class ModelAsset;

/**
 * Exists to put a Model in a scene. It draws the whole model, the meshes of one node or a
 * single mesh, so a model can be split into objects that move on their own. While
 * registered it adds one render item per mesh it draws to the scene, and moves them when
 * the GameObject moves, so the renderer never looks at components.
 *
 * A whole model can also be posed by whoever animates it (an AnimatorComponent on the same
 * GameObject): setPose moves the render items of the nodes' meshes. The pose never becomes
 * GameObjects, so bones stay out of the scene graph. Skinned meshes of a whole model give
 * their render items the bone matrices of the pose, which the renderer uses to move the
 * vertices, and bounds made from the boxes of their posed bones.
 */
class CH_CORE_EXPORT ModelComponent : public Component
{
 public:
  ModelComponent();

  explicit ModelComponent(const SPtr<ModelAsset>& modelAsset);

  /**
   * Draws one node (an index of Model::getNodes) and one of its meshes, or all of them
   * with ALL. The object stands at pivot, a point in the node's space, so the meshes are
   * drawn moved back by it.
   */
  ModelComponent(const SPtr<ModelAsset>& modelAsset,
                 uint32 nodeIndex,
                 uint32 meshIndex,
                 const Vector3& pivot);

  /**
   * Draws the whole model and forgets every override.
   */
  void
  setModel(const SPtr<ModelAsset>& modelAsset);

  /**
   * Same as the constructor with a part; forgets every override.
   */
  void
  setModelPart(const SPtr<ModelAsset>& modelAsset,
               uint32 nodeIndex,
               uint32 meshIndex,
               const Vector3& pivot);

  NODISCARD FORCEINLINE const SPtr<ModelAsset>&
  getModelAsset() const { return m_modelAsset; }

  /**
   * The model of the asset, kept while the component uses it, even if the asset unloads.
   */
  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const { return m_model; }

  NODISCARD FORCEINLINE bool
  isWholeModel() const { return m_nodeIndex == ALL; }

  NODISCARD FORCEINLINE uint32
  getNodeIndex() const { return m_nodeIndex; }

  NODISCARD FORCEINLINE uint32
  getMeshIndex() const { return m_meshIndex; }

  NODISCARD FORCEINLINE const Vector3&
  getPivot() const { return m_pivot; }

  /**
   * The model's material slots the drawn meshes use, in increasing order.
   */
  NODISCARD FORCEINLINE const Vector<uint32>&
  getUsedMaterialSlots() const { return m_usedSlots; }

  /**
   * Replaces the model's material of that slot on this object only; null goes back to it.
   * Ignored for a slot the drawn meshes do not use.
   */
  void
  setMaterial(uint32 slot, const SPtr<MaterialAsset>& material);

  /**
   * Null when the slot uses the model's material.
   */
  NODISCARD const SPtr<MaterialAsset>&
  getMaterialOverride(uint32 slot) const;

  /**
   * The override, else the model's material; null when neither exists.
   */
  NODISCARD const SPtr<MaterialAsset>&
  getMaterial(uint32 slot) const;

  /**
   * One matrix per node of the model, in the model's space, that the meshes follow until
   * the next call; empty goes back to the rest pose. The matrices are not copied, so they
   * must live until the pose is set again or cleared. Ignored by a part and when the count
   * is not the model's node count. Call it after each change of the matrices.
   */
  void
  setPose(Span<const Matrix4> nodeMatrices);

  /**
   * The matrices setPose was given; empty at rest.
   */
  NODISCARD FORCEINLINE Span<const Matrix4>
  getPose() const noexcept { return m_pose; }

  /**
   * Where the node stands in the model's space in the current pose: the rest pose when
   * nothing poses it.
   */
  NODISCARD const Matrix4&
  getNodeMatrix(uint32 nodeIndex) const;

  NODISCARD bool
  getWorldBounds(AABox& outBounds) const override;

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

  static constexpr uint32 ALL = 0xFFFFFFFFu;
  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  void
  onRegister() override;

  void
  onUnregister() override;

  void
  onTransformChanged() override;

 private:
  struct MeshPart
  {
    // Where the mesh sits relative to the GameObject.
    Matrix4 localMatrix;
    const Mesh* mesh = nullptr;
    uint32 renderItemId = 0;
    // The model node that draws the mesh, or ALL for a part (its node is the GameObject).
    uint32 node = ALL;
    // Skinned meshes of a whole model: their first matrix in m_skinMatrices, else ALL.
    uint32 firstSkinMatrix = ALL;
  };

  /**
   * Gives each part drawn by a node its matrix in the current pose, then moves the render
   * items.
   */
  void
  refreshPartMatrices();

  /**
   * Rebuilds the skin matrices of the skinned parts from the current pose and gives their
   * render items a new revision, so the renderer skins them again.
   */
  void
  updateSkinMatrices();

  /**
   * Writes the world matrix and bounds of each render item from its part's local matrix;
   * a skinned part takes the object's matrix and the bounds of its posed bones.
   */
  void
  updateRenderItems();

  void
  rebuildUsedSlots();

  void
  addRenderItems();

  void
  removeRenderItems();

  NODISCARD const Material*
  getRenderMaterial(uint32 slot) const;

  /**
   * Index in m_usedSlots, or ALL when the slot is not used.
   */
  NODISCARD uint32
  findUsedSlot(uint32 slot) const;

  SPtr<ModelAsset> m_modelAsset;
  SPtr<Model> m_model;
  // Kept when the asset is missing, so saving again keeps the reference.
  UUID m_modelAssetId = UUID::null();
  uint32 m_nodeIndex = ALL;
  uint32 m_meshIndex = ALL;
  Vector3 m_pivot = Vector3::ZERO;
  Vector<uint32> m_usedSlots;
  // One per used slot; null uses the model's material.
  Vector<SPtr<MaterialAsset>> m_materialOverrides;
  Vector<MeshPart> m_parts;

  // Owned by whoever animates the model; one per node, or empty at rest.
  Span<const Matrix4> m_pose;
  // The skin matrices of every skinned part one after the other; their render items point
  // here, so it is sized once when the items are added and never grows while they exist.
  Vector<Matrix4> m_skinMatrices;
};
DECLARE_TYPE_TRAITS(ModelComponent)

} // namespace chEngineSDK
