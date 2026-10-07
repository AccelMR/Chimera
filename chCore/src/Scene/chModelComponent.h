/************************************************************************/
/**
 * @file chModelComponent.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model at its GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chBox.h"
#include "chComponent.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class Material;
class MaterialAsset;
class ModelNode;

/**
 * Exists to put a Model in a scene. While registered it adds one render item per mesh of
 * each model node to the scene, and moves them when the GameObject moves, so the renderer
 * never looks at components.
 */
class CH_CORE_EXPORT ModelComponent : public Component
{
 public:
  ModelComponent() = default;

  explicit ModelComponent(const SPtr<Model>& model);

  void
  setModel(const SPtr<Model>& model);

  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const { return m_model; }

  /**
   * As many as the model has.
   */
  NODISCARD uint32
  getMaterialSlotCount() const;

  /**
   * Replaces the model's material of that slot on this object only; null goes back to it.
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
    const ModelNode* node = nullptr;
    const Mesh* mesh = nullptr;
    uint32 renderItemId = 0;
  };

  void
  addRenderItems();

  void
  removeRenderItems();

  NODISCARD const Material*
  getRenderMaterial(uint32 slot) const;

  SPtr<Model> m_model;
  // One per material slot of the model; null uses the model's material.
  Vector<SPtr<MaterialAsset>> m_materialOverrides;
  Vector<MeshPart> m_parts;
};
DECLARE_TYPE_TRAITS(ModelComponent)

} // namespace chEngineSDK
