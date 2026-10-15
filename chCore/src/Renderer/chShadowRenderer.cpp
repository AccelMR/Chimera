/************************************************************************/
/**
 * @file chShadowRenderer.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Draws the shadow maps of a frame, before the lit render path reads them.
 */
/************************************************************************/

#include "chShadowRenderer.h"

#include "chAlgorithm.h"
#include "chAngle.h"
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
// Spot tiles shrink so their atlas stays within this side.
constexpr uint32 kMaxSpotAtlasSize = 8192;

constexpr int32 kMaxFilterRadius = 3;

// Past half of a cascade the blend band would reach the cascade before it.
constexpr float kMaxCascadeBlend = 0.5f;

// The size of the fitted map is rounded up to this fraction of a world unit, so the
// floating point noise of a moving camera does not change it every frame.
constexpr float kRadiusSteps = 16.0f;

// Sent as the w of the caster frustum's near plane, so no point is ever behind it, and as
// the split depth of spot maps, so their only cascade covers every depth.
constexpr float kNoPlaneDistance = 1e30f;

// A cone narrower than this would need an infinite projection scale.
constexpr float kMinSpotHalfAngle = 0.01f;

// Must match PushConstants in shadow.hlsl.
struct ShadowPushConstants
{
  Matrix4 modelViewProjection;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 padding[2];
};
static_assert(sizeof(ShadowPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

// Must match PushConstants in shadowmapview.hlsl.
struct ShadowMapViewPushConstants
{
  // Where each atlas is drawn, in pixels: x, y, width, height. A width of 0 hides it.
  float directionalRect[4];
  float spotRect[4];
  uint32 directionalIndex;
  uint32 spotIndex;
  uint32 directionalColumns;
  uint32 directionalTileCount;
  uint32 spotColumns;
  uint32 spotTileCount;
  uint32 padding[2];
  float spotNearRatios[ShadowRenderer::MAX_SPOT_SHADOWS];
};
static_assert(sizeof(ShadowMapViewPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

// Space between the atlases of the debug view and around them, in pixels.
constexpr float kDebugViewMargin = 8.0f;

NODISCARD FORCEINLINE uint32
getCascadeColumns(uint32 cascadeCount)
{
  return cascadeCount > 1 ? 2 : 1;
}

NODISCARD FORCEINLINE uint32
getCascadeRows(uint32 cascadeCount)
{
  return cascadeCount > 2 ? 2 : 1;
}

// Rows as close to a square as the count allows, wider than tall: 8 lights are 4x2.
NODISCARD uint32
getSpotRows(uint32 lightCount)
{
  uint32 rows = 1;
  while ((rows + 1) * (rows + 1) <= lightCount) {
    ++rows;
  }
  return rows;
}

// Fits an atlas inside a box of the screen, keeping its shape, centered.
void
fitRect(float boxX, float boxWidth, float boxHeight, float aspect, float outRect[4])
{
  float width = boxWidth;
  float height = width / aspect;
  if (height > boxHeight) {
    height = boxHeight;
    width = height * aspect;
  }
  outRect[0] = boxX + (boxWidth - width) * 0.5f;
  outRect[1] = kDebugViewMargin + (boxHeight - height) * 0.5f;
  outRect[2] = Math::max(width, 0.0f);
  outRect[3] = Math::max(height, 0.0f);
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
  m_debugVertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "shadowmapview");
  m_debugFragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "shadowmapview");

  for (Vector<const RenderItem*>& casters : m_cascadeCasters) {
    casters.reserve(256);
  }
  for (Vector<const RenderItem*>& casters : m_spotCasters) {
    casters.reserve(64);
  }
  m_spotCandidates.reserve(MAX_SPOT_SHADOWS);
  // Never grows past this, so the spans handed out stay valid until the next frame.
  m_shadowViews.reserve(1 + MAX_SPOT_SHADOWS);
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
  m_directionalAtlas.handle = {};
  m_spotAtlas.handle = {};
  m_spotViewCount = 0;
  if (!g_cvarShadows.get()) {
    return {};
  }

  for (const LightItem* light : view.lights) {
    if (light->type == LightType::Directional && light->bCastShadows) {
      addDirectionalPasses(graph, scene, view, *light);
      break;
    }
  }
  addSpotPasses(graph, scene, view);
  return m_shadowViews;
}

/*
 */
void
ShadowRenderer::addDirectionalPasses(RenderGraph& graph,
                                     const Scene& scene,
                                     const RenderView& view,
                                     const LightItem& light)
{
  m_directionalPipelines.update(m_vertexShader, m_maskedFragmentShader,
                                g_cvarShadowDepthBias.get(), g_cvarShadowSlopeBias.get());

  const uint32 cascadeCount = static_cast<uint32>(Math::clamp(
      g_cvarShadowCascades.get(), 1, static_cast<int32>(ShadowView::MAX_CASCADES)));
  const uint32 columns = getCascadeColumns(cascadeCount);
  const uint32 rows = getCascadeRows(cascadeCount);
  const uint32 tileSize = static_cast<uint32>(
      Math::clamp(g_cvarShadowMapSize.get(), kMinShadowMapSize, kMaxShadowMapSize));
  ShadowAtlas& atlas = m_directionalAtlas;
  atlas.update(tileSize, columns, rows);

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

  const RGTextureHandle texture = atlas.import(graph, "ShadowMap");

  m_directionalViewIndex = static_cast<uint32>(m_shadowViews.size());
  ShadowView& shadow = m_shadowViews.emplace_back();
  shadow.light = &light;
  shadow.texture = texture;
  shadow.textureIndex = atlas.view->getBindlessIndex();
  shadow.cascadeCount = cascadeCount;
  shadow.atlasScale =
      Vector2(1.0f / static_cast<float>(columns), 1.0f / static_cast<float>(rows));
  shadow.texelSize = Vector2(1.0f / static_cast<float>(atlas.width),
                             1.0f / static_cast<float>(atlas.height));
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
        camera, light.direction, fitStart, splitDepth, tileSize);
    scene.gatherRenderItems(projection.casterFrustum, m_cascadeCasters[i]);

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
      .setExecute([this](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        const ShadowView& directional = m_shadowViews[m_directionalViewIndex];
        for (uint32 i = 0; i < directional.cascadeCount; ++i) {
          recordTile(commandList, m_directionalPipelines.pipelines, m_directionalAtlas, i,
                     directional.cascades[i].viewProjection, m_cascadeCasters[i]);
        }
      });
}

/*
 */
void
ShadowRenderer::addSpotPasses(RenderGraph& graph, const Scene& scene, const RenderView& view)
{
  const uint32 maxLights = static_cast<uint32>(Math::clamp(
      g_cvarSpotShadowMaxLights.get(), 0, static_cast<int32>(MAX_SPOT_SHADOWS)));
  if (maxLights == 0) {
    return;
  }
  const float distance = Math::max(g_cvarSpotShadowDistance.get(), 0.0f);
  selectSpotShadows(view.camera.getPosition(), view.lights, maxLights, distance,
                    m_spotCandidates);
  if (m_spotCandidates.empty()) {
    return;
  }

  m_spotPipelines.update(m_vertexShader, m_maskedFragmentShader,
                         g_cvarSpotShadowDepthBias.get(), g_cvarSpotShadowSlopeBias.get());

  // The atlas is sized for the most lights allowed, not for this frame's, so lights coming
  // and going do not make it again.
  const uint32 rows = getSpotRows(maxLights);
  const uint32 columns = (maxLights + rows - 1) / rows;
  const uint32 tileSize = Math::min(
      static_cast<uint32>(
          Math::clamp(g_cvarSpotShadowMapSize.get(), kMinShadowMapSize, kMaxShadowMapSize)),
      kMaxSpotAtlasSize / columns);
  ShadowAtlas& atlas = m_spotAtlas;
  atlas.update(tileSize, columns, rows);
  const RGTextureHandle texture = atlas.import(graph, "SpotShadowMap");

  const Vector2 atlasScale(1.0f / static_cast<float>(columns),
                           1.0f / static_cast<float>(rows));
  const Vector2 texelSize(1.0f / static_cast<float>(atlas.width),
                          1.0f / static_cast<float>(atlas.height));
  const uint32 filterRadius = static_cast<uint32>(
      Math::clamp(g_cvarSpotShadowFilterRadius.get(), 0, kMaxFilterRadius));
  const float normalOffsetTexels = Math::max(g_cvarSpotShadowNormalOffset.get(), 0.0f);
  const float nearPlane = g_cvarSpotShadowNearPlane.get();
  const float fadeStart =
      distance * (1.0f - Math::clamp(g_cvarSpotShadowFade.get(), 0.0f, 1.0f));

  m_firstSpotViewIndex = static_cast<uint32>(m_shadowViews.size());
  m_spotViewCount = static_cast<uint32>(m_spotCandidates.size());
  for (uint32 i = 0; i < m_spotViewCount; ++i) {
    const SpotShadowCandidate& candidate = m_spotCandidates[i];
    const SpotShadowProjection projection =
        computeSpotProjection(*candidate.light, nearPlane, tileSize);
    scene.gatherRenderItems(projection.casterFrustum, m_spotCasters[i]);
    m_spotNearRatios[i] = projection.nearPlane / Math::max(candidate.light->range, 1e-3f);

    ShadowView& shadow = m_shadowViews.emplace_back();
    shadow.light = candidate.light;
    shadow.texture = texture;
    shadow.textureIndex = atlas.view->getBindlessIndex();
    shadow.cascadeCount = 1;
    shadow.atlasScale = atlasScale;
    shadow.texelSize = texelSize;
    shadow.filterRadius = filterRadius;
    shadow.lightFade = Math::clamp(
        (candidate.rangeDistance - fadeStart) / Math::max(distance - fadeStart, 1e-4f), 0.0f,
        1.0f);
    shadow.bPerspective = true;

    ShadowCascade& cascade = shadow.cascades[0];
    cascade.viewProjection = projection.viewProjection;
    cascade.atlasOffset = Vector2(static_cast<float>(i % columns) * atlasScale.x,
                                  static_cast<float>(i / columns) * atlasScale.y);
    cascade.splitDepth = kNoPlaneDistance;
    cascade.blendStart = kNoPlaneDistance;
    cascade.normalOffset = projection.texelWorldSizePerDistance * normalOffsetTexels;
  }

  graph.addPass("SpotShadowDepth")
      .writeDepth(texture)
      .setExecute([this](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        for (uint32 i = 0; i < m_spotViewCount; ++i) {
          recordTile(commandList, m_spotPipelines.pipelines, m_spotAtlas, i,
                     m_shadowViews[m_firstSpotViewIndex + i].cascades[0].viewProjection,
                     m_spotCasters[i]);
        }
      });
}

/*
 */
void
ShadowRenderer::addDebugPass(RenderGraph& graph, RGTextureHandle output)
{
  updateDebugPipeline(graph.getTextureDesc(output).format);

  RenderPassBuilder pass = graph.addPass("ShadowMapView");
  pass.writeColor(output, LoadOp::Clear, LinearColor(0.02f, 0.02f, 0.02f, 1.0f));
  if (m_directionalAtlas.handle.isValid()) {
    pass.read(m_directionalAtlas.handle);
  }
  if (m_spotAtlas.handle.isValid()) {
    pass.read(m_spotAtlas.handle);
  }
  pass.setExecute([this](RenderPassContext& context) {
    const float halfWidth = static_cast<float>(context.getWidth()) * 0.5f;
    const float boxWidth = halfWidth - kDebugViewMargin * 1.5f;
    const float boxHeight = static_cast<float>(context.getHeight()) - kDebugViewMargin * 2.0f;

    ShadowMapViewPushConstants pushConstants{};
    if (m_directionalAtlas.handle.isValid()) {
      fitRect(kDebugViewMargin, boxWidth, boxHeight,
              static_cast<float>(m_directionalAtlas.width) /
                  static_cast<float>(m_directionalAtlas.height),
              pushConstants.directionalRect);
      pushConstants.directionalIndex = m_directionalAtlas.view->getBindlessIndex();
      pushConstants.directionalColumns = m_directionalAtlas.columns;
      pushConstants.directionalTileCount = m_shadowViews[m_directionalViewIndex].cascadeCount;
    }
    if (m_spotAtlas.handle.isValid()) {
      fitRect(halfWidth + kDebugViewMargin * 0.5f, boxWidth, boxHeight,
              static_cast<float>(m_spotAtlas.width) / static_cast<float>(m_spotAtlas.height),
              pushConstants.spotRect);
      pushConstants.spotIndex = m_spotAtlas.view->getBindlessIndex();
      pushConstants.spotColumns = m_spotAtlas.columns;
      pushConstants.spotTileCount = m_spotViewCount;
      for (uint32 i = 0; i < m_spotViewCount; ++i) {
        pushConstants.spotNearRatios[i] = m_spotNearRatios[i];
      }
    }

    ICommandList& commandList = context.getCommandList();
    commandList.bindPipeline(*m_debugPipeline);
    commandList.pushConstants(&pushConstants, sizeof(pushConstants));
    commandList.draw(3);
  });
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
SpotShadowProjection
ShadowRenderer::computeSpotProjection(const LightItem& light, float nearPlane, uint32 mapSize)
{
  const float range = Math::max(light.range, 1e-3f);
  SpotShadowProjection projection;
  projection.nearPlane = Math::clamp(nearPlane, 1e-4f, range * 0.5f);

  const Radian halfAngle(
      Math::max(Math::acos(light.outerConeCos).valueRadian(), kMinSpotHalfAngle));
  const Vector3& up = Math::abs(light.direction.z) > 0.99f ? Vector3::FORWARD : Vector3::UP;
  const LookAtMatrix lightView(light.position, light.position + light.direction, up);
  projection.viewProjection =
      lightView * PerspectiveMatrix(halfAngle, 1.0f, 1.0f, projection.nearPlane, range);

  // The near plane goes back to pass through the light, which is where casters start.
  projection.casterFrustum = Frustum(projection.viewProjection);
  Plane& nearSide = projection.casterFrustum.planes[static_cast<SIZE_T>(FrustumSide::Near)];
  nearSide.w = nearSide.normal.dot(light.position);

  projection.texelWorldSizePerDistance =
      2.0f * Math::tan(halfAngle) / static_cast<float>(mapSize);
  return projection;
}

/*
 */
void
ShadowRenderer::selectSpotShadows(const Vector3& cameraPosition,
                                  Span<const LightItem* const> lights,
                                  uint32 maxCount,
                                  float maxDistance,
                                  Vector<SpotShadowCandidate>& outCandidates)
{
  outCandidates.clear();
  for (const LightItem* light : lights) {
    if (light->type != LightType::Spot || !light->bCastShadows) {
      continue;
    }
    const float centerDistance = cameraPosition.distance(light->position);
    const float rangeDistance = Math::max(centerDistance - light->range, 0.0f);
    if (rangeDistance >= maxDistance) {
      continue;
    }
    outCandidates.push_back({.light = light,
                             .score = light->range / Math::max(centerDistance, 1e-3f),
                             .rangeDistance = rangeDistance});
  }

  Algorithm::sort(outCandidates,
                  [](const SpotShadowCandidate& a, const SpotShadowCandidate& b) {
                    return a.score > b.score;
                  });
  if (outCandidates.size() > maxCount) {
    outCandidates.resize(maxCount);
  }
}

/*
 */
void
ShadowRenderer::ShadowAtlas::update(uint32 newTileSize, uint32 newColumns, uint32 rows)
{
  tileSize = newTileSize;
  columns = newColumns;
  const uint32 newWidth = newTileSize * newColumns;
  const uint32 newHeight = newTileSize * rows;
  if (texture != nullptr && width == newWidth && height == newHeight) {
    return;
  }

  // The view points at its texture, so it goes first.
  view.reset();
  texture = IGraphicsAPI::instance().createTexture(
      {.format = kShadowMapFormat,
       .width = newWidth,
       .height = newHeight,
       .usage = TextureUsage::DepthStencil | TextureUsage::Sampled});
  view = texture->createView();
  width = newWidth;
  height = newHeight;
  state = ResourceState::Undefined;
}

/*
 */
RGTextureHandle
ShadowRenderer::ShadowAtlas::import(RenderGraph& graph, const ANSICHAR* name)
{
  handle = graph.importTexture(name, *texture, *view, state, ResourceState::ShaderRead);
  state = ResourceState::ShaderRead;
  return handle;
}

/*
 */
void
ShadowRenderer::CasterPipelines::update(const SPtr<IShader>& vertexShader,
                                        const SPtr<IShader>& maskedFragmentShader,
                                        float newDepthBias,
                                        float newSlopeBias)
{
  if (pipelines.isBuilt() && newDepthBias == depthBias && newSlopeBias == slopeBias) {
    return;
  }

  // Pipelines are freed through the deferred deletion, so frames still on the GPU keep the
  // old ones; clearing keeps a slider drag from piling up one set per value.
  cache.clear();
  // Opaque casters only write depth, so they need no fragment shader. Depth clamp keeps the
  // casters between the light and the map's near plane, at depth 0.
  const GraphicsPipelineDesc pipelineDesc{.vertexShader = vertexShader,
                                          .vertexLayout = VertexStatic::getLayout(),
                                          .raster = {.depthBiasConstant = newDepthBias,
                                                     .depthBiasSlope = newSlopeBias,
                                                     .depthClampEnable = true},
                                          .depthFormat = kShadowMapFormat};
  pipelines.build(cache, pipelineDesc, maskedFragmentShader);
  depthBias = newDepthBias;
  slopeBias = newSlopeBias;
}

/*
 */
void
ShadowRenderer::recordTile(ICommandList& commandList,
                           const MaterialPipelines& pipelines,
                           const ShadowAtlas& atlas,
                           uint32 tileIndex,
                           const Matrix4& lightViewProjection,
                           const Vector<const RenderItem*>& casters)
{
  const uint32 x = (tileIndex % atlas.columns) * atlas.tileSize;
  const uint32 y = (tileIndex / atlas.columns) * atlas.tileSize;
  const float tileSize = static_cast<float>(atlas.tileSize);
  commandList.setViewport(static_cast<float>(x), static_cast<float>(y), tileSize, tileSize);
  commandList.setScissor(x, y, atlas.tileSize, atlas.tileSize);

  ShadowPushConstants pushConstants{.modelViewProjection = Matrix4::IDENTITY,
                                    .materialIndex = 0,
                                    .samplerIndex = m_sampler->getBindlessIndex(),
                                    .padding = {0, 0}};
  pipelines.recordDraws(
      commandList, casters, m_defaultMaterial,
      [&commandList, &pushConstants, &lightViewProjection](const RenderItem& item,
                                                           const Material& material) {
        pushConstants.modelViewProjection = item.worldMatrix * lightViewProjection;
        pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
      });
}

/*
 */
void
ShadowRenderer::updateDebugPipeline(Format outputFormat)
{
  if (m_debugPipeline != nullptr && m_debugPipelineFormat == outputFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_debugVertexShader,
                                    .fragmentShader = m_debugFragmentShader,
                                    .raster = {.cullMode = CullMode::None},
                                    .depth = {.testEnable = false, .writeEnable = false},
                                    .colorAttachmentCount = 1};
  pipelineDesc.colorFormats[0] = outputFormat;
  m_debugPipeline = m_debugPipelineCache.getOrCreate(pipelineDesc).get();
  m_debugPipelineFormat = outputFormat;
}

} // namespace chEngineSDK
