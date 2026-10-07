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
   * Drawn on every mesh until materials exist; null uses the renderer's default.
   */
  void
  setTexture(const SPtr<ITexture>& texture);

  NODISCARD FORCEINLINE const SPtr<ITexture>&
  getTexture() const { return m_texture; }

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

  SPtr<Model> m_model;
  SPtr<ITexture> m_texture;
  Vector<MeshPart> m_parts;
};
DECLARE_TYPE_TRAITS(ModelComponent)

} // namespace chEngineSDK
