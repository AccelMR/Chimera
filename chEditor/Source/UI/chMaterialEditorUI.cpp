/************************************************************************/
/**
 * @file chMaterialEditorUI.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Edits the values and textures of a material asset.
 */
/************************************************************************/
#include "chMaterialEditorUI.h"

#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chAssetThumbnails.h"
#include "chITexture.h"
#include "chImGuiRenderer.h"
#include "chLogger.h"
#include "chMaterial.h"
#include "chMaterialAsset.h"
#include "chTextureAsset.h"

#include "imgui.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(MaterialEditorLog, All);

namespace {
// In MaterialAlphaMode order.
constexpr const ANSICHAR* kAlphaModeNames[] = {"Opaque", "Mask"};
static_assert(sizeof(kAlphaModeNames) / sizeof(kAlphaModeNames[0]) ==
              static_cast<SIZE_T>(MaterialAlphaMode::COUNT));

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
 * True when the texture of the slot changed.
 */
bool
renderMaterialTexture(Material& material, MaterialTexture slot)
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
} // namespace

/*
 */
void
MaterialEditorUI::render(const SPtr<MaterialAsset>& materialAsset)
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

  // A combo and a checkbox finish their edit in the frame they change, so they save then.
  int32 alphaMode = static_cast<int32>(material.getAlphaMode());
  if (ImGui::Combo("Alpha Mode", &alphaMode, kAlphaModeNames,
                   static_cast<int32>(MaterialAlphaMode::COUNT))) {
    material.setAlphaMode(static_cast<MaterialAlphaMode>(alphaMode));
    bSave = true;
  }
  if (material.getAlphaMode() == MaterialAlphaMode::Mask) {
    float alphaCutoff = material.getAlphaCutoff();
    if (ImGui::SliderFloat("Alpha Cutoff", &alphaCutoff, 0.0f, 1.0f)) {
      material.setAlphaCutoff(alphaCutoff);
    }
    bSave |= ImGui::IsItemDeactivatedAfterEdit();
  }

  bool bTwoSided = material.isTwoSided();
  if (ImGui::Checkbox("Two Sided", &bTwoSided)) {
    material.setTwoSided(bTwoSided);
    bSave = true;
  }

  for (uint32 i = 0; i < static_cast<uint32>(MaterialTexture::COUNT); ++i) {
    ImGui::PushID(static_cast<int32>(i));
    bSave |= renderMaterialTexture(material, static_cast<MaterialTexture>(i));
    ImGui::PopID();
  }

  if (!bSave) {
    return;
  }
  if (!AssetManager::instance().saveAsset(materialAsset)) {
    CH_LOG_ERROR(MaterialEditorLog, "Failed to save material {0}", materialAsset->getName());
  }
  AssetThumbnails::regenerate(*materialAsset);
}

} // namespace chEngineSDK
