/************************************************************************/
/**
 * @file chInspectorUI.h
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Inspector UI class for the Chimera Editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chQuaternion.h"

namespace chEngineSDK {
class Material;
class MaterialAsset;
enum class MaterialTexture : uint32;

/**
 * Editor window that shows and edits the transform and components of the selected
 * GameObject.
 */
class InspectorUI
{
 public:
  InspectorUI() = default;

  void
  renderInspectorUI();

 private:
  void
  renderTransform(GameObject& gameObject);

  void
  renderModelComponent(ModelComponent& modelComponent);

  void
  renderLightComponent(GameObject& gameObject);

  void
  renderMaterialSlot(ModelComponent& modelComponent, uint32 slot);

  void
  renderMaterial(const SPtr<MaterialAsset>& materialAsset);

  // True when the texture changed, so the material needs saving.
  NODISCARD bool
  renderMaterialTexture(Material& material, MaterialTexture slot);

  // The rotation is edited as angles but stored as a quaternion, and converting back each
  // frame would make the angles jump (a yaw of 180 can come back as pitch and roll 180).
  // So the angles are kept while the same object stays selected.
  const GameObject* m_rotationOwner = nullptr;
  float m_rotationDegrees[3] = {0.0f, 0.0f, 0.0f};
  // The rotation the angles were last read from or written to.
  Quaternion m_rotationShown = Quaternion::IDENTITY;
};

} // namespace chEngineSDK
