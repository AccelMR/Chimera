/************************************************************************/
/**
 * @file chAnimationPreviewUI.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Window that plays a model's animations away from the open scene.
 */
/************************************************************************/
#include "chAnimationPreviewUI.h"

#include "chAlgorithm.h"
#include "chAnimationClip.h"
#include "chAnimationClipAsset.h"
#include "chAnimationTimelineUI.h"
#include "chAnimatorComponent.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chBox.h"
#include "chCamera.h"
#include "chConsoleVariable.h"
#include "chDebugDraw.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chLightComponent.h"
#include "chLogger.h"
#include "chMath.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneRenderer.h"
#include "chSkeletonAsset.h"
#include "chStringUtils.h"
#include "chUIHelpers.h"
#include "chViewportGrid.h"
#include "chViewportTools.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(AnimationPreviewLog, All);

namespace {
ConsoleVariable<bool> g_cvarShowAnimationPreview("Editor.ShowAnimationPreview",
                                                 false,
                                                 "Shows the Animation Preview window.",
                                                 "ShowAnimationPreview");
ConsoleVariable<bool> g_cvarPreviewSkeleton("Editor.PreviewSkeleton",
                                            true,
                                            "Draws the bones of the previewed model.");
ConsoleVariable<bool> g_cvarPreviewGrid("Editor.PreviewGrid",
                                        true,
                                        "Draws the ground grid in the Animation Preview.");
ConsoleVariable<float> g_cvarPreviewOrbitSensitivity(
    "Editor.PreviewOrbitSensitivity",
    0.3f,
    "Degrees the Animation Preview camera turns per pixel dragged.");
ConsoleVariable<float> g_cvarPreviewZoomStep(
    "Editor.PreviewZoomStep",
    1.15f,
    "How many times closer each wheel step takes the Animation Preview camera.");
ConsoleVariable<float> g_cvarPreviewLightIntensity(
    "Editor.PreviewLightIntensity",
    3.0f,
    "Intensity of the key light of the Animation Preview; the fill light gets a third.");

// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 640.0f;
constexpr float kWindowHeight = 600.0f;
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

struct PreviewState
{
  // What the window shows; either may be null.
  SPtr<ModelAsset> model;
  SPtr<AnimationClipAsset> clip;
  // The models and clips the combos offer: made for this skeleton, or with a null one, in
  // this folder.
  UUID skeletonId = UUID::null();
  String folder;
  Vector<SPtr<IAsset>> models;
  Vector<SPtr<IAsset>> clips;

  UniquePtr<Scene> scene;
  SPtr<GameObject> keyLight;
  SPtr<GameObject> fillLight;
  SPtr<GameObject> modelObject;
  ModelComponent* component = nullptr;
  AnimatorComponent* animator = nullptr;
  Camera camera;
  bool bFrameCamera = false;

  UniquePtr<SceneRenderer> renderer;
  UniquePtr<ViewportGrid> grid;
  SPtr<ITexture> target;
  SPtr<ITextureView> targetView;
  uint32 targetWidth = 0;
  uint32 targetHeight = 0;
  DebugDraw lines;

  bool bFocus = false;
  // The window showed the texture this frame, so the update plays the preview and the
  // render draws it; otherwise neither runs.
  bool bDrawn = false;
  bool bExtracted = false;
};

PreviewState&
getState()
{
  static PreviewState state;
  return state;
}

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
void
sortByName(Vector<SPtr<IAsset>>& assets)
{
  Algorithm::sort(assets, [](const SPtr<IAsset>& a, const SPtr<IAsset>& b) {
    return StringView(a->getName()) < StringView(b->getName());
  });
}

/*
 */
bool
ensureLoaded(const SPtr<IAsset>& asset)
{
  if (asset->isLoaded() || AssetManager::instance().syncLoadAsset(asset)) {
    return true;
  }
  CH_LOG_ERROR(AnimationPreviewLog, "Failed to load asset: {0}", asset->getName());
  return false;
}

/*
 * Models know their skeleton from the references of their file, so finding them loads
 * nothing.
 */
void
collectChoices(PreviewState& state)
{
  state.models.clear();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<ModelAsset>()) {
      continue;
    }
    const bool bFits = state.skeletonId.isNull()
                           ? StringUtils::equals(asset->getAssetPath(), state.folder)
                           : Algorithm::contains(asset->getReferencedAssets(), state.skeletonId);
    if (bFits) {
      state.models.push_back(asset);
    }
  }
  sortByName(state.models);
  AnimationPreviewUI::findClips(state.skeletonId, state.folder, state.clips);
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

/*
 */
void
updateLights(PreviewState& state)
{
  const float intensity = Math::max(g_cvarPreviewLightIntensity.get(), 0.0f);
  if (DirectionalLightComponent* key =
          state.keyLight->getComponent<DirectionalLightComponent>()) {
    key->setIntensity(intensity);
  }
  if (DirectionalLightComponent* fill =
          state.fillLight->getComponent<DirectionalLightComponent>()) {
    fill->setIntensity(intensity * kFillLightFraction);
  }
}

/*
 * Puts the model in the preview scene again, playing the clip from its start.
 */
void
rebuildModel(PreviewState& state)
{
  if (!state.scene) {
    state.scene = chMakeUnique<Scene>("AnimationPreview", UUID::createRandom());
    state.keyLight = addLight(*state.scene, "Key Light", Rotator(-40.0f, -120.0f, 0.0f));
    state.fillLight = addLight(*state.scene, "Fill Light", Rotator(-20.0f, 160.0f, 0.0f));
  }
  if (state.modelObject) {
    state.scene->destroyGameObject(*state.modelObject);
    state.modelObject.reset();
    state.component = nullptr;
    state.animator = nullptr;
  }
  if (!state.model || !state.model->getModel()) {
    return;
  }

  state.modelObject = state.scene->createGameObject(state.model->getName());
  state.component = &state.modelObject->addComponent<ModelComponent>(state.model);
  state.animator = &state.modelObject->addComponent<AnimatorComponent>(state.clip);
  state.bFrameCamera = true;
}

/*
 */
void
setModel(PreviewState& state, const SPtr<IAsset>& asset)
{
  state.model = asset && ensureLoaded(asset) ? asset->as<ModelAsset>() : nullptr;
  rebuildModel(state);
}

/*
 */
void
setClip(PreviewState& state, const SPtr<IAsset>& asset)
{
  state.clip = asset && ensureLoaded(asset) ? asset->as<AnimationClipAsset>() : nullptr;
  if (state.animator) {
    state.animator->play(state.clip);
    state.animator->setPlaying(true);
  }
}

/*
 * The field of view is horizontal, so on a wide window the vertical one decides how far
 * the camera must be for the sphere around the bounds to fit.
 */
void
frameCamera(PreviewState& state)
{
  AABox bounds;
  if (!state.modelObject || !state.modelObject->getHierarchyWorldBounds(bounds)) {
    return;
  }
  Camera& camera = state.camera;
  const Radian halfHorizontal = camera.getFieldOfView() * 0.5f;
  const Radian halfVertical =
      Math::atan(Math::tan(halfHorizontal) / camera.getAspectRatio());
  const Radian halfAngle = halfVertical < halfHorizontal ? halfVertical : halfHorizontal;
  const float radius = Math::max(bounds.getExtent().magnitude(), kMinRadius);
  const float distance = radius / Math::sin(halfAngle);

  const Vector3 forward =
      Quaternion(Rotator(kFramePitch, kFrameYaw, 0.0f)).rotateVector(Vector3::FORWARD);
  const Vector3 center = bounds.getCenter();
  camera.setClipPlanes(radius * kNearPlaneScale, (distance + radius) * kFarPlaneScale);
  camera.setPosition(center - forward * distance);
  camera.lookAt(center);
}

/*
 * Drag to orbit, middle drag to pan and the wheel to zoom, all around the look at point.
 */
void
handleCameraInput(PreviewState& state)
{
  const ImGuiIO& io = ImGui::GetIO();
  Camera& camera = state.camera;
  if (ImGui::IsItemActive()) {
    const ImVec2 delta = io.MouseDelta;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      // The scene follows the mouse.
      const float sensitivity = g_cvarPreviewOrbitSensitivity.get();
      camera.rotate(-delta.y * sensitivity, delta.x * sensitivity);
    }
    else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
      const float distance = (camera.getLookAt() - camera.getPosition()).magnitude();
      const float unitsPerPixel = 2.0f * distance * Math::tan(camera.getFieldOfView() * 0.5f) /
                                  Math::max(static_cast<float>(state.targetWidth), 1.0f);
      camera.pan(-delta.x * unitsPerPixel, delta.y * unitsPerPixel);
    }
  }

  if (ImGui::IsItemHovered()) {
    if (io.MouseWheel != 0.0f) {
      const float step = Math::max(g_cvarPreviewZoomStep.get(), 1.001f);
      const Vector3 lookAt = camera.getLookAt();
      const Vector3 offset = camera.getPosition() - lookAt;
      const float scale = Math::pow(step, -io.MouseWheel);
      camera.setPosition(lookAt + offset * scale);
      camera.lookAt(lookAt);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
      state.bFrameCamera = true;
    }
  }
}

/*
 * The texture follows the size of the window, in pixels.
 */
void
updateTarget(PreviewState& state, const ImVec2& size)
{
  const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
  const uint32 width = Math::max(static_cast<uint32>(size.x * scale.x), kMinTargetSize);
  const uint32 height = Math::max(static_cast<uint32>(size.y * scale.y), kMinTargetSize);
  if (state.target && width == state.targetWidth && height == state.targetHeight) {
    return;
  }

  // The old texture is freed by the deferred deletion once the GPU is done with it, and
  // the view points at it, so the view goes first.
  state.targetView.reset();
  state.target = IGraphicsAPI::instance().createTexture(
      {.format = kTargetFormat,
       .width = width,
       .height = height,
       .usage = TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled});
  state.targetView = state.target->createView();
  state.targetWidth = width;
  state.targetHeight = height;
  state.camera.setViewportSize(static_cast<float>(width), static_cast<float>(height));
}

/*
 */
void
acceptDrop(PreviewState& state)
{
  if (!ImGui::BeginDragDropTarget()) {
    return;
  }
  if (const SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
    setModel(state, model);
  }
  if (const SPtr<AnimationClipAsset> clip = AssetDragDrop::acceptAnimation()) {
    setClip(state, clip);
  }
  ImGui::EndDragDropTarget();
}

/*
 */
void
renderAssetCombo(const ANSICHAR* label,
                 const IAsset* current,
                 const ANSICHAR* noneLabel,
                 const Vector<SPtr<IAsset>>& choices,
                 SPtr<IAsset>& outPicked,
                 bool& outbPicked)
{
  if (!ImGui::BeginCombo(label, current ? current->getName() : noneLabel)) {
    return;
  }
  if (ImGui::Selectable(noneLabel, current == nullptr)) {
    outPicked.reset();
    outbPicked = true;
  }
  for (const SPtr<IAsset>& choice : choices) {
    ImGui::PushID(choice.get());
    if (ImGui::Selectable(choice->getName(), choice.get() == current)) {
      outPicked = choice;
      outbPicked = true;
    }
    ImGui::PopID();
  }
  ImGui::EndCombo();
}

/*
 */
void
renderSettings()
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

/*
 */
void
renderToolbar(PreviewState& state)
{
  const float comboWidth = Math::max(ImGui::GetContentRegionAvail().x * 0.35f, 80.0f);
  SPtr<IAsset> picked;
  bool bPicked = false;
  ImGui::SetNextItemWidth(comboWidth);
  renderAssetCombo(ICON_FA_CUBE "##Model", state.model.get(), "No model", state.models,
                   picked, bPicked);
  ImGui::SetItemTooltip("Model the clip plays on; drop any model on the view too");
  if (bPicked) {
    setModel(state, picked);
  }

  ImGui::SameLine();
  bPicked = false;
  ImGui::SetNextItemWidth(comboWidth);
  renderAssetCombo(ICON_FA_FILM "##Clip", state.clip.get(), "Rest pose", state.clips, picked,
                   bPicked);
  ImGui::SetItemTooltip("Clip to play; drop any clip on the view too");
  if (bPicked) {
    setClip(state, picked);
  }

  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_EXPAND "##Frame")) {
    state.bFrameCamera = true;
  }
  ImGui::SetItemTooltip("Frame the model (F over the view)");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_COG "##Settings")) {
    ImGui::OpenPopup("PreviewSettings");
  }
  ImGui::SetItemTooltip("Preview settings");
  if (ImGui::BeginPopup("PreviewSettings")) {
    renderSettings();
    ImGui::EndPopup();
  }
}

/*
 */
void
renderView(PreviewState& state)
{
  // The timeline stays under the view, so the view takes what is left above it.
  const ImGuiStyle& style = ImGui::GetStyle();
  const bool bTimeline = state.animator && state.animator->getClip();
  const float timelineHeight =
      bTimeline ? ImGui::GetFrameHeightWithSpacing() * 3.0f +
                      ImGui::GetTextLineHeightWithSpacing() + style.ItemSpacing.y
                : 0.0f;
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 size(Math::max(available.x, 1.0f),
                    Math::max(available.y - timelineHeight, 1.0f));

  ImGui::InvisibleButton("##View", size,
                         ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                             ImGuiButtonFlags_MouseButtonMiddle);
  acceptDrop(state);
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  if (!state.component) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const ANSICHAR* text = "Drop a model or a clip here.";
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    drawList->AddText(ImVec2(min.x + (size.x - textSize.x) * 0.5f,
                             min.y + (size.y - textSize.y) * 0.5f),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
    return;
  }

  handleCameraInput(state);
  updateTarget(state, size);
  const uint64 textureId = ImGuiRenderer::getTextureId(*state.target);
  if (textureId != 0) {
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(textureId), min, max);
    state.bDrawn = true;
  }

  if (bTimeline) {
    AnimationTimelineUI::render(*state.animator, false);
    if (!state.animator->hasBoundTracks()) {
      ImGui::TextDisabled("No node of this model has the name of a track of the clip.");
    }
  }
}
} // namespace

/*
 */
void
AnimationPreviewUI::render()
{
  if (!g_cvarShowAnimationPreview.get()) {
    return;
  }

  PreviewState& state = getState();
  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  if (state.bFocus) {
    ImGui::SetNextWindowFocus();
    state.bFocus = false;
  }
  bool bVisible = true;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
  const bool bOpen =
      ImGui::Begin(ICON_FA_RUNNING "  Animation Preview###AnimationPreview", &bVisible);
  ImGui::PopStyleVar();
  if (bOpen) {
    renderToolbar(state);
    renderView(state);
  }
  ImGui::End();

  if (!bVisible) {
    setWindowVisible(false);
  }
}

/*
 */
void
AnimationPreviewUI::update(float deltaTime)
{
  PreviewState& state = getState();
  const bool bDrawn = state.bDrawn;
  state.bDrawn = false;
  if (!bDrawn || !state.scene) {
    return;
  }

  if (!state.renderer) {
    // Lit always, whatever the main viewport shows; the main renderer times the GPU.
    state.renderer = chMakeUnique<SceneRenderer>(
        SceneRendererDesc{.bProfileGpu = false, .viewMode = ViewMode::Lit});
    state.grid = chMakeUnique<ViewportGrid>();
  }

  updateLights(state);
  state.scene->update(deltaTime);
  state.scene->updateTransforms();
  if (state.bFrameCamera) {
    state.bFrameCamera = false;
    frameCamera(state);
  }

  state.lines.clear();
  if (g_cvarPreviewSkeleton.get() && state.component) {
    ViewportTools::addSkeleton(*state.component, state.lines);
  }
  state.renderer->extract(*state.scene, state.camera, chUIHelpers::UIHelpers::rendererColor,
                          &state.lines);
  state.bExtracted = true;
}

/*
 */
void
AnimationPreviewUI::recordFrame(ICommandList& commandList)
{
  PreviewState& state = getState();
  if (!state.bExtracted) {
    return;
  }
  state.bExtracted = false;

  RenderGraph& graph = state.renderer->beginFrame();
  // Drawn again every frame, so nothing of the last one is kept.
  const RGTextureHandle output =
      graph.importTexture("AnimationPreview", *state.target, *state.targetView,
                          ResourceState::Undefined, ResourceState::ShaderRead);
  const RGTextureHandle depth = state.renderer->addScenePasses(output);
  if (g_cvarPreviewGrid.get()) {
    state.grid->addPass(graph, state.renderer->getFrame().getCamera(), output, depth);
  }
  state.renderer->execute(commandList);
}

/*
 */
bool
AnimationPreviewUI::canOpen(const IAsset& asset)
{
  return asset.isTypeOf<ModelAsset>() || asset.isTypeOf<AnimationClipAsset>() ||
         asset.isTypeOf<SkeletonAsset>();
}

/*
 */
void
AnimationPreviewUI::open(const SPtr<IAsset>& asset)
{
  if (!asset || !canOpen(*asset) || !ensureLoaded(asset)) {
    return;
  }

  PreviewState& state = getState();
  state.folder = asset->getAssetPath();
  if (const SPtr<ModelAsset> model = asset->as<ModelAsset>()) {
    state.skeletonId = model->getModel() ? model->getModel()->getSkeletonId() : UUID::null();
    state.clip.reset();
    collectChoices(state);
    setModel(state, model);
  }
  else if (const SPtr<AnimationClipAsset> clip = asset->as<AnimationClipAsset>()) {
    state.skeletonId = clip->getClip() ? clip->getClip()->getSkeletonId() : UUID::null();
    state.clip = clip;
    collectChoices(state);
    // The model shown stays when the clip is made for it too.
    if (!state.model ||
        !Algorithm::containsIf(state.models, [&state](const SPtr<IAsset>& choice) {
          return choice.get() == state.model.get();
        })) {
      setModel(state, state.models.empty() ? nullptr : state.models.front());
    }
    else {
      setClip(state, clip);
    }
  }
  else {
    state.skeletonId = asset->getUUID();
    state.clip.reset();
    collectChoices(state);
    setModel(state, state.models.empty() ? nullptr : state.models.front());
  }

  state.bFocus = true;
  if (!isWindowVisible()) {
    setWindowVisible(true);
  }
}

/*
 */
void
AnimationPreviewUI::close(const IAsset& asset)
{
  PreviewState& state = getState();
  const auto isAsset = [&asset](const SPtr<IAsset>& choice) {
    return choice.get() == &asset;
  };
  Algorithm::removeAllIf(state.models, isAsset);
  Algorithm::removeAllIf(state.clips, isAsset);
  if (state.clip.get() == &asset) {
    setClip(state, nullptr);
  }
  if (state.model.get() == &asset) {
    setModel(state, nullptr);
  }
}

/*
 */
void
AnimationPreviewUI::findClips(const UUID& skeletonId,
                              StringView folder,
                              Vector<SPtr<IAsset>>& outClips)
{
  outClips.clear();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<AnimationClipAsset>()) {
      continue;
    }
    if (skeletonId.isNull()) {
      if (StringUtils::equals(asset->getAssetPath(), folder)) {
        outClips.push_back(asset);
      }
      continue;
    }
    if (!asset->isLoaded() && !AssetManager::instance().syncLoadAsset(asset)) {
      continue;
    }
    const SPtr<AnimationClipAsset> clip = asset->as<AnimationClipAsset>();
    if (clip && clip->getClip() && clip->getClip()->getSkeletonId() == skeletonId) {
      outClips.push_back(asset);
    }
  }
  sortByName(outClips);
}

/*
 */
bool
AnimationPreviewUI::isWindowVisible()
{
  return g_cvarShowAnimationPreview.get();
}

/*
 */
void
AnimationPreviewUI::setWindowVisible(bool bVisible)
{
  g_cvarShowAnimationPreview.set(bVisible, ConsoleVariableSource::Console);
  saveSetting("ShowAnimationPreview", bVisible);
}

/*
 */
void
AnimationPreviewUI::shutDown()
{
  PreviewState& state = getState();
  state.renderer.reset();
  state.grid.reset();
  state.targetView.reset();
  state.target.reset();
  state.component = nullptr;
  state.animator = nullptr;
  state.modelObject.reset();
  state.keyLight.reset();
  state.fillLight.reset();
  state.scene.reset();
  state.model.reset();
  state.clip.reset();
  state.models.clear();
  state.clips.clear();
  state.bDrawn = false;
  state.bExtracted = false;
}

} // namespace chEngineSDK
