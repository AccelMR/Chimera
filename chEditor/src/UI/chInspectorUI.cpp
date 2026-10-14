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
#include "chLightComponent.h"
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
acceptTextureDrop(Material& material, MaterialTexture slot)
{
  if (!ImGui::BeginDragDropTarget()) {
    return false;
  }
  bool bDropped = false;
  if (SPtr<TextureAsset> dropped = AssetDragDrop::acceptTexture()) {
    material.setTexture(slot, dropped);
    bDropped = true;
  }
  ImGui::EndDragDropTarget();
  return bDropped;
}

/*
 */
const ANSICHAR*
getTextureLabel(MaterialTexture slot)
{
  switch (slot) {
    case MaterialTexture::BaseColor:
      return "Base Color";
    case MaterialTexture::MetallicRoughness:
      return "Metallic (B) Roughness (G)";
    case MaterialTexture::Normal:
      return "Normal";
    case MaterialTexture::Emissive:
      return "Emissive";
    case MaterialTexture::Occlusion:
      return "Occlusion (R)";
    case MaterialTexture::COUNT:
      break;
  }
  return "";
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
  renderLightComponent(*selected);
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
InspectorUI::renderLightComponent(GameObject& gameObject)
{
  PointLightComponent* pointLight = gameObject.getComponent<PointLightComponent>();
  SpotLightComponent* spotLight = gameObject.getComponent<SpotLightComponent>();
  LightComponent* light = gameObject.getComponent<DirectionalLightComponent>();
  const ANSICHAR* title = "Directional Light";
  if (pointLight) {
    light = pointLight;
    title = "Point Light";
  }
  else if (spotLight) {
    light = spotLight;
    title = "Spot Light";
  }
  if (!light || !ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  bool bEnabled = light->isEnabled();
  if (ImGui::Checkbox("Enabled", &bEnabled)) {
    light->setEnabled(bEnabled);
  }

  const LinearColor& color = light->getColor();
  float colorValues[3] = {color.r, color.g, color.b};
  if (ImGui::ColorEdit3("Color", colorValues, ImGuiColorEditFlags_Float)) {
    light->setColor(LinearColor(colorValues[0], colorValues[1], colorValues[2], 1.0f));
  }

  float intensity = light->getIntensity();
  if (ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 10000.0f)) {
    light->setIntensity(intensity);
  }

  if (pointLight) {
    float range = pointLight->getRange();
    if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 100000.0f)) {
      pointLight->setRange(range);
    }
  }

  if (spotLight) {
    float range = spotLight->getRange();
    if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 100000.0f)) {
      spotLight->setRange(range);
    }
    float inner = spotLight->getInnerConeAngle().valueDegree();
    float outer = spotLight->getOuterConeAngle().valueDegree();
    const bool bInnerChanged = ImGui::SliderFloat("Inner Angle", &inner, 0.0f, 89.0f);
    const bool bOuterChanged = ImGui::SliderFloat("Outer Angle", &outer, 0.0f, 89.0f);
    if (bInnerChanged || bOuterChanged) {
      spotLight->setConeAngles(Degree(inner), Degree(outer));
    }
  }
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
  // Saved once an edit ends instead of on every frame of a drag.
  bool bSave = false;

  const LinearColor& baseColor = material.getBaseColorFactor();
  float color[4] = {baseColor.r, baseColor.g, baseColor.b, baseColor.a};
  if (ImGui::ColorEdit4("Base Color", color)) {
    material.setBaseColorFactor(LinearColor(color[0], color[1], color[2], color[3]));
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  float metallic = material.getMetallicFactor();
  if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
    material.setMetallicFactor(metallic);
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  float roughness = material.getRoughnessFactor();
  if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
    material.setRoughnessFactor(roughness);
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  // Above one is allowed: emissive light has no upper limit before the tonemap.
  const LinearColor& emissive = material.getEmissiveFactor();
  float emissiveColor[3] = {emissive.r, emissive.g, emissive.b};
  if (ImGui::ColorEdit3("Emissive", emissiveColor,
                        ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float)) {
    material.setEmissiveFactor(
        LinearColor(emissiveColor[0], emissiveColor[1], emissiveColor[2], 1.0f));
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  float normalScale = material.getNormalScale();
  if (ImGui::DragFloat("Normal Scale", &normalScale, 0.01f, 0.0f, 4.0f)) {
    material.setNormalScale(normalScale);
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  float occlusionStrength = material.getOcclusionStrength();
  if (ImGui::SliderFloat("Occlusion Strength", &occlusionStrength, 0.0f, 1.0f)) {
    material.setOcclusionStrength(occlusionStrength);
  }
  bSave |= ImGui::IsItemDeactivatedAfterEdit();

  for (uint32 i = 0; i < static_cast<uint32>(MaterialTexture::COUNT); ++i) {
    ImGui::PushID(static_cast<int32>(i));
    bSave |= renderMaterialTexture(material, static_cast<MaterialTexture>(i));
    ImGui::PopID();
  }

  if (bSave) {
    saveMaterial(materialAsset);
  }
}

/*
 */
bool
InspectorUI::renderMaterialTexture(Material& material, MaterialTexture slot)
{
  const SPtr<TextureAsset>& texture = material.getTexture(slot);
  const ITexture* gpuTexture = material.getGpuTexture(slot);
  const uint64 textureId = gpuTexture ? ImGuiRenderer::getTextureId(*gpuTexture) : 0;
  constexpr float kThumbnailSize = 48.0f;
  if (textureId != 0) {
    ImGui::Image(static_cast<ImTextureID>(textureId), ImVec2(kThumbnailSize, kThumbnailSize));
  }
  else {
    ImGui::Button("##noTexture", ImVec2(kThumbnailSize, kThumbnailSize));
  }
  const bool bDroppedTexture = acceptTextureDrop(material, slot);
  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::TextUnformatted(getTextureLabel(slot));
  ImGui::TextDisabled("%s", texture ? texture->getName() : "None (drop a texture)");
  bool bClearedTexture = false;
  if (texture && ImGui::SmallButton("Clear")) {
    material.setTexture(slot, nullptr);
    bClearedTexture = true;
  }
  ImGui::EndGroup();
  const bool bDroppedOnText = acceptTextureDrop(material, slot);
  return bDroppedTexture || bDroppedOnText || bClearedTexture;
}

} // namespace chEngineSDK
