/************************************************************************/
/**
 * @file chShadowRenderer.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Draws the shadow maps of a frame, before the lit render path reads them.
 */
/************************************************************************/

#include "chShadowRenderer.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chLightItem.h"
#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chMesh.h"
#include "chRenderItem.h"
#include "chRenderSettings.h"
#include "chScene.h"
#include "chVector4.h"

namespace chEngineSDK {

namespace {
constexpr Format kShadowMapFormat = Format::D32_SFLOAT;

constexpr int32 kMinShadowMapSize = 256;
// An atlas of four 4096 tiles is 8192 texels wide, 256 MB of 32-bit depth.
constexpr int32 kMaxShadowMapSize = 4096;

constexpr int32 kMaxFilterRadius = 3;

// Past half of a cascade the blend band would reach the cascade before it.
constexpr float kMaxCascadeBlend = 0.5f;

// The size of the fitted map is rounded up to this fraction of a world unit, so the
// floating point noise of a moving camera does not change it every frame.
constexpr float kRadiusSteps = 16.0f;

// Sent as the w of the caster frustum's near plane, so no point is ever behind it.
constexpr float kNoPlaneDistance = 1e30f;

// Must match PushConstants in shadow.hlsl.
struct ShadowPushConstants
{
  Matrix4 modelViewProjection;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 padding[2];
};
static_assert(sizeof(ShadowPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

NODISCARD FORCEINLINE uint32
getAtlasColumns(uint32 cascadeCount)
{
  return cascadeCount > 1 ? 2 : 1;
}

NODISCARD FORCEINLINE uint32
getAtlasRows(uint32 cascadeCount)
{
  return cascadeCount > 2 ? 2 : 1;
}
} // namespace

/*
 */
ShadowRenderer::ShadowRenderer()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "shadow");
  m_maskedFragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "shadow_masked");

  for (Vector<const RenderItem*>& casters : m_casters) {
    casters.reserve(256);
  }
  m_shadowViews.reserve(1);
}

/*
 */
ShadowRenderer::~ShadowRenderer() = default;

/*
 */
Span<const ShadowView>
ShadowRenderer::addPasses(RenderGraph& graph, const Scene& scene, const RenderView& view)
{
  m_shadowViews.clear();
  if (!g_cvarShadows.get()) {
    return {};
  }

  const LightItem* light = nullptr;
  for (const LightItem* candidate : view.lights) {
    if (candidate->type == LightType::Directional && candidate->bCastShadows) {
      light = candidate;
      break;
    }
  }
  if (light == nullptr) {
    return {};
  }

  updatePipelines(g_cvarShadowDepthBias.get(), g_cvarShadowSlopeBias.get());

  const uint32 cascadeCount = static_cast<uint32>(Math::clamp(
      g_cvarShadowCascades.get(), 1, static_cast<int32>(ShadowView::MAX_CASCADES)));
  const uint32 columns = getAtlasColumns(cascadeCount);
  const uint32 rows = getAtlasRows(cascadeCount);
  m_tileSize = static_cast<uint32>(
      Math::clamp(g_cvarShadowMapSize.get(), kMinShadowMapSize, kMaxShadowMapSize));
  updateShadowMap(m_tileSize * columns, m_tileSize * rows);

  const Camera& camera = view.camera;
  const float nearPlane = camera.getNearClipPlane();
  const float distance =
      Math::clamp(g_cvarShadowDistance.get(), nearPlane, camera.getFarClipPlane());
  float splits[ShadowView::MAX_CASCADES];
  computeCascadeSplits(nearPlane, distance,
                       Math::clamp(g_cvarShadowSplitLambda.get(), 0.0f, 1.0f),
                       Span<float>(splits, cascadeCount));
  const float blend = Math::clamp(g_cvarShadowCascadeBlend.get(), 0.0f, kMaxCascadeBlend);
  const float normalOffsetTexels = Math::max(g_cvarShadowNormalOffset.get(), 0.0f);

  const RGTextureHandle texture =
      graph.importTexture("ShadowMap", *m_shadowMap, *m_shadowMapView, m_shadowMapState,
                          ResourceState::ShaderRead);
  m_shadowMapState = ResourceState::ShaderRead;

  ShadowView& shadow = m_shadowViews.emplace_back();
  shadow.light = light;
  shadow.texture = texture;
  shadow.textureIndex = m_shadowMapView->getBindlessIndex();
  shadow.cascadeCount = cascadeCount;
  shadow.atlasScale =
      Vector2(1.0f / static_cast<float>(columns), 1.0f / static_cast<float>(rows));
  shadow.texelSize = Vector2(1.0f / static_cast<float>(m_shadowMapWidth),
                             1.0f / static_cast<float>(m_shadowMapHeight));
  shadow.filterRadius =
      static_cast<uint32>(Math::clamp(g_cvarShadowFilterRadius.get(), 0, kMaxFilterRadius));
  shadow.fadeEnd = distance;
  shadow.fadeStart = distance * (1.0f - Math::clamp(g_cvarShadowFade.get(), 0.0f, 1.0f));

  // Each cascade is fitted from where the one before starts blending into it, so the blend
  // band is covered by both.
  float cascadeStart = nearPlane;
  float fitStart = nearPlane;
  for (uint32 i = 0; i < cascadeCount; ++i) {
    const float splitDepth = splits[i];
    const bool bLast = i + 1 == cascadeCount;
    const float blendStart =
        bLast ? splitDepth : splitDepth - blend * (splitDepth - cascadeStart);
    const DirectionalShadowProjection projection = computeDirectionalProjection(
        camera, light->direction, fitStart, splitDepth, m_tileSize);
    scene.gatherRenderItems(projection.casterFrustum, m_casters[i]);

    ShadowCascade& cascade = shadow.cascades[i];
    cascade.viewProjection = projection.viewProjection;
    cascade.atlasOffset = Vector2(static_cast<float>(i % columns) * shadow.atlasScale.x,
                                  static_cast<float>(i / columns) * shadow.atlasScale.y);
    cascade.splitDepth = splitDepth;
    cascade.blendStart = blendStart;
    cascade.normalOffset = projection.texelWorldSize * normalOffsetTexels;

    cascadeStart = splitDepth;
    fitStart = blendStart;
  }

  graph.addPass("ShadowDepth")
      .writeDepth(texture)
      .setExecute([this](RenderPassContext& context) { recordDraws(context); });
  return m_shadowViews;
}

/*
 */
DirectionalShadowProjection
ShadowRenderer::computeDirectionalProjection(const Camera& camera,
                                             const Vector3& lightDirection,
                                             float nearDistance,
                                             float farDistance,
                                             uint32 mapSize)
{
  // The covered part of the view goes between the clip depths of the points at both
  // distances in front of the camera; its corners come back from clip space.
  const Matrix4& cameraViewProjection = camera.getViewProjectionMatrix();
  const auto getClipDepth = [&camera, &cameraViewProjection](float distance) {
    const Vector3 point = camera.getPosition() + camera.getForwardVector() * distance;
    const Vector4 clip =
        cameraViewProjection.transformVector4(Vector4(point.x, point.y, point.z, 1.0f));
    return clip.z / clip.w;
  };
  const float depths[2] = {
      Math::max(getClipDepth(Math::max(nearDistance, camera.getNearClipPlane())), 0.0f),
      getClipDepth(Math::min(farDistance, camera.getFarClipPlane()))};
  constexpr float kSigns[2] = {-1.0f, 1.0f};

  const Matrix4 inverseViewProjection = cameraViewProjection.getInverse();
  Vector3 corners[8];
  Vector3 center = Vector3::ZERO;
  uint32 cornerCount = 0;
  for (const float depth : depths) {
    for (const float y : kSigns) {
      for (const float x : kSigns) {
        const Vector4 corner = inverseViewProjection.transformVector4(Vector4(x, y, depth, 1.0f));
        corners[cornerCount] =
            Vector3(corner.x / corner.w, corner.y / corner.w, corner.z / corner.w);
        center += corners[cornerCount];
        ++cornerCount;
      }
    }
  }
  center *= 1.0f / static_cast<float>(cornerCount);

  float radius = 0.0f;
  for (const Vector3& corner : corners) {
    radius = Math::max(radius, center.distance(corner));
  }
  radius = Math::ceil(radius * kRadiusSteps) / kRadiusSteps;

  // A light pointing straight down the world up axis needs another up vector.
  const Vector3& up = Math::abs(lightDirection.z) > 0.99f ? Vector3::FORWARD : Vector3::UP;
  const LookAtMatrix lightRotation(Vector3::ZERO, lightDirection, up);
  const float texelWorldSize = radius * 2.0f / static_cast<float>(mapSize);
  const Vector4 rotatedCenter = lightRotation.transformPosition(center);
  Vector3 lightCenter(rotatedCenter.x, rotatedCenter.y, rotatedCenter.z);
  lightCenter.x = Math::floor(lightCenter.x / texelWorldSize) * texelWorldSize;
  lightCenter.y = Math::floor(lightCenter.y / texelWorldSize) * texelWorldSize;

  const Matrix4 lightView = lightRotation * TranslationMatrix(-lightCenter);
  DirectionalShadowProjection projection;
  projection.viewProjection =
      lightView * OrthographicMatrix(radius, radius, -radius, radius);
  projection.casterFrustum = Frustum(projection.viewProjection);
  projection.casterFrustum.planes[static_cast<SIZE_T>(FrustumSide::Near)].w =
      -kNoPlaneDistance;
  projection.texelWorldSize = texelWorldSize;
  return projection;
}

/*
 */
void
ShadowRenderer::computeCascadeSplits(float nearPlane,
                                     float distance,
                                     float lambda,
                                     Span<float> outSplits)
{
  const SIZE_T count = outSplits.size();
  const float ratio = distance / nearPlane;
  for (SIZE_T i = 0; i < count; ++i) {
    const float fraction = static_cast<float>(i + 1) / static_cast<float>(count);
    const float even = nearPlane + (distance - nearPlane) * fraction;
    const float proportional = nearPlane * Math::pow(ratio, fraction);
    outSplits[i] = Math::lerp(even, proportional, lambda);
  }
  // Rounding must not leave the far end short of the distance.
  if (count > 0) {
    outSplits[count - 1] = distance;
  }
}

/*
 */
void
ShadowRenderer::updateShadowMap(uint32 width, uint32 height)
{
  if (m_shadowMap != nullptr && m_shadowMapWidth == width && m_shadowMapHeight == height) {
    return;
  }

  // The view points at its texture, so it goes first.
  m_shadowMapView.reset();
  m_shadowMap = IGraphicsAPI::instance().createTexture(
      {.format = kShadowMapFormat,
       .width = width,
       .height = height,
       .usage = TextureUsage::DepthStencil | TextureUsage::Sampled});
  m_shadowMapView = m_shadowMap->createView();
  m_shadowMapWidth = width;
  m_shadowMapHeight = height;
  m_shadowMapState = ResourceState::Undefined;
}

/*
 */
void
ShadowRenderer::updatePipelines(float depthBias, float slopeBias)
{
  if (m_pipelines.isBuilt() && depthBias == m_depthBias && slopeBias == m_slopeBias) {
    return;
  }

  // Pipelines are freed through the deferred deletion, so frames still on the GPU keep the
  // old ones; clearing keeps a slider drag from piling up one set per value.
  m_pipelineCache.clear();
  // Opaque casters only write depth, so they need no fragment shader. Depth clamp keeps the
  // casters between the light and the map's near plane, at depth 0.
  const GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                          .vertexLayout = VertexStatic::getLayout(),
                                          .raster = {.depthBiasConstant = depthBias,
                                                     .depthBiasSlope = slopeBias,
                                                     .depthClampEnable = true},
                                          .depthFormat = kShadowMapFormat};
  m_pipelines.build(m_pipelineCache, pipelineDesc, m_maskedFragmentShader);
  m_depthBias = depthBias;
  m_slopeBias = slopeBias;
}

/*
 */
void
ShadowRenderer::recordDraws(RenderPassContext& context)
{
  ICommandList& commandList = context.getCommandList();
  const ShadowView& shadow = m_shadowViews[0];
  const uint32 columns = getAtlasColumns(shadow.cascadeCount);
  const float tileSize = static_cast<float>(m_tileSize);
  ShadowPushConstants pushConstants{.modelViewProjection = Matrix4::IDENTITY,
                                    .materialIndex = 0,
                                    .samplerIndex = m_sampler->getBindlessIndex(),
                                    .padding = {0, 0}};
  for (uint32 i = 0; i < shadow.cascadeCount; ++i) {
    const uint32 x = (i % columns) * m_tileSize;
    const uint32 y = (i / columns) * m_tileSize;
    commandList.setViewport(static_cast<float>(x), static_cast<float>(y), tileSize, tileSize);
    commandList.setScissor(x, y, m_tileSize, m_tileSize);

    const Matrix4& lightViewProjection = shadow.cascades[i].viewProjection;
    m_pipelines.recordDraws(
        commandList, m_casters[i], m_defaultMaterial,
        [&commandList, &pushConstants, &lightViewProjection](const RenderItem& item,
                                                             const Material& material) {
          pushConstants.modelViewProjection = item.worldMatrix * lightViewProjection;
          pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
          commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        });
  }
}

} // namespace chEngineSDK
