/************************************************************************/
/**
 * @file chModelPreview.cpp
 * @author AccelMR
 * @date 2026/10/24
 * @brief A model in a scene of its own, drawn into a texture an editor window shows.
 */
/************************************************************************/
#include "chModelPreview.h"

#include "chBox.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chLightComponent.h"
#include "chMath.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneRenderer.h"
#include "chStringUtils.h"
#include "chUIHelpers.h"
#include "chViewportGrid.h"
#include "chViewportTools.h"

#include "imgui.h"

namespace chEngineSDK {
namespace {
ConsoleVariable<bool> g_cvarPreviewSkeleton("Editor.PreviewSkeleton",
                                            true,
                                            "Draws the bones of the previewed model.");
ConsoleVariable<bool> g_cvarPreviewGrid("Editor.PreviewGrid",
                                        true,
                                        "Draws the ground grid in the model previews.");
ConsoleVariable<float> g_cvarPreviewOrbitSensitivity(
    "Editor.PreviewOrbitSensitivity",
    0.3f,
    "Degrees the model preview camera turns per pixel dragged.");
ConsoleVariable<float> g_cvarPreviewZoomStep(
    "Editor.PreviewZoomStep",
    1.15f,
    "How many times closer each wheel step takes the model preview camera.");
ConsoleVariable<float> g_cvarPreviewLightIntensity(
    "Editor.PreviewLightIntensity",
    3.0f,
    "Intensity of the key light of the model previews; the fill light gets a third.");

// The tonemap writes encoded sRGB, the same as into the swap chain, so ImGui shows it as is.
constexpr Format kTargetFormat = Format::R8G8B8A8_UNORM;
constexpr uint32 kMinTargetSize = 16;
// Framed from the front left and a little above, so a character shows its face and side.
constexpr float kFramePitch = -15.0f;
constexpr float kFrameYaw = -150.0f;
constexpr float kFillLightFraction = 1.0f / 3.0f;
// Near and far planes follow the model's size, so small and huge models both keep depth
// precision.
constexpr float kNearPlaneScale = 0.01f;
constexpr float kFarPlaneScale = 200.0f;
constexpr float kMinRadius = 0.01f;

/*
 */
void
saveSetting(StringView key, float value)
{
  ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
  EngineConfig::setUserValue("Editor", "Editor", key, StringUtils::toChars(buffer, value));
}

/*
 */
void
saveSetting(StringView key, bool bValue)
{
  EngineConfig::setUserValue("Editor", "Editor", key, bValue ? "true" : "false");
}

/*
 */
void
saveSettings()
{
  saveSetting("PreviewSkeleton", g_cvarPreviewSkeleton.get());
  saveSetting("PreviewGrid", g_cvarPreviewGrid.get());
  saveSetting("PreviewOrbitSensitivity", g_cvarPreviewOrbitSensitivity.get());
  saveSetting("PreviewZoomStep", g_cvarPreviewZoomStep.get());
  saveSetting("PreviewLightIntensity", g_cvarPreviewLightIntensity.get());
}

/*
 */
SPtr<GameObject>
addLight(Scene& scene, const ANSICHAR* name, const Rotator& rotation)
{
  const SPtr<GameObject> light = scene.createGameObject(name);
  light->getTransform().setLocalRotation(Quaternion(rotation));
  // Shadows would give the preview a renderer's worth of shadow maps for little: there is
  // no ground to fall on.
  light->addComponent<DirectionalLightComponent>().setCastShadows(false);
  return light;
}
} // namespace

/*
 */
ModelPreview::ModelPreview(StringView name) : m_name(name) {}

/*
 */
ModelPreview::~ModelPreview()
{
  shutDown();
}

/*
 */
void
ModelPreview::createScene()
{
  if (m_scene) {
    return;
  }
  m_scene = chMakeUnique<Scene>(m_name, UUID::createRandom());
  m_keyLight = addLight(*m_scene, "Key Light", Rotator(-40.0f, -120.0f, 0.0f));
  m_fillLight = addLight(*m_scene, "Fill Light", Rotator(-20.0f, 160.0f, 0.0f));
}

/*
 */
void
ModelPreview::setModel(const SPtr<ModelAsset>& model)
{
  createScene();
  if (m_modelObject) {
    m_scene->destroyGameObject(*m_modelObject);
    m_modelObject.reset();
    m_component = nullptr;
  }
  m_model = model;
  if (!m_model || !m_model->getModel()) {
    return;
  }
  m_modelObject = m_scene->createGameObject(m_model->getName());
  m_component = &m_modelObject->addComponent<ModelComponent>(m_model);
  m_bFrameCamera = true;
}

/*
 */
void
ModelPreview::updateLights()
{
  const float intensity = Math::max(g_cvarPreviewLightIntensity.get(), 0.0f);
  if (DirectionalLightComponent* key = m_keyLight->getComponent<DirectionalLightComponent>()) {
    key->setIntensity(intensity);
  }
  if (DirectionalLightComponent* fill =
          m_fillLight->getComponent<DirectionalLightComponent>()) {
    fill->setIntensity(intensity * kFillLightFraction);
  }
}

/*
 * The field of view is horizontal, so on a wide view the vertical one decides how far the
 * camera must be for the sphere around the bounds to fit.
 */
void
ModelPreview::frameCamera()
{
  AABox bounds;
  if (!m_modelObject || !m_modelObject->getHierarchyWorldBounds(bounds)) {
    return;
  }
  const Radian halfHorizontal = m_camera.getFieldOfView() * 0.5f;
  const Radian halfVertical =
      Math::atan(Math::tan(halfHorizontal) / m_camera.getAspectRatio());
  const Radian halfAngle = halfVertical < halfHorizontal ? halfVertical : halfHorizontal;
  const float radius = Math::max(bounds.getExtent().magnitude(), kMinRadius);
  const float distance = radius / Math::sin(halfAngle);

  const Vector3 forward =
      Quaternion(Rotator(kFramePitch, kFrameYaw, 0.0f)).rotateVector(Vector3::FORWARD);
  const Vector3 center = bounds.getCenter();
  m_camera.setClipPlanes(radius * kNearPlaneScale, (distance + radius) * kFarPlaneScale);
  m_camera.setPosition(center - forward * distance);
  m_camera.lookAt(center);
}

/*
 * Drag to orbit, middle drag to pan and the wheel to zoom, all around the look at point.
 */
void
ModelPreview::handleCameraInput()
{
  const ImGuiIO& io = ImGui::GetIO();
  if (ImGui::IsItemActive()) {
    const ImVec2 delta = io.MouseDelta;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      // The scene follows the mouse.
      const float sensitivity = g_cvarPreviewOrbitSensitivity.get();
      m_camera.rotate(-delta.y * sensitivity, delta.x * sensitivity);
    }
    else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
      const float distance = (m_camera.getLookAt() - m_camera.getPosition()).magnitude();
      const float unitsPerPixel = 2.0f * distance * Math::tan(m_camera.getFieldOfView() * 0.5f) /
                                  Math::max(static_cast<float>(m_targetWidth), 1.0f);
      m_camera.pan(-delta.x * unitsPerPixel, delta.y * unitsPerPixel);
    }
  }

  if (ImGui::IsItemHovered()) {
    if (io.MouseWheel != 0.0f) {
      const float step = Math::max(g_cvarPreviewZoomStep.get(), 1.001f);
      const Vector3 lookAt = m_camera.getLookAt();
      const Vector3 offset = m_camera.getPosition() - lookAt;
      const float scale = Math::pow(step, -io.MouseWheel);
      m_camera.setPosition(lookAt + offset * scale);
      m_camera.lookAt(lookAt);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
      m_bFrameCamera = true;
    }
  }
}

/*
 * The texture follows the size of the view, in pixels.
 */
void
ModelPreview::updateTarget(const ImVec2& size)
{
  const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
  const uint32 width = Math::max(static_cast<uint32>(size.x * scale.x), kMinTargetSize);
  const uint32 height = Math::max(static_cast<uint32>(size.y * scale.y), kMinTargetSize);
  if (m_target && width == m_targetWidth && height == m_targetHeight) {
    return;
  }

  // The old texture is freed by the deferred deletion once the GPU is done with it, and
  // the view points at it, so the view goes first.
  m_targetView.reset();
  m_target = IGraphicsAPI::instance().createTexture(
      {.format = kTargetFormat,
       .width = width,
       .height = height,
       .usage = TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled});
  m_targetView = m_target->createView();
  m_targetWidth = width;
  m_targetHeight = height;
  m_camera.setViewportSize(static_cast<float>(width), static_cast<float>(height));
}

/*
 */
void
ModelPreview::renderView(const ImVec2& size, const ANSICHAR* emptyText)
{
  const ImVec2 viewSize(Math::max(size.x, 1.0f), Math::max(size.y, 1.0f));
  ImGui::InvisibleButton("##View", viewSize,
                         ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                             ImGuiButtonFlags_MouseButtonMiddle);
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  if (!m_component) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const ImVec2 textSize = ImGui::CalcTextSize(emptyText);
    drawList->AddText(ImVec2(min.x + (viewSize.x - textSize.x) * 0.5f,
                             min.y + (viewSize.y - textSize.y) * 0.5f),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled), emptyText);
    return;
  }

  handleCameraInput();
  updateTarget(viewSize);
  const uint64 textureId = ImGuiRenderer::getTextureId(*m_target);
  if (textureId != 0) {
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(textureId), min, max);
    m_bDrawn = true;
  }
}

/*
 */
void
ModelPreview::update(float deltaTime)
{
  const bool bDrawn = m_bDrawn;
  m_bDrawn = false;
  if (!bDrawn || !m_scene) {
    return;
  }

  if (!m_renderer) {
    // Lit always, whatever the main viewport shows; the main renderer times the GPU.
    m_renderer = chMakeUnique<SceneRenderer>(
        SceneRendererDesc{.bProfileGpu = false, .viewMode = ViewMode::Lit});
    m_grid = chMakeUnique<ViewportGrid>();
  }

  updateLights();
  m_scene->update(deltaTime);
  m_scene->updateTransforms();
  if (m_bFrameCamera) {
    m_bFrameCamera = false;
    frameCamera();
  }

  m_lines.clear();
  if (g_cvarPreviewSkeleton.get() && m_component) {
    ViewportTools::addSkeleton(*m_component, m_lines);
  }
  m_renderer->extract(*m_scene, m_camera, chUIHelpers::UIHelpers::rendererColor, &m_lines);
  m_bExtracted = true;
}

/*
 */
void
ModelPreview::recordFrame(ICommandList& commandList)
{
  if (!m_bExtracted) {
    return;
  }
  m_bExtracted = false;

  RenderGraph& graph = m_renderer->beginFrame();
  // Drawn again every frame, so nothing of the last one is kept.
  const RGTextureHandle output = graph.importTexture(m_name, *m_target, *m_targetView,
                                                     ResourceState::Undefined,
                                                     ResourceState::ShaderRead);
  const RGTextureHandle depth = m_renderer->addScenePasses(output);
  if (g_cvarPreviewGrid.get()) {
    m_grid->addPass(graph, m_renderer->getFrame().getCamera(), output, depth);
  }
  m_renderer->execute(commandList);
}

/*
 */
void
ModelPreview::shutDown()
{
  m_renderer.reset();
  m_grid.reset();
  m_targetView.reset();
  m_target.reset();
  m_component = nullptr;
  m_modelObject.reset();
  m_keyLight.reset();
  m_fillLight.reset();
  m_scene.reset();
  m_model.reset();
  m_bDrawn = false;
  m_bExtracted = false;
}

/*
 */
void
ModelPreview::renderSettings()
{
  bool bSave = false;
  bool bSkeleton = g_cvarPreviewSkeleton.get();
  if (ImGui::Checkbox("Show Skeleton", &bSkeleton)) {
    g_cvarPreviewSkeleton.set(bSkeleton, ConsoleVariableSource::Console);
    bSave = true;
  }
  bool bGrid = g_cvarPreviewGrid.get();
  if (ImGui::Checkbox("Show Grid", &bGrid)) {
    g_cvarPreviewGrid.set(bGrid, ConsoleVariableSource::Console);
    bSave = true;
  }

  const auto slider = [&bSave](const ANSICHAR* label, ConsoleVariable<float>& variable,
                               float minValue, float maxValue, const ANSICHAR* format) {
    float value = variable.get();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderFloat(label, &value, minValue, maxValue, format)) {
      variable.set(value, ConsoleVariableSource::Console);
    }
    ImGui::SetItemTooltip("%s", variable.getHelp().c_str());
    bSave |= ImGui::IsItemDeactivatedAfterEdit();
  };
  slider("Light Intensity", g_cvarPreviewLightIntensity, 0.0f, 20.0f, "%.2f");
  slider("Orbit Sensitivity", g_cvarPreviewOrbitSensitivity, 0.05f, 2.0f, "%.2f deg/px");
  slider("Zoom Step", g_cvarPreviewZoomStep, 1.01f, 2.0f, "%.2fx");

  if (ImGui::Button("Reset to Defaults")) {
    g_cvarPreviewSkeleton.set(g_cvarPreviewSkeleton.getDefault(),
                              ConsoleVariableSource::Console);
    g_cvarPreviewGrid.set(g_cvarPreviewGrid.getDefault(), ConsoleVariableSource::Console);
    g_cvarPreviewLightIntensity.set(g_cvarPreviewLightIntensity.getDefault(),
                                    ConsoleVariableSource::Console);
    g_cvarPreviewOrbitSensitivity.set(g_cvarPreviewOrbitSensitivity.getDefault(),
                                      ConsoleVariableSource::Console);
    g_cvarPreviewZoomStep.set(g_cvarPreviewZoomStep.getDefault(),
                              ConsoleVariableSource::Console);
    bSave = true;
  }
  if (bSave) {
    saveSettings();
  }
}

} // namespace chEngineSDK
