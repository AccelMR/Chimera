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
#include "chISampler.h"
#include "chLightItem.h"
#include "chMath.h"
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
  uint32 shadowBufferIndex;
  uint32 shadowSamplerIndex;
  uint32 debugShadowIndex;
  uint32 featureFlags;
  uint32 irradianceIndex;
  uint32 prefilteredIndex;
  uint32 brdfLutIndex;
  uint32 environmentSamplerIndex;
  float environmentIntensity;
  float prefilteredMaxMip;
};
static_assert(sizeof(ViewData) % 16 == 0, "Constant buffers are read in 16-byte rows");

// Must match the kFeature flags in chView.hlsli.
constexpr uint32 kFeatureNormalMaps = 1u << 0;
constexpr uint32 kFeatureOcclusionMaps = 1u << 1;
constexpr uint32 kFeatureEmissive = 1u << 2;
constexpr uint32 kFeatureAOMultiBounce = 1u << 3;

uint32
getFeatureFlags()
{
  return (g_cvarNormalMaps.get() ? kFeatureNormalMaps : 0u) |
         (g_cvarOcclusionMaps.get() ? kFeatureOcclusionMaps : 0u) |
         (g_cvarEmissive.get() ? kFeatureEmissive : 0u) |
         (g_cvarAOMultiBounce.get() ? kFeatureAOMultiBounce : 0u);
}

// Must match kNoShadow in chShadows.hlsli.
constexpr uint32 kNoShadow = 0xFFFFFFFF;

// Must match kNoEnvironment in chView.hlsli.
constexpr uint32 kNoEnvironment = 0xFFFFFFFF;

// Enough for a small scene, so most frames never make the light buffers again.
constexpr uint32 kInitialLightCapacity = 16;
constexpr uint32 kInitialShadowCapacity = 4;

NODISCARD SPtr<IBuffer>
createStorageBuffer(uint32 capacity, SIZE_T elementSize)
{
  return IGraphicsAPI::instance().createBuffer(
      {.size = static_cast<uint32>(capacity * elementSize),
       .usage = BufferUsage::StorageBuffer,
       .memoryUsage = MemoryUsage::CpuToGpu});
}

// Makes the buffer of this frame slot again, bigger, when the frame has more elements.
template<typename Element, SIZE_T Slots>
IBuffer&
writeStorageBuffer(Array<SPtr<IBuffer>, Slots>& buffers,
                   Array<uint32, Slots>& capacities,
                   uint32 frameIndex,
                   const Vector<Element>& elements)
{
  const uint32 count = static_cast<uint32>(elements.size());
  uint32& capacity = capacities[frameIndex];
  if (count > capacity) {
    capacity = Math::max(count, capacity * 2);
    buffers[frameIndex] = createStorageBuffer(capacity, sizeof(Element));
  }
  IBuffer& buffer = *buffers[frameIndex];
  if (count > 0) {
    buffer.update(elements.data(), elements.size() * sizeof(Element));
  }
  return buffer;
}

NODISCARD uint32
findShadowIndex(Span<const ShadowView> shadows, const LightItem* light)
{
  for (SIZE_T i = 0; i < shadows.size(); ++i) {
    if (shadows[i].light == light) {
      return static_cast<uint32>(i);
    }
  }
  return kNoShadow;
}

// The directional shadow, the only one with cascades to tint.
NODISCARD uint32
findCascadedShadowIndex(Span<const ShadowView> shadows)
{
  for (SIZE_T i = 0; i < shadows.size(); ++i) {
    if (shadows[i].type == ShadowMapType::Cascaded) {
      return static_cast<uint32>(i);
    }
  }
  return kNoShadow;
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
    m_lightBuffers[i] = createStorageBuffer(kInitialLightCapacity, sizeof(GpuLight));
    m_lightBufferCapacities[i] = kInitialLightCapacity;
    m_shadowBuffers[i] = createStorageBuffer(kInitialShadowCapacity, sizeof(GpuShadow));
    m_shadowBufferCapacities[i] = kInitialShadowCapacity;
  }
  m_gpuLights.reserve(kInitialLightCapacity);
  m_gpuShadows.reserve(kInitialShadowCapacity);

  // Lit where the receiver is not farther than the stored depth; linear filtering blends
  // the four nearest results, which softens the edge.
  m_shadowSampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                               .minFilter = SamplerFilter::Linear,
                                               .mipmapMode = SamplerMipmapMode::Nearest,
                                               .addressModeU = SamplerAddressMode::ClampToEdge,
                                               .addressModeV = SamplerAddressMode::ClampToEdge,
                                               .addressModeW = SamplerAddressMode::ClampToEdge,
                                               .compareEnable = true,
                                               .compareOp = CompareOp::LessOrEqual,
                                               .maxLod = 0.0f});
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
         .shadowIndex = findShadowIndex(view.shadows, light),
         .padding = {0.0f, 0.0f}});
  }

  static_assert(sizeof(GpuShadow::cascades) / sizeof(GpuShadowCascade) ==
                ShadowView::MAX_CASCADES);
  m_gpuShadows.clear();
  for (const ShadowView& shadow : view.shadows) {
    // Value initialized: its default constructor is implicit, so unused cascades are zeros.
    GpuShadow& gpuShadow = m_gpuShadows.emplace_back();
    gpuShadow.textureIndex = shadow.textureIndex;
    gpuShadow.cascadeCount = shadow.cascadeCount;
    gpuShadow.filterRadius = shadow.filterRadius;
    gpuShadow.fadeStart = shadow.fadeStart;
    gpuShadow.texelSize[0] = shadow.texelSize.x;
    gpuShadow.texelSize[1] = shadow.texelSize.y;
    gpuShadow.atlasScale[0] = shadow.atlasScale.x;
    gpuShadow.atlasScale[1] = shadow.atlasScale.y;
    gpuShadow.fadeEnd = shadow.fadeEnd;
    gpuShadow.type = static_cast<uint32>(shadow.type);
    gpuShadow.lightFade = shadow.lightFade;
    gpuShadow.cubeIndex = shadow.cubeIndex;
    gpuShadow.lightPosition[0] = shadow.light->position.x;
    gpuShadow.lightPosition[1] = shadow.light->position.y;
    gpuShadow.lightPosition[2] = shadow.light->position.z;
    gpuShadow.cubeDepthScale = shadow.cubeDepthScale;
    gpuShadow.cubeDepthOffset = shadow.cubeDepthOffset;
    gpuShadow.cubeNormalOffset = shadow.cubeNormalOffset;
    for (uint32 i = 0; i < shadow.cascadeCount; ++i) {
      const ShadowCascade& cascade = shadow.cascades[i];
      gpuShadow.splitDepths[i] = cascade.splitDepth;
      gpuShadow.blendStarts[i] = cascade.blendStart;
      GpuShadowCascade& gpuCascade = gpuShadow.cascades[i];
      gpuCascade.viewProjection = cascade.viewProjection;
      gpuCascade.atlasOffset[0] = cascade.atlasOffset.x;
      gpuCascade.atlasOffset[1] = cascade.atlasOffset.y;
      gpuCascade.normalOffset = cascade.normalOffset;
    }
  }

  const IBuffer& lightBuffer =
      writeStorageBuffer(m_lightBuffers, m_lightBufferCapacities, frameIndex, m_gpuLights);
  const IBuffer& shadowBuffer =
      writeStorageBuffer(m_shadowBuffers, m_shadowBufferCapacities, frameIndex, m_gpuShadows);

  const ImageBasedLightingView& ibl = view.ibl;
  const Camera& camera = view.camera;
  const Vector3& cameraPosition = camera.getPosition();
  const ViewData viewData{
      .view = camera.getViewMatrix(),
      .projection = camera.getProjectionMatrix(),
      .inverseViewProjection = camera.getViewProjectionMatrix().getInverse(),
      .cameraPosition = {cameraPosition.x, cameraPosition.y, cameraPosition.z},
      .lightCount = static_cast<uint32>(m_gpuLights.size()),
      .lightBufferIndex = lightBuffer.getBindlessIndex(),
      .ambient = g_cvarAmbient.get(),
      .shadowBufferIndex = shadowBuffer.getBindlessIndex(),
      .shadowSamplerIndex = m_shadowSampler->getBindlessIndex(),
      .debugShadowIndex =
          view.bShowShadowCascades ? findCascadedShadowIndex(view.shadows) : kNoShadow,
      .featureFlags = getFeatureFlags(),
      .irradianceIndex = ibl.hasEnvironment() ? ibl.irradianceIndex : kNoEnvironment,
      .prefilteredIndex = ibl.prefilteredIndex,
      .brdfLutIndex = ibl.brdfLutIndex,
      .environmentSamplerIndex = ibl.samplerIndex,
      .environmentIntensity = ibl.intensity,
      .prefilteredMaxMip = ibl.prefilteredMaxMip};
  IBuffer& viewBuffer = *m_viewBuffers[frameIndex];
  viewBuffer.update(&viewData, sizeof(viewData));
  return viewBuffer.getBindlessIndex();
}

} // namespace chEngineSDK
