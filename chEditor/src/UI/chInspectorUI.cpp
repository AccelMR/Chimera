/************************************************************************/
/**
 * @file chInspectorUI.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Inspector UI class for the Chimera Editor.
 */
/************************************************************************/

#include "chInspectorUI.h"

#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chModel.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRotator.h"

#include "imgui.h"

namespace chEngineSDK {

/*
 */
void
InspectorUI::renderInspectorUI()
{
  ImGui::Begin("Inspector", nullptr);
  const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
  if (!selected) {
    m_rotationOwner = nullptr;
    ImGui::End();
    return;
  }

  ImGui::TextUnformatted(selected->getName().c_str());
  ImGui::Separator();
  renderTransform(*selected);

  if (ModelComponent* modelComponent = selected->getComponent<ModelComponent>()) {
    renderModelComponent(*modelComponent);
  }
  ImGui::End();
}

/*
 */
void
InspectorUI::renderTransform(GameObject& gameObject)
{
  if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  Transform& transform = gameObject.getTransform();
  if (m_rotationOwner != &gameObject) {
    m_rotationOwner = &gameObject;
    const Rotator rotator = transform.getLocalRotation().toRotator();
    m_rotationDegrees[0] = rotator.pitch.valueDegree();
    m_rotationDegrees[1] = rotator.yaw.valueDegree();
    m_rotationDegrees[2] = rotator.roll.valueDegree();
  }

  const Vector3& position = transform.getLocalPosition();
  float positionValues[3] = {position.x, position.y, position.z};
  if (ImGui::DragFloat3("Position", positionValues, 0.01f)) {
    transform.setLocalPosition(
        Vector3(positionValues[0], positionValues[1], positionValues[2]));
  }

  // Pitch, yaw, roll, in the order Rotator takes them.
  if (ImGui::DragFloat3("Rotation", m_rotationDegrees, 0.5f)) {
    transform.setLocalRotation(
        Quaternion(Rotator(m_rotationDegrees[0], m_rotationDegrees[1], m_rotationDegrees[2])));
  }

  const Vector3& scale = transform.getLocalScale();
  float scaleValues[3] = {scale.x, scale.y, scale.z};
  if (ImGui::DragFloat3("Scale", scaleValues, 0.01f)) {
    transform.setLocalScale(Vector3(scaleValues[0], scaleValues[1], scaleValues[2]));
  }
}

/*
 */
void
InspectorUI::renderModelComponent(ModelComponent& modelComponent)
{
  if (!ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  bool bVisible = modelComponent.isEnabled();
  if (ImGui::Checkbox("Visible", &bVisible)) {
    modelComponent.setEnabled(bVisible);
  }

  const SPtr<Model>& model = modelComponent.getModel();
  ImGui::Text("Nodes: %u", model ? model->getNodeCount() : 0u);
  ImGui::TextUnformatted(modelComponent.getTexture() ? "Texture: set" :
                                                       "Texture: default (white)");
}

} // namespace chEngineSDK
