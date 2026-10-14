/************************************************************************/
/**
 * @file chSceneViewBuffers.cpp
 * @author AccelMR
 * @date 2026/10/13
 * @brief Camera and light data of a frame, as the lighting shaders read it.
 */
/************************************************************************/

#include "chSceneViewBuffers.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chIGraphicsAPI.h"
#include "chIRenderPath.h"
#include "chLightItem.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chRenderSettings.h"

namespace chEngineSDK {

namespace {
// Must match ViewData in chView.hlsli.
struct ViewData
{
  Matrix4 view;
  Matrix4 projection;
  Matrix4 inverseViewProjection;
  float cameraPosition[3];
  uint32 lightCount;
  uint32 lightBufferIndex;
  float ambient;
  float padding[2];
};

// Enough for a small scene, so most frames never make the light buffers again.
constexpr uint32 kInitialLightCapacity = 16;

NODISCARD SPtr<IBuffer>
createLightBuffer(uint32 capacity, SIZE_T lightSize)
{
  return IGraphicsAPI::instance().createBuffer(
      {.size = static_cast<uint32>(capacity * lightSize),
       .usage = BufferUsage::StorageBuffer,
       .memoryUsage = MemoryUsage::CpuToGpu});
}
} // namespace

/*
 */
SceneViewBuffers::SceneViewBuffers()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  for (SPtr<IBuffer>& viewBuffer : m_viewBuffers) {
    viewBuffer = graphicsAPI.createBuffer({.size = sizeof(ViewData),
                                           .usage = BufferUsage::UniformBuffer,
                                           .memoryUsage = MemoryUsage::CpuToGpu});
  }
  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    m_lightBuffers[i] = createLightBuffer(kInitialLightCapacity, sizeof(GpuLight));
    m_lightBufferCapacities[i] = kInitialLightCapacity;
  }
  m_gpuLights.reserve(kInitialLightCapacity);
}

/*
 */
SceneViewBuffers::~SceneViewBuffers() = default;

/*
 */
uint32
SceneViewBuffers::update(const RenderView& view)
{
  const uint32 frameIndex = IGraphicsAPI::instance().getFrameIndex();
  const uint32 lightCount = static_cast<uint32>(view.lights.size());

  m_gpuLights.clear();
  for (const LightItem* light : view.lights) {
    // Spot falloff as glTF defines it: 0 at the outer cone, 1 at the inner one.
    float spotScale = 0.0f;
    float spotOffset = 1.0f;
    if (light->type == LightType::Spot) {
      spotScale = 1.0f / Math::max(light->innerConeCos - light->outerConeCos, 1e-4f);
      spotOffset = -light->outerConeCos * spotScale;
    }
    m_gpuLights.push_back(
        {.position = {light->position.x, light->position.y, light->position.z},
         .type = static_cast<uint32>(light->type),
         .direction = {light->direction.x, light->direction.y, light->direction.z},
         .range = light->range,
         .color = {light->color.r, light->color.g, light->color.b},
         .spotScale = spotScale,
         .spotOffset = spotOffset,
         .padding = {0.0f, 0.0f, 0.0f}});
  }

  uint32& capacity = m_lightBufferCapacities[frameIndex];
  if (lightCount > capacity) {
    capacity = Math::max(lightCount, capacity * 2);
    m_lightBuffers[frameIndex] = createLightBuffer(capacity, sizeof(GpuLight));
  }
  IBuffer& lightBuffer = *m_lightBuffers[frameIndex];
  if (lightCount > 0) {
    lightBuffer.update(m_gpuLights.data(), m_gpuLights.size() * sizeof(GpuLight));
  }

  const Camera& camera = view.camera;
  const Vector3& cameraPosition = camera.getPosition();
  const ViewData viewData{
      .view = camera.getViewMatrix(),
      .projection = camera.getProjectionMatrix(),
      .inverseViewProjection = camera.getViewProjectionMatrix().getInverse(),
      .cameraPosition = {cameraPosition.x, cameraPosition.y, cameraPosition.z},
      .lightCount = lightCount,
      .lightBufferIndex = lightBuffer.getBindlessIndex(),
      .ambient = g_cvarAmbient.get(),
      .padding = {0.0f, 0.0f}};
  IBuffer& viewBuffer = *m_viewBuffers[frameIndex];
  viewBuffer.update(&viewData, sizeof(viewData));
  return viewBuffer.getBindlessIndex();
}

} // namespace chEngineSDK
