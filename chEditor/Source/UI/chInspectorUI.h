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
class AnimationGraphInstance;
class AnimatorComponent;
class CameraComponent;
class ScriptComponent;
class ShapeComponent;
class SkyLightComponent;

/**
 * Exists so the selected GameObject of the open scene can be seen and edited: its name,
 * transform and components. Assets have their own window, the Asset Inspector.
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
  renderCameraComponent(CameraComponent& camera);

  /**
   * A section per script of the object, then the Add Script button.
   */
  void
  renderScriptComponents(GameObject& gameObject);

  /**
   * True when its Remove button was pressed.
   */
  NODISCARD bool
  renderScriptComponent(ScriptComponent& scriptComponent);

  void
  renderAddScript(GameObject& gameObject);

  void
  renderAddAnimator(ModelComponent& modelComponent);

  /**
   * True when its Remove button was pressed.
   */
  NODISCARD bool
  renderAnimatorComponent(AnimatorComponent& animator);

  /**
   * The state of every layer and the parameters, which can be changed to try the graph in
   * the editor (they are not saved).
   */
  void
  renderAnimationGraphState(AnimationGraphInstance& instance);

  void
  renderMaterialSlot(ModelComponent& modelComponent, uint32 slot);

  // The rotation is edited as angles but stored as a quaternion, and converting back each
  // frame would make the angles jump (a yaw of 180 can come back as pitch and roll 180).
  // So the angles are kept while the same object stays selected.
  const GameObject* m_rotationOwner = nullptr;
  // Roll, pitch, yaw: the turns around X, Y and Z, so they share the axis colors.
  float m_rotationDegrees[3] = {0.0f, 0.0f, 0.0f};
  // The rotation the angles were last read from or written to.
  Quaternion m_rotationShown = Quaternion::IDENTITY;

  // The object shown, kept while the left button is held (see renderInspectorUI).
  WeakPtr<GameObject> m_shownObject;

  // Read again from the object whenever the field is not being typed in.
  const GameObject* m_nameOwner = nullptr;
  ANSICHAR m_nameBuffer[256] = "";
  bool m_bEditingName = false;
};

} // namespace chEngineSDK
