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

#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chITexture.h"
#include "chImGuiRenderer.h"
#include "chLogger.h"
#include "chMaterialAsset.h"
#include "chModel.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chTextureAsset.h"

#include "imgui.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(InspectorLog, All);

namespace {
/*
 * Makes the last item a drop target for textures; true when one was set this frame.
 */
bool
acceptTextureDrop(Material& material)
{
  if (!ImGui::BeginDragDropTarget()) {
    return false;
  }
  bool bDropped = false;
  if (SPtr<TextureAsset> dropped = AssetDragDrop::acceptTexture()) {
    material.setBaseColorTexture(dropped);
    bDropped = true;
  }
  ImGui::EndDragDropTarget();
  return bDropped;
}

/*
 */
void
saveMaterial(const SPtr<MaterialAsset>& materialAsset)
{
  if (!AssetManager::instance().saveAsset(materialAsset)) {
    CH_LOG_ERROR(InspectorLog, "Failed to save material {0}", materialAsset->getName());
  }
}
} // namespace

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

  const uint32 slotCount = modelComponent.getMaterialSlotCount();
  if (slotCount == 0 || !ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  for (uint32 slot = 0; slot < slotCount; ++slot) {
    ImGui::PushID(static_cast<int32>(slot));
    renderMaterialSlot(modelComponent, slot);
    ImGui::PopID();
  }
  ImGui::TreePop();
}

/*
 */
void
InspectorUI::renderMaterialSlot(ModelComponent& modelComponent, uint32 slot)
{
  const ModelMaterialSlot& modelSlot = modelComponent.getModel()->getMaterialSlots()[slot];
  if (!ImGui::TreeNodeEx("##slot", ImGuiTreeNodeFlags_DefaultOpen, "%s",
                         modelSlot.name.c_str())) {
    return;
  }

  // Dropping a material replaces it on this object only.
  const SPtr<MaterialAsset>& material = modelComponent.getMaterial(slot);
  const bool bOverride = modelComponent.getMaterialOverride(slot) != nullptr;
  ImGui::Button(material ? material->getName() : "None (drop a material)",
                ImVec2(-FLT_MIN, 0.0f));
  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<MaterialAsset> dropped = AssetDragDrop::acceptMaterial()) {
      modelComponent.setMaterial(slot, dropped);
    }
    ImGui::EndDragDropTarget();
  }
  if (bOverride) {
    ImGui::TextDisabled("Only on this object");
    ImGui::SameLine();
    if (ImGui::SmallButton("Use model's")) {
      modelComponent.setMaterial(slot, nullptr);
    }
  }

  // Editing the material changes the shared asset, so every object that uses it follows.
  if (material) {
    renderMaterial(material);
  }
  ImGui::TreePop();
}

/*
 */
void
InspectorUI::renderMaterial(const SPtr<MaterialAsset>& materialAsset)
{
  Material& material = materialAsset->getMaterial();

  const LinearColor& baseColor = material.getBaseColorFactor();
  float color[4] = {baseColor.r, baseColor.g, baseColor.b, baseColor.a};
  if (ImGui::ColorEdit4("Base Color", color)) {
    material.setBaseColorFactor(LinearColor(color[0], color[1], color[2], color[3]));
  }
  // Saved once the edit ends instead of on every frame of a drag.
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    saveMaterial(materialAsset);
  }

  const SPtr<TextureAsset>& texture = material.getBaseColorTexture();
  const ITexture* gpuTexture = material.getBaseColorGpuTexture();
  const uint64 textureId =
      gpuTexture ? ImGuiRenderer::getTextureId(gpuTexture->getBindlessIndex()) : 0;
  constexpr float kThumbnailSize = 48.0f;
  if (textureId != 0) {
    ImGui::Image(static_cast<ImTextureID>(textureId), ImVec2(kThumbnailSize, kThumbnailSize));
  }
  else {
    ImGui::Button("##noTexture", ImVec2(kThumbnailSize, kThumbnailSize));
  }
  const bool bDroppedTexture = acceptTextureDrop(material);
  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::TextUnformatted("Base Color Texture");
  ImGui::TextDisabled("%s", texture ? texture->getName() : "None (drop a texture)");
  bool bClearedTexture = false;
  if (texture && ImGui::SmallButton("Clear")) {
    material.setBaseColorTexture(nullptr);
    bClearedTexture = true;
  }
  ImGui::EndGroup();
  const bool bDroppedOnText = acceptTextureDrop(material);

  if (bDroppedTexture || bDroppedOnText || bClearedTexture) {
    saveMaterial(materialAsset);
  }
}

} // namespace chEngineSDK
