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
constexpr int32 kMaxShadowMapSize = 8192;

// Receivers move this many texels along their normal before the test. Less leaves dark
// stripes on lit surfaces (shadow acne); more detaches the shadow from its caster.
constexpr float kNormalOffsetTexels = 1.5f;

// The slope term grows with how steeply a triangle faces away from the light, where one
// texel covers the biggest range of depths.
constexpr float kDepthBiasConstant = 1.0f;
constexpr float kDepthBiasSlope = 2.0f;

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

  // Opaque casters only write depth, so they need no fragment shader. Depth clamp keeps the
  // casters between the light and the map's near plane, at depth 0.
  const GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                          .vertexLayout = VertexStatic::getLayout(),
                                          .raster = {.depthBiasConstant = kDepthBiasConstant,
                                                     .depthBiasSlope = kDepthBiasSlope,
                                                     .depthClampEnable = true},
                                          .depthFormat = kShadowMapFormat};
  m_pipelines.build(m_pipelineCache, pipelineDesc, m_maskedFragmentShader);

  m_casters.reserve(256);
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

  const uint32 mapSize = static_cast<uint32>(
      Math::clamp(g_cvarShadowMapSize.get(), kMinShadowMapSize, kMaxShadowMapSize));
  updateShadowMap(mapSize);

  const Camera& camera = view.camera;
  const float distance = Math::clamp(g_cvarShadowDistance.get(), camera.getNearClipPlane(),
                                     camera.getFarClipPlane());
  const DirectionalShadowProjection projection =
      computeDirectionalProjection(camera, light->direction, distance, mapSize);
  scene.gatherRenderItems(projection.casterFrustum, m_casters);

  const RGTextureHandle texture =
      graph.importTexture("ShadowMap", *m_shadowMap, *m_shadowMapView, m_shadowMapState,
                          ResourceState::ShaderRead);
  m_shadowMapState = ResourceState::ShaderRead;

  m_shadowViews.push_back(
      {.viewProjection = projection.viewProjection,
       .light = light,
       .texture = texture,
       .textureIndex = m_shadowMapView->getBindlessIndex(),
       .texelSize = 1.0f / static_cast<float>(mapSize),
       .normalOffset = projection.texelWorldSize * kNormalOffsetTexels,
       .fadeDistance = distance});

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
                                             float distance,
                                             uint32 mapSize)
{
  // The covered part of the view goes from clip depth 0 to the depth of the point at
  // distance in front of the camera; its corners come back from clip space.
  const Matrix4& cameraViewProjection = camera.getViewProjectionMatrix();
  const Vector3 farPoint = camera.getPosition() + camera.getForwardVector() * distance;
  const Vector4 farClip = cameraViewProjection.transformVector4(
      Vector4(farPoint.x, farPoint.y, farPoint.z, 1.0f));
  const float depths[2] = {0.0f, farClip.z / farClip.w};
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
ShadowRenderer::updateShadowMap(uint32 size)
{
  if (m_shadowMap != nullptr && m_shadowMapSize == size) {
    return;
  }

  // The view points at its texture, so it goes first.
  m_shadowMapView.reset();
  m_shadowMap = IGraphicsAPI::instance().createTexture(
      {.format = kShadowMapFormat,
       .width = size,
       .height = size,
       .usage = TextureUsage::DepthStencil | TextureUsage::Sampled});
  m_shadowMapView = m_shadowMap->createView();
  m_shadowMapSize = size;
  m_shadowMapState = ResourceState::Undefined;
}

/*
 */
void
ShadowRenderer::recordDraws(RenderPassContext& context)
{
  ICommandList& commandList = context.getCommandList();
  const Matrix4& lightViewProjection = m_shadowViews[0].viewProjection;
  ShadowPushConstants pushConstants{.modelViewProjection = Matrix4::IDENTITY,
                                    .materialIndex = 0,
                                    .samplerIndex = m_sampler->getBindlessIndex(),
                                    .padding = {0, 0}};
  m_pipelines.recordDraws(
      commandList, m_casters, m_defaultMaterial,
      [&commandList, &pushConstants, &lightViewProjection](const RenderItem& item,
                                                           const Material& material) {
        pushConstants.modelViewProjection = item.worldMatrix * lightViewProjection;
        pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
      });
}

} // namespace chEngineSDK
