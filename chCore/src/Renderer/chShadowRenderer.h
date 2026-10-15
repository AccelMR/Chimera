/************************************************************************/
/**
 * @file chShadowRenderer.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Draws the shadow maps of a frame, before the lit render path reads them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chFrustum.h"
#include "chIRenderPath.h"
#include "chMaterial.h"
#include "chMaterialPipelines.h"
#include "chPipelineCache.h"

namespace chEngineSDK {

/**
 * Where a directional shadow map looks from, fitted to the part of the camera view it
 * covers.
 */
struct DirectionalShadowProjection
{
  // World to the shadow map: xy in NDC, z the depth 0..1.
  Matrix4 viewProjection;
  // The map's box without its near plane: every caster between the light and the covered
  // view, however far, can throw a shadow into it.
  Frustum casterFrustum;
  // Width of one texel in world units.
  float texelWorldSize;
};

/**
 * Exists so the lit render paths get their shadows without drawing them: it adds one depth
 * pass per shadow map to the frame's graph and hands the paths a ShadowView each, which
 * SceneViewBuffers sends to the lighting shaders. For now only the first directional light
 * that casts shadows gets a map, fitted to the camera view up to Renderer.ShadowDistance.
 *
 * The map is a texture it keeps and imports into each frame's graph, so its bindless index
 * is known while the graph is built, when the shadow data is written.
 *
 * Needs a started IGraphicsAPI and must be destroyed before it shuts down.
 */
class CH_CORE_EXPORT ShadowRenderer
{
 public:
  ShadowRenderer();

  ~ShadowRenderer();

  ShadowRenderer(const ShadowRenderer&) = delete;

  ShadowRenderer&
  operator=(const ShadowRenderer&) = delete;

  /**
   * Adds the shadow passes for the lights of the view. The result stays valid until the
   * next call; it is empty when shadows are off or no light casts them.
   */
  NODISCARD Span<const ShadowView>
  addPasses(RenderGraph& graph, const Scene& scene, const RenderView& view);

  /**
   * Fits a map of mapSize texels to the camera view from its near plane to distance (at
   * most its far plane). The map is a sphere around that part of the view, so turning the
   * camera does not change its size, and it moves in whole texels, so its edges do not
   * shimmer while the camera moves.
   */
  NODISCARD static DirectionalShadowProjection
  computeDirectionalProjection(const Camera& camera,
                               const Vector3& lightDirection,
                               float distance,
                               uint32 mapSize);

 private:
  /**
   * Makes the map again when Renderer.ShadowMapSize changed.
   */
  void
  updateShadowMap(uint32 size);

  void
  recordDraws(RenderPassContext& context);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_maskedFragmentShader;
  MaterialPipelines m_pipelines;
  PipelineCache m_pipelineCache;

  SPtr<ITexture> m_shadowMap;
  SPtr<ITextureView> m_shadowMapView;
  uint32 m_shadowMapSize = 0;
  // What the map was left in by the last frame that drew it.
  ResourceState m_shadowMapState = ResourceState::Undefined;

  // Kept between frames, so gathering does not allocate.
  Vector<const RenderItem*> m_casters;
  Vector<ShadowView> m_shadowViews;

  // For items without a material.
  Material m_defaultMaterial;
  // Reads the base color alpha of masked materials.
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
