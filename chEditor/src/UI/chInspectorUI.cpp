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
#include "chEditorScene.h"
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
#include "chPrefabAsset.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chShapeComponent.h"
#include "chSkyLightComponent.h"
#include "chStringUtils.h"
#include "chTextureAsset.h"

#include "chMath.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(InspectorLog, All);

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
 */
void
saveMaterial(const SPtr<MaterialAsset>& materialAsset)
{
  if (!AssetManager::instance().saveAsset(materialAsset)) {
    CH_LOG_ERROR(InspectorLog, "Failed to save material {0}", materialAsset->getName());
  }
}

// The colors most editors and the gizmo give each axis: X red, Y green, Z blue.
constexpr ImVec4 kAxisColors[3] = {ImVec4(0.75f, 0.18f, 0.18f, 1.0f),
                                   ImVec4(0.25f, 0.60f, 0.18f, 1.0f),
                                   ImVec4(0.18f, 0.38f, 0.82f, 1.0f)};
constexpr ImVec4 kAxisHoverColors[3] = {ImVec4(0.90f, 0.30f, 0.30f, 1.0f),
                                        ImVec4(0.38f, 0.75f, 0.30f, 1.0f),
                                        ImVec4(0.30f, 0.50f, 0.95f, 1.0f)};
constexpr const ANSICHAR* kAxisLabels[3] = {"X", "Y", "Z"};
constexpr const ANSICHAR* kAxisFieldIds[3] = {"##X", "##Y", "##Z"};

/*
 * One row with the label, then a colored axis button and a drag field per component.
 * Clicking an axis button puts that component back to resetValue. True when a value
 * changed.
 */
bool
renderAxisRow(const ANSICHAR* label,
              float (&values)[3],
              float speed,
              float resetValue,
              const ANSICHAR* format,
              const ANSICHAR* const (&tooltips)[3])
{
  ImGui::PushID(label);
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine(ImGui::GetFontSize() * 6.5f);

  const float buttonSize = ImGui::GetFrameHeight();
  const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
  const float fieldWidth = Math::max(
      (ImGui::GetContentRegionAvail().x - 3.0f * buttonSize - 2.0f * spacing) / 3.0f, 1.0f);

  bool bChanged = false;
  for (uint32 axis = 0; axis < 3; ++axis) {
    if (axis > 0) {
      ImGui::SameLine(0.0f, spacing);
    }
    ImGui::PushStyleColor(ImGuiCol_Button, kAxisColors[axis]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAxisHoverColors[axis]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAxisColors[axis]);
    if (ImGui::Button(kAxisLabels[axis], ImVec2(buttonSize, buttonSize))) {
      values[axis] = resetValue;
      bChanged = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SetItemTooltip("%s, click to reset", tooltips[axis]);

    // The field touches its button, so both read as one control.
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(fieldWidth);
    bChanged |= ImGui::DragFloat(kAxisFieldIds[axis], &values[axis], speed, 0.0f, 0.0f, format);
  }
  ImGui::PopID();
  return bChanged;
}
} // namespace

/*
 */
void
InspectorUI::renderInspectorUI()
{
  ImGui::Begin(ICON_FA_INFO_CIRCLE "  Inspector###Inspector", nullptr);
  const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
  if (!selected) {
    m_rotationOwner = nullptr;
    ImGui::End();
    return;
  }
  // Only the active object is edited here; the others move with it through the gizmo.
  const SIZE_T selectedCount = EditorSelection::getSelection().size();
  if (selectedCount > 1) {
    ImGui::TextDisabled(ICON_FA_LAYER_GROUP "  %u objects selected, showing the last one",
                        static_cast<uint32>(selectedCount));
  }

  // What an instance holds comes from its prefab and is not saved with the scene, so it is
  // shown but cannot be edited; only the instance's own name and transform can.
  GameObject* instance = selected->getOutermostPrefabInstance();
  ImGui::BeginDisabled(instance && instance != selected.get());
  renderName(*selected);
  ImGui::EndDisabled();
  if (instance) {
    renderPrefabInstance(*selected, *instance);
  }
  ImGui::Separator();
  ImGui::BeginDisabled(instance && instance != selected.get());
  renderTransform(*selected);
  ImGui::EndDisabled();

  ImGui::BeginDisabled(instance != nullptr);
  if (ModelComponent* modelComponent = selected->getComponent<ModelComponent>()) {
    renderModelComponent(*modelComponent);
  }
  if (ShapeComponent* shapeComponent = selected->getComponent<ShapeComponent>()) {
    renderShapeComponent(*shapeComponent);
  }
  renderLightComponent(*selected);
  if (SkyLightComponent* skyLight = selected->getComponent<SkyLightComponent>()) {
    renderSkyLightComponent(*skyLight);
  }
  ImGui::EndDisabled();
  ImGui::End();
}

/*
 * The name changes once the field is left or Enter is pressed, so a rename is one undo step
 * and the Scene Graph does not change on every key.
 */
void
InspectorUI::renderName(GameObject& gameObject)
{
  if (m_nameOwner != &gameObject || !m_bEditingName) {
    m_nameOwner = &gameObject;
    StringUtils::copyToBuffer(m_nameBuffer, gameObject.getName());
  }
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputText("##Name", m_nameBuffer, sizeof(m_nameBuffer));
  m_bEditingName = ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    const StringView name = StringUtils::trimView(StringView(m_nameBuffer));
    if (!name.empty() && name != gameObject.getName()) {
      EditorScene::recordChange(gameObject, "Rename");
      gameObject.setName(String(name));
    }
  }
}

/*
 */
void
InspectorUI::renderPrefabInstance(GameObject& selected, GameObject& instance)
{
  const SPtr<IAsset> asset = AssetManager::instance().getAsset(instance.getPrefab());
  const SPtr<PrefabAsset> prefab = asset ? asset->as<PrefabAsset>() : nullptr;
  const ANSICHAR* prefabName = prefab ? prefab->getName() : "(missing)";
  if (&selected == &instance) {
    ImGui::TextDisabled(ICON_FA_CUBES "  Instance of %s", prefabName);
  }
  else {
    ImGui::TextDisabled(ICON_FA_CUBES "  Part of %s, an instance of %s",
                        instance.getName().c_str(), prefabName);
  }

  if (prefab && ImGui::SmallButton("Edit Prefab")) {
    EditorScene::requestEditPrefab(prefab);
  }
  if (&selected == &instance) {
    ImGui::SameLine();
    if (ImGui::SmallButton("Unpack")) {
      // The content stays; it is saved with the scene from now on.
      EditorScene::recordChange(instance, "Unpack");
      instance.setPrefab(UUID::null());
    }
  }
}

/*
 */
void
InspectorUI::renderTransform(GameObject& gameObject)
{
  if (!ImGui::CollapsingHeader(ICON_FA_VECTOR_SQUARE "  Transform###Transform",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  Transform& transform = gameObject.getTransform();
  // Read again also when something else (the viewport gizmo) turned the object.
  if (m_rotationOwner != &gameObject || transform.getLocalRotation() != m_rotationShown) {
    m_rotationOwner = &gameObject;
    m_rotationShown = transform.getLocalRotation();
    const Rotator rotator = transform.getLocalRotation().toRotator();
    m_rotationDegrees[0] = rotator.roll.valueDegree();
    m_rotationDegrees[1] = rotator.pitch.valueDegree();
    m_rotationDegrees[2] = rotator.yaw.valueDegree();
  }

  // Every instance is placed where it is dropped, so the prefab keeps no position; its
  // rotation and scale are what new instances start with.
  const bool bPrefabRoot = &gameObject == EditorScene::getPrefabRoot().get();
  if (bPrefabRoot) {
    ImGui::TextWrapped("Prefab root: position is locked, each instance goes where it is "
                       "dropped. Rotation and scale are what new instances start with; "
                       "placed instances keep their own.");
  }

  const Vector3& position = transform.getLocalPosition();
  float positionValues[3] = {position.x, position.y, position.z};
  static constexpr const ANSICHAR* kPositionTooltips[3] = {"X (forward)", "Y (right)",
                                                           "Z (up)"};
  ImGui::BeginDisabled(bPrefabRoot);
  if (renderAxisRow(ICON_FA_ARROWS_ALT "  Position", positionValues, 0.01f, 0.0f, "%.3f",
                    kPositionTooltips)) {
    EditorScene::recordChange(gameObject, "Move");
    transform.setLocalPosition(
        Vector3(positionValues[0], positionValues[1], positionValues[2]));
  }
  ImGui::EndDisabled();

  static constexpr const ANSICHAR* kRotationTooltips[3] = {
      "Roll (around X)", "Pitch (around Y)", "Yaw (around Z)"};
  if (renderAxisRow(ICON_FA_SYNC_ALT "  Rotation", m_rotationDegrees, 0.5f, 0.0f, "%.1f",
                    kRotationTooltips)) {
    EditorScene::recordChange(gameObject, "Rotate");
    transform.setLocalRotation(
        Quaternion(Rotator(m_rotationDegrees[1], m_rotationDegrees[2], m_rotationDegrees[0])));
    m_rotationShown = transform.getLocalRotation();
  }

  const Vector3& scale = transform.getLocalScale();
  float scaleValues[3] = {scale.x, scale.y, scale.z};
  static constexpr const ANSICHAR* kScaleTooltips[3] = {"X", "Y", "Z"};
  if (renderAxisRow(ICON_FA_EXPAND_ARROWS_ALT "  Scale", scaleValues, 0.01f, 1.0f, "%.3f",
                    kScaleTooltips)) {
    EditorScene::recordChange(gameObject, "Scale");
    transform.setLocalScale(Vector3(scaleValues[0], scaleValues[1], scaleValues[2]));
  }
}

/*
 */
void
InspectorUI::renderModelComponent(ModelComponent& modelComponent)
{
  if (!ImGui::CollapsingHeader(ICON_FA_CUBE "  Model###Model",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  bool bVisible = modelComponent.isEnabled();
  if (ImGui::Checkbox("Visible", &bVisible)) {
    EditorScene::recordChange(*modelComponent.getOwner(), "Edit Visible");
    modelComponent.setEnabled(bVisible);
  }

  const SPtr<Model>& model = modelComponent.getModel();
  const uint32 nodeIndex = modelComponent.getNodeIndex();
  if (modelComponent.isWholeModel()) {
    ImGui::Text("Nodes: %u", model ? model->getNodeCount() : 0u);
  }
  else if (model && nodeIndex < model->getNodeCount()) {
    const ANSICHAR* nodeName = model->getAllNodes()[nodeIndex]->getName();
    if (modelComponent.getMeshIndex() == ModelComponent::ALL) {
      ImGui::Text("Part: node %s", nodeName);
    }
    else {
      ImGui::Text("Part: node %s, mesh %u", nodeName, modelComponent.getMeshIndex());
    }
  }

  const Vector<uint32>& usedSlots = modelComponent.getUsedMaterialSlots();
  if (usedSlots.empty() ||
      !ImGui::TreeNodeEx(ICON_FA_PALETTE "  Materials###Materials",
                         ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  for (const uint32 slot : usedSlots) {
    ImGui::PushID(static_cast<int32>(slot));
    renderMaterialSlot(modelComponent, slot);
    ImGui::PopID();
  }
  ImGui::TreePop();
}

/*
 * Only the sizes the shape's type reads are shown; the others keep their values, so
 * switching the type back finds them again. A drag builds a new mesh on every change,
 * which is cheap at these vertex counts.
 */
void
InspectorUI::renderShapeComponent(ShapeComponent& shapeComponent)
{
  if (!ImGui::CollapsingHeader(ICON_FA_SHAPES "  Shape###Shape",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  GameObject& gameObject = *shapeComponent.getOwner();

  bool bEnabled = shapeComponent.isEnabled();
  if (ImGui::Checkbox("Visible##Shape", &bEnabled)) {
    EditorScene::recordChange(gameObject, "Edit Visible");
    shapeComponent.setEnabled(bEnabled);
  }

  ShapeDesc shape = shapeComponent.getShape();
  bool bChanged = false;
  if (ImGui::BeginCombo("Type", MeshGenerator::getName(shape.type))) {
    for (uint32 i = 0; i < static_cast<uint32>(ShapeType::COUNT); ++i) {
      const ShapeType type = static_cast<ShapeType>(i);
      if (ImGui::Selectable(MeshGenerator::getName(type), type == shape.type)) {
        bChanged |= type != shape.type;
        shape.type = type;
      }
    }
    ImGui::EndCombo();
  }

  const float minSize = MeshGenerator::MIN_SIZE;
  constexpr float kMaxSize = 100000.0f;
  constexpr float kSizeSpeed = 0.01f;
  const auto dragCount = [&bChanged](const ANSICHAR* label, uint32& value, uint32 low,
                                     uint32 high) {
    int32 count = static_cast<int32>(value);
    if (ImGui::DragInt(label, &count, 0.2f, static_cast<int32>(low),
                       static_cast<int32>(high), "%d", ImGuiSliderFlags_AlwaysClamp)) {
      value = static_cast<uint32>(count);
      bChanged = true;
    }
  };
  switch (shape.type) {
    case ShapeType::Plane:
      bChanged |= ImGui::DragFloat2("Size", &shape.size.x, kSizeSpeed, minSize, kMaxSize);
      dragCount("Subdivisions", shape.subdivisions, 1, MeshGenerator::MAX_SUBDIVISIONS);
      break;
    case ShapeType::Box:
      bChanged |= ImGui::DragFloat3("Size", &shape.size.x, kSizeSpeed, minSize, kMaxSize);
      break;
    case ShapeType::Sphere:
    case ShapeType::Capsule:
    case ShapeType::Cylinder:
    case ShapeType::Cone:
      bChanged |= ImGui::DragFloat("Radius", &shape.radius, kSizeSpeed, minSize, kMaxSize);
      if (shape.type != ShapeType::Sphere) {
        bChanged |= ImGui::DragFloat("Height", &shape.height, kSizeSpeed, minSize, kMaxSize);
        if (shape.type == ShapeType::Capsule) {
          ImGui::SetItemTooltip("From tip to tip; at least twice the radius");
        }
      }
      dragCount("Segments", shape.segments, MeshGenerator::MIN_SEGMENTS,
                MeshGenerator::MAX_SEGMENTS);
      ImGui::SetItemTooltip("Sides around the Z axis");
      if (shape.type == ShapeType::Sphere || shape.type == ShapeType::Capsule) {
        dragCount("Rings", shape.rings, MeshGenerator::MIN_RINGS, MeshGenerator::MAX_RINGS);
        ImGui::SetItemTooltip("Rows from pole to pole");
      }
      break;
    case ShapeType::COUNT:
      break;
  }
  if (bChanged) {
    EditorScene::recordChange(gameObject, "Edit Shape");
    shapeComponent.setShape(shape);
  }

  // Dropping a material sets it on this shape only; editing it changes the shared asset.
  const SPtr<MaterialAsset>& material = shapeComponent.getMaterial();
  const ANSICHAR* materialName = "None (drop a material)";
  if (material) {
    materialName = material->getName();
  }
  else if (!shapeComponent.getMaterialId().isNull()) {
    materialName = "Missing material";
  }
  ImGui::Button(materialName, ImVec2(-FLT_MIN, 0.0f));
  if (ImGui::BeginDragDropTarget()) {
    if (SPtr<MaterialAsset> dropped = AssetDragDrop::acceptMaterial()) {
      EditorScene::recordChange(gameObject, "Set Material");
      shapeComponent.setMaterial(dropped);
    }
    ImGui::EndDragDropTarget();
  }
  if (!shapeComponent.getMaterialId().isNull() && ImGui::SmallButton("Clear##ShapeMaterial")) {
    EditorScene::recordChange(gameObject, "Clear Material");
    shapeComponent.setMaterial(nullptr);
  }
  if (material) {
    renderMaterial(material);
  }
}

/*
 */
void
InspectorUI::renderLightComponent(GameObject& gameObject)
{
  PointLightComponent* pointLight = gameObject.getComponent<PointLightComponent>();
  SpotLightComponent* spotLight = gameObject.getComponent<SpotLightComponent>();
  LightComponent* light = gameObject.getComponent<DirectionalLightComponent>();
  const ANSICHAR* title = ICON_FA_SUN "  Directional Light###DirectionalLight";
  if (pointLight) {
    light = pointLight;
    title = ICON_FA_LIGHTBULB "  Point Light###PointLight";
  }
  else if (spotLight) {
    light = spotLight;
    title = ICON_FA_BULLSEYE "  Spot Light###SpotLight";
  }
  if (!light || !ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  bool bEnabled = light->isEnabled();
  if (ImGui::Checkbox("Enabled", &bEnabled)) {
    EditorScene::recordChange(gameObject, "Edit Enabled");
    light->setEnabled(bEnabled);
  }

  const LinearColor& color = light->getColor();
  float colorValues[3] = {color.r, color.g, color.b};
  if (ImGui::ColorEdit3("Color", colorValues, ImGuiColorEditFlags_Float)) {
    EditorScene::recordChange(gameObject, "Edit Color");
    light->setColor(LinearColor(colorValues[0], colorValues[1], colorValues[2], 1.0f));
  }

  float intensity = light->getIntensity();
  if (ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 10000.0f)) {
    EditorScene::recordChange(gameObject, "Edit Intensity");
    light->setIntensity(intensity);
  }

  bool bCastShadows = light->getCastShadows();
  if (ImGui::Checkbox("Cast Shadows", &bCastShadows)) {
    EditorScene::recordChange(gameObject, "Edit Cast Shadows");
    light->setCastShadows(bCastShadows);
  }

  if (pointLight) {
    float range = pointLight->getRange();
    if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 100000.0f)) {
      EditorScene::recordChange(gameObject, "Edit Range");
      pointLight->setRange(range);
    }
  }

  if (spotLight) {
    float range = spotLight->getRange();
    if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 100000.0f)) {
      EditorScene::recordChange(gameObject, "Edit Range");
      spotLight->setRange(range);
    }
    float inner = spotLight->getInnerConeAngle().valueDegree();
    float outer = spotLight->getOuterConeAngle().valueDegree();
    const bool bInnerChanged = ImGui::SliderFloat("Inner Angle", &inner, 0.0f, 89.0f);
    const bool bOuterChanged = ImGui::SliderFloat("Outer Angle", &outer, 0.0f, 89.0f);
    if (bInnerChanged || bOuterChanged) {
      EditorScene::recordChange(gameObject, "Edit Cone");
      spotLight->setConeAngles(Degree(inner), Degree(outer));
    }
  }
}

/*
 * The image turns with the GameObject's rotation, so it has no angle of its own here.
 */
void
InspectorUI::renderSkyLightComponent(SkyLightComponent& skyLight)
{
  if (!ImGui::CollapsingHeader(ICON_FA_CLOUD_SUN "  Sky Light###SkyLight",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  GameObject& gameObject = *skyLight.getOwner();

  bool bEnabled = skyLight.isEnabled();
  if (ImGui::Checkbox("Enabled##Sky", &bEnabled)) {
    EditorScene::recordChange(gameObject, "Edit Enabled");
    skyLight.setEnabled(bEnabled);
  }

  // Equirectangular images are twice as wide as tall.
  constexpr ImVec2 kThumbnailSize(96.0f, 48.0f);
  const SPtr<TextureAsset>& texture = skyLight.getTexture();
  const ITexture* gpuTexture = texture ? texture->getTexture().get() : nullptr;
  const uint64 textureId = gpuTexture ? ImGuiRenderer::getTextureId(*gpuTexture) : 0;
  if (textureId != 0) {
    ImGui::Image(static_cast<ImTextureID>(textureId), kThumbnailSize);
  }
  else {
    ImGui::Button("##noSkyImage", kThumbnailSize);
  }
  const auto acceptDrop = [&]() {
    if (!ImGui::BeginDragDropTarget()) {
      return;
    }
    if (SPtr<TextureAsset> dropped = AssetDragDrop::acceptTexture()) {
      EditorScene::recordChange(gameObject, "Set Sky Image");
      skyLight.setTexture(dropped);
    }
    ImGui::EndDragDropTarget();
  };
  acceptDrop();
  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::TextUnformatted("Image");
  if (texture) {
    ImGui::TextDisabled("%s", texture->getName());
  }
  else if (!skyLight.getTextureId().isNull()) {
    ImGui::TextDisabled("Missing image");
  }
  else {
    ImGui::TextDisabled("None (drop an HDR image)");
  }
  if (!skyLight.getTextureId().isNull() && ImGui::SmallButton("Clear##Sky")) {
    EditorScene::recordChange(gameObject, "Clear Sky Image");
    skyLight.setTexture(nullptr);
  }
  ImGui::EndGroup();
  acceptDrop();

  float intensity = skyLight.getIntensity();
  if (ImGui::DragFloat("Intensity##Sky", &intensity, 0.01f, 0.0f, 1000.0f)) {
    EditorScene::recordChange(gameObject, "Edit Intensity");
    skyLight.setIntensity(intensity);
  }

  bool bShowSky = skyLight.getShowSky();
  if (ImGui::Checkbox("Show Sky", &bShowSky)) {
    EditorScene::recordChange(gameObject, "Edit Show Sky");
    skyLight.setShowSky(bShowSky);
  }
  ImGui::SetItemTooltip("Draws the image behind the scene");
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
      EditorScene::recordChange(*modelComponent.getOwner(), "Set Material");
      modelComponent.setMaterial(slot, dropped);
    }
    ImGui::EndDragDropTarget();
  }
  if (bOverride) {
    ImGui::TextDisabled("Only on this object");
    ImGui::SameLine();
    if (ImGui::SmallButton("Use model's")) {
      EditorScene::recordChange(*modelComponent.getOwner(), "Clear Material");
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
