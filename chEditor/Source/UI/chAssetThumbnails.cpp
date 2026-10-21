/************************************************************************/
/**
 * @file chAssetThumbnails.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Pictures of assets for the Content Browser.
 */
/************************************************************************/
#include "chAssetThumbnails.h"

#include "chAngle.h"
#include "chAssetManager.h"
#include "chBox.h"
#include "chCamera.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chGameObjectSerializer.h"
#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chLightComponent.h"
#include "chLogger.h"
#include "chMaterialAsset.h"
#include "chMath.h"
#include "chMesh.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chPrefabAsset.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneRenderer.h"
#include "chShapeComponent.h"
#include "chStringUtils.h"
#include "chTextureAsset.h"
#include "chUIHelpers.h"

#include "imgui.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(AssetThumbnailsLog, All);

namespace {
ConsoleVariable<int32> g_cvarThumbnailSize("Editor.ThumbnailSize",
                                           256,
                                           "Pixels per side of the asset thumbnails.");
ConsoleVariable<float> g_cvarThumbnailFieldOfView(
    "Editor.ThumbnailFieldOfView",
    30.0f,
    "Horizontal field of view, in degrees, of the camera that draws the asset thumbnails.");
ConsoleVariable<float> g_cvarThumbnailLightIntensity(
    "Editor.ThumbnailLightIntensity",
    3.0f,
    "Intensity of the key light of the asset thumbnails; the fill light gets a third.");

// The tonemap writes encoded sRGB, the same as into the swap chain, so ImGui shows it as is.
constexpr Format kTargetFormat = Format::R8G8B8A8_UNORM;
constexpr int32 kMinSize = 64;
constexpr int32 kMaxSize = 1024;
constexpr float kMinFieldOfView = 10.0f;
constexpr float kMaxFieldOfView = 90.0f;
// Seen from the front left and a little above, like the Animation Preview.
constexpr float kFramePitch = -20.0f;
constexpr float kFrameYaw = -150.0f;
constexpr float kFillLightFraction = 1.0f / 3.0f;
constexpr float kNearPlaneScale = 0.01f;
constexpr float kFarPlaneScale = 10.0f;
constexpr float kMinRadius = 0.01f;
// Enough faces that the outline of the sphere looks round at the largest size.
constexpr ShapeDesc kMaterialSphere{.type = ShapeType::Sphere, .segments = 64, .rings = 32};

struct Thumbnail
{
  SPtr<ITexture> texture;
  uint64 textureId = 0;
  bool bQueued = false;
};

struct ThumbnailsState
{
  // Every asset a tile asked for; a failed one keeps an empty entry, so it is not tried
  // again until it is regenerated.
  UnorderedMap<UUID, Thumbnail> thumbnails;
  // First asked, first drawn; head is the next one.
  Vector<UUID> queue;
  SIZE_T queueHead = 0;

  UniquePtr<Scene> scene;
  SPtr<GameObject> keyLight;
  SPtr<GameObject> fillLight;
  // Keeps the shared sphere mesh alive between material thumbnails.
  SPtr<Mesh> sphereMesh;
  UniquePtr<SceneRenderer> renderer;
  Camera camera;

  // The asset being drawn, from update() to recordFrame().
  SPtr<IAsset> asset;
  SPtr<GameObject> object;
  SPtr<ITexture> target;
  SPtr<ITextureView> targetView;
  // Loaded only for the thumbnail, so it is unloaded once drawn.
  bool bUnloadAsset = false;
  bool bExtracted = false;
};

ThumbnailsState&
getState()
{
  static ThumbnailsState state;
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
saveSetting(StringView key, int32 value)
{
  ANSICHAR buffer[StringUtils::MAX_INTEGER_CHARS];
  EngineConfig::setUserValue("Editor", "Editor", key, StringUtils::toChars(buffer, value));
}

/*
 */
void
saveSettings()
{
  saveSetting("ThumbnailSize", g_cvarThumbnailSize.get());
  saveSetting("ThumbnailFieldOfView", g_cvarThumbnailFieldOfView.get());
  saveSetting("ThumbnailLightIntensity", g_cvarThumbnailLightIntensity.get());
}

/*
 */
void
enqueue(ThumbnailsState& state, const UUID& uuid, Thumbnail& thumbnail)
{
  if (!thumbnail.bQueued) {
    thumbnail.bQueued = true;
    state.queue.push_back(uuid);
  }
}

/*
 * The next asset of the queue that still exists, or null.
 */
SPtr<IAsset>
popQueue(ThumbnailsState& state)
{
  while (state.queueHead < state.queue.size()) {
    const UUID uuid = state.queue[state.queueHead++];
    auto it = state.thumbnails.find(uuid);
    if (it == state.thumbnails.end() || !it->second.bQueued) {
      continue;
    }
    it->second.bQueued = false;
    if (SPtr<IAsset> asset = AssetManager::instance().getAsset(uuid)) {
      return asset;
    }
  }
  state.queue.clear();
  state.queueHead = 0;
  return nullptr;
}

/*
 */
SPtr<GameObject>
addLight(Scene& scene, const ANSICHAR* name, const Rotator& rotation)
{
  const SPtr<GameObject> light = scene.createGameObject(name);
  light->getTransform().setLocalRotation(Quaternion(rotation));
  light->addComponent<DirectionalLightComponent>().setCastShadows(false);
  return light;
}

/*
 */
void
updateLights(ThumbnailsState& state)
{
  const float intensity = Math::max(g_cvarThumbnailLightIntensity.get(), 0.0f);
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
 * Drops the object of the last asset, and the asset when it was loaded only to be drawn.
 */
void
finishAsset(ThumbnailsState& state)
{
  if (state.object) {
    state.scene->destroyGameObject(*state.object);
    state.object.reset();
  }
  if (state.asset && state.bUnloadAsset) {
    AssetManager::instance().unloadAsset(state.asset);
  }
  state.asset.reset();
  state.bUnloadAsset = false;
}

/*
 * Textures are their own thumbnail. Keeping the GPU texture keeps it valid after the asset
 * is unloaded.
 */
void
takeTexture(const SPtr<IAsset>& asset, Thumbnail& thumbnail)
{
  const bool bLoadedHere = asset->isUnloaded();
  if (bLoadedHere && !AssetManager::instance().syncLoadAsset(asset)) {
    CH_LOG_ERROR(AssetThumbnailsLog, "Failed to load texture asset: {0}", asset->getName());
    return;
  }
  const SPtr<TextureAsset> textureAsset = asset->as<TextureAsset>();
  SPtr<ITexture> texture = textureAsset ? textureAsset->getTexture() : nullptr;
  const uint64 textureId = texture ? ImGuiRenderer::getTextureId(*texture) : 0;
  if (textureId != 0) {
    thumbnail.texture = std::move(texture);
    thumbnail.textureId = textureId;
  }
  if (bLoadedHere) {
    AssetManager::instance().unloadAsset(asset);
  }
}

/*
 */
SPtr<GameObject>
createObject(ThumbnailsState& state, const SPtr<IAsset>& asset)
{
  Scene& scene = *state.scene;
  if (const SPtr<ModelAsset> model = asset->as<ModelAsset>()) {
    if (!model->getModel()) {
      return nullptr;
    }
    const SPtr<GameObject> object = scene.createGameObject(asset->getName());
    object->addComponent<ModelComponent>(model);
    return object;
  }
  if (const SPtr<MaterialAsset> material = asset->as<MaterialAsset>()) {
    const SPtr<GameObject> object = scene.createGameObject(asset->getName());
    ShapeComponent& shape = object->addComponent<ShapeComponent>(kMaterialSphere);
    shape.setMaterial(material);
    state.sphereMesh = shape.getMesh();
    return object;
  }
  if (const SPtr<PrefabAsset> prefab = asset->as<PrefabAsset>()) {
    return GameObjectSerializer::instantiate(*prefab, scene, nullptr);
  }
  return nullptr;
}

/*
 * The field of view is horizontal and the target square, so the sphere around the bounds
 * fits with the same distance either way.
 */
bool
frameCamera(ThumbnailsState& state)
{
  AABox bounds;
  if (!state.object->getHierarchyWorldBounds(bounds)) {
    return false;
  }
  Camera& camera = state.camera;
  const float fieldOfView =
      Math::clamp(g_cvarThumbnailFieldOfView.get(), kMinFieldOfView, kMaxFieldOfView);
  camera.setFieldOfView(Radian(Degree(fieldOfView)));
  const float radius = Math::max(bounds.getExtent().magnitude(), kMinRadius);
  const float distance = radius / Math::sin(camera.getFieldOfView() * 0.5f);

  const Vector3 forward =
      Quaternion(Rotator(kFramePitch, kFrameYaw, 0.0f)).rotateVector(Vector3::FORWARD);
  const Vector3 center = bounds.getCenter();
  camera.setClipPlanes(radius * kNearPlaneScale, (distance + radius) * kFarPlaneScale);
  camera.setPosition(center - forward * distance);
  camera.lookAt(center);
  return true;
}

/*
 * A new texture for every thumbnail, because the one shown before stays until this one is
 * drawn.
 */
void
createTarget(ThumbnailsState& state)
{
  const uint32 size =
      static_cast<uint32>(Math::clamp(g_cvarThumbnailSize.get(), kMinSize, kMaxSize));
  state.target = IGraphicsAPI::instance().createTexture(
      {.format = kTargetFormat,
       .width = size,
       .height = size,
       .usage = TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled});
  state.targetView = state.target->createView();
  state.camera.setViewportSize(static_cast<float>(size), static_cast<float>(size));
}

/*
 * Builds the scene of the asset; false when there is nothing to draw.
 */
bool
prepareAsset(ThumbnailsState& state, const SPtr<IAsset>& asset)
{
  if (!state.scene) {
    state.scene = chMakeUnique<Scene>("AssetThumbnails", UUID::createRandom());
    state.keyLight = addLight(*state.scene, "Key Light", Rotator(-40.0f, -120.0f, 0.0f));
    state.fillLight = addLight(*state.scene, "Fill Light", Rotator(-20.0f, 160.0f, 0.0f));
    // The thumbnails are Lit whatever the main viewport shows, and the main renderer times
    // the GPU.
    state.renderer = chMakeUnique<SceneRenderer>(
        SceneRendererDesc{.bProfileGpu = false, .viewMode = ViewMode::Lit});
  }

  state.asset = asset;
  state.bUnloadAsset = asset->isUnloaded();
  if (state.bUnloadAsset && !AssetManager::instance().syncLoadAsset(asset)) {
    CH_LOG_ERROR(AssetThumbnailsLog, "Failed to load asset: {0}", asset->getName());
    state.bUnloadAsset = false;
    return false;
  }
  state.object = createObject(state, asset);
  if (!state.object) {
    return false;
  }

  updateLights(state);
  state.scene->updateTransforms();
  if (!frameCamera(state)) {
    return false;
  }
  createTarget(state);
  state.renderer->extract(*state.scene, state.camera, chUIHelpers::UIHelpers::rendererColor);
  return true;
}
} // namespace

/*
 */
uint64
AssetThumbnails::get(const SPtr<IAsset>& asset)
{
  if (!canHaveThumbnail(*asset)) {
    return 0;
  }
  ThumbnailsState& state = getState();
  const auto [it, bAdded] = state.thumbnails.try_emplace(asset->getUUID());
  if (bAdded) {
    enqueue(state, it->first, it->second);
  }
  return it->second.textureId;
}

/*
 */
bool
AssetThumbnails::canHaveThumbnail(const IAsset& asset)
{
  return asset.isTypeOf<ModelAsset>() || asset.isTypeOf<MaterialAsset>() ||
         asset.isTypeOf<PrefabAsset>() || asset.isTypeOf<TextureAsset>();
}

/*
 */
void
AssetThumbnails::regenerate(const IAsset& asset)
{
  ThumbnailsState& state = getState();
  auto it = state.thumbnails.find(asset.getUUID());
  if (it != state.thumbnails.end()) {
    enqueue(state, it->first, it->second);
  }
}

/*
 */
void
AssetThumbnails::regenerateAll()
{
  ThumbnailsState& state = getState();
  for (auto& [uuid, thumbnail] : state.thumbnails) {
    enqueue(state, uuid, thumbnail);
  }
}

/*
 */
void
AssetThumbnails::forget(const IAsset& asset)
{
  ThumbnailsState& state = getState();
  state.thumbnails.erase(asset.getUUID());
  if (state.asset.get() == &asset) {
    // Drawn this frame or not, the asset is about to go, so nothing of it is kept.
    state.bExtracted = false;
    state.target.reset();
    state.targetView.reset();
    finishAsset(state);
  }
}

/*
 */
void
AssetThumbnails::update()
{
  ThumbnailsState& state = getState();
  // A frame that was not rendered (minimized window) still has its asset ready.
  if (state.bExtracted) {
    return;
  }

  const SPtr<IAsset> asset = popQueue(state);
  if (!asset) {
    return;
  }
  Thumbnail& thumbnail = state.thumbnails[asset->getUUID()];
  if (asset->isTypeOf<TextureAsset>()) {
    takeTexture(asset, thumbnail);
    return;
  }

  state.bExtracted = prepareAsset(state, asset);
  if (!state.bExtracted) {
    CH_LOG_DEBUG(AssetThumbnailsLog, "Nothing to draw for the thumbnail of {0}.",
                 asset->getName());
    state.target.reset();
    state.targetView.reset();
    finishAsset(state);
  }
}

/*
 */
void
AssetThumbnails::recordFrame(ICommandList& commandList)
{
  ThumbnailsState& state = getState();
  if (!state.bExtracted) {
    return;
  }
  state.bExtracted = false;

  RenderGraph& graph = state.renderer->beginFrame();
  const RGTextureHandle output =
      graph.importTexture("AssetThumbnail", *state.target, *state.targetView,
                          ResourceState::Undefined, ResourceState::ShaderRead);
  state.renderer->addScenePasses(output);
  state.renderer->execute(commandList);

  // The commands are recorded, so the asset can go: the GPU buffers it releases wait in the
  // deferred deletion until this frame is done. Nothing between update() and here can start
  // using it, because the UI runs before update().
  auto it = state.thumbnails.find(state.asset->getUUID());
  if (it != state.thumbnails.end()) {
    it->second.textureId = ImGuiRenderer::getTextureId(*state.target);
    it->second.texture = std::move(state.target);
  }
  state.target.reset();
  state.targetView.reset();
  finishAsset(state);
}

/*
 */
void
AssetThumbnails::renderSettings()
{
  bool bChanged = false;
  int32 size = g_cvarThumbnailSize.get();
  ImGui::SetNextItemWidth(160.0f);
  if (ImGui::SliderInt("Size", &size, kMinSize, kMaxSize, "%d px")) {
    g_cvarThumbnailSize.set(size, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", g_cvarThumbnailSize.getHelp().c_str());
  bChanged |= ImGui::IsItemDeactivatedAfterEdit();

  const auto slider = [&bChanged](const ANSICHAR* label, ConsoleVariable<float>& variable,
                                  float minValue, float maxValue, const ANSICHAR* format) {
    float value = variable.get();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderFloat(label, &value, minValue, maxValue, format)) {
      variable.set(value, ConsoleVariableSource::Console);
    }
    ImGui::SetItemTooltip("%s", variable.getHelp().c_str());
    bChanged |= ImGui::IsItemDeactivatedAfterEdit();
  };
  slider("Field of View", g_cvarThumbnailFieldOfView, kMinFieldOfView, kMaxFieldOfView,
         "%.0f deg");
  slider("Light Intensity", g_cvarThumbnailLightIntensity, 0.0f, 20.0f, "%.2f");

  if (ImGui::Button("Reset to Defaults")) {
    g_cvarThumbnailSize.set(g_cvarThumbnailSize.getDefault(), ConsoleVariableSource::Console);
    g_cvarThumbnailFieldOfView.set(g_cvarThumbnailFieldOfView.getDefault(),
                                   ConsoleVariableSource::Console);
    g_cvarThumbnailLightIntensity.set(g_cvarThumbnailLightIntensity.getDefault(),
                                      ConsoleVariableSource::Console);
    bChanged = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Regenerate All")) {
    regenerateAll();
  }

  if (bChanged) {
    saveSettings();
    regenerateAll();
  }
}

/*
 */
void
AssetThumbnails::shutDown()
{
  ThumbnailsState& state = getState();
  state.bExtracted = false;
  state.target.reset();
  state.targetView.reset();
  finishAsset(state);
  state.thumbnails.clear();
  state.queue.clear();
  state.queueHead = 0;
  state.renderer.reset();
  state.sphereMesh.reset();
  state.keyLight.reset();
  state.fillLight.reset();
  state.scene.reset();
}

} // namespace chEngineSDK
