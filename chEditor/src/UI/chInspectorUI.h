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
class AnimationClip;
class IAsset;
class Material;
class MaterialAsset;
class ShapeComponent;
class Skeleton;
class SkyLightComponent;
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
  renderName(GameObject& gameObject);

  /**
   * Says which prefab selected comes from, with buttons to edit or unpack it.
   */
  void
  renderPrefabInstance(GameObject& selected, GameObject& instance);

  void
  renderTransform(GameObject& gameObject);

  void
  renderModelComponent(ModelComponent& modelComponent);

  void
  renderShapeComponent(ShapeComponent& shapeComponent);

  void
  renderLightComponent(GameObject& gameObject);

  void
  renderSkyLightComponent(SkyLightComponent& skyLight);

  void
  renderAnimation(ModelComponent& modelComponent);

  void
  renderMaterialSlot(ModelComponent& modelComponent, uint32 slot);

  void
  renderAsset(const SPtr<IAsset>& asset);

  void
  renderAnimationClip(const AnimationClip& clip);

  void
  renderSkeleton(const Skeleton& skeleton);

  void
  renderBone(const Skeleton& skeleton, uint32 bone);

  void
  renderMaterial(const SPtr<MaterialAsset>& materialAsset);

  // True when the texture changed, so the material needs saving.
  NODISCARD bool
  renderMaterialTexture(Material& material, MaterialTexture slot);

  // The rotation is edited as angles but stored as a quaternion, and converting back each
  // frame would make the angles jump (a yaw of 180 can come back as pitch and roll 180).
  // So the angles are kept while the same object stays selected.
  const GameObject* m_rotationOwner = nullptr;
  // Roll, pitch, yaw: the turns around X, Y and Z, so they share the axis colors.
  float m_rotationDegrees[3] = {0.0f, 0.0f, 0.0f};
  // The rotation the angles were last read from or written to.
  Quaternion m_rotationShown = Quaternion::IDENTITY;

  // Read again from the object whenever the field is not being typed in.
  const GameObject* m_nameOwner = nullptr;
  ANSICHAR m_nameBuffer[256] = "";
  bool m_bEditingName = false;

  // The children of each bone of the skeleton last shown, built when it changes, since the
  // skeleton only stores parents.
  const Skeleton* m_childrenOwner = nullptr;
  Vector<Vector<uint32>> m_boneChildren;
};

} // namespace chEngineSDK
