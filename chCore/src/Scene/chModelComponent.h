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
 */
class CH_CORE_EXPORT ModelComponent : public Component
{
 public:
  ModelComponent() = default;

  explicit ModelComponent(const SPtr<ModelAsset>& modelAsset);

  /**
   * Draws one node (an index of Model::getAllNodes) and one of its meshes, or all of them
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
   * Box around everything it draws, in world space as of the last transform update. Zero at
   * the origin when it draws nothing.
   */
  NODISCARD AABox
  getWorldBounds() const;

  /**
   * Box around what every ModelComponent of gameObject and its children draws. False when
   * none draws anything.
   */
  NODISCARD static bool
  getHierarchyWorldBounds(const GameObject& gameObject, AABox& outBounds);

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
  };

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
};
DECLARE_TYPE_TRAITS(ModelComponent)

} // namespace chEngineSDK
