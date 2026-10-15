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
 * Where a spot shadow map looks from: the light's cone seen through a square perspective
 * projection that reaches the light's range.
 */
struct SpotShadowProjection
{
  // World to the shadow map: xy in NDC, z the depth 0..1.
  Matrix4 viewProjection;
  // The map's volume with its near plane moved back to the light, so casters between the
  // light and the near plane are kept; the depth clamp draws them at depth 0.
  Frustum casterFrustum;
  // Width of one texel in world units, per unit of distance in front of the light.
  float texelWorldSizePerDistance;
  // The near plane used, after it was kept under half the range.
  float nearPlane;
};

/**
 * A spot light that may draw a shadow this frame, with what decides whether it does.
 */
struct SpotShadowCandidate
{
  const LightItem* light = nullptr;
  // Range over the distance from the camera to the light: about how much of the screen the
  // light can cover. Bigger wins.
  float score = 0.0f;
  // From the camera to the closest point of the light's range, 0 when the camera is inside.
  float rangeDistance = 0.0f;
};

/**
 * Exists so the lit render paths get their shadows without drawing them: it adds the depth
 * passes of the shadow maps to the frame's graph and hands the paths a ShadowView each,
 * which SceneViewBuffers sends to the lighting shaders. The first directional light that
 * casts shadows gets a map split into Renderer.ShadowCascades cascades along the camera
 * view up to Renderer.ShadowDistance; up to Renderer.SpotShadowMaxLights spot lights get
 * one map each.
 *
 * Each kind of light draws into its own atlas texture, one tile per map, so one pass draws
 * all of its maps, each tile with its own viewport. An atlas is a texture it keeps and
 * imports into each frame's graph, so its bindless index is known while the graph is
 * built, when the shadow data is written.
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
   * Adds a pass that shows the atlases the last addPasses drew over all of output: the
   * directional one on the left, the spot one on the right, nearer depths brighter.
   */
  void
  addDebugPass(RenderGraph& graph, RGTextureHandle output);

  /**
   * Fits a map of mapSize texels to the camera view between two distances in front of it
   * (clamped to its clip planes). The map is a sphere around that part of the view, so
   * turning the camera does not change its size, and it moves in whole texels, so its
   * edges do not shimmer while the camera moves.
   */
  NODISCARD static DirectionalShadowProjection
  computeDirectionalProjection(const Camera& camera,
                               const Vector3& lightDirection,
                               float nearDistance,
                               float farDistance,
                               uint32 mapSize);

  /**
   * Writes the view depth where each cascade ends, one per element of outSplits, the last
   * one at distance. lambda goes from even steps (0) to steps that grow in proportion to
   * the distance (1), which keep the size of a texel on screen about the same.
   */
  static void
  computeCascadeSplits(float nearPlane, float distance, float lambda, Span<float> outSplits);

  /**
   * The map of a spot light, from nearPlane in front of it to its range (the near plane is
   * kept under half the range).
   */
  NODISCARD static SpotShadowProjection
  computeSpotProjection(const LightItem& light, float nearPlane, uint32 mapSize);

  /**
   * Picks the spot lights with Cast Shadows whose range starts closer to the camera than
   * maxDistance, at most maxCount of them, the ones with the best score first.
   */
  static void
  selectSpotShadows(const Vector3& cameraPosition,
                    Span<const LightItem* const> lights,
                    uint32 maxCount,
                    float maxDistance,
                    Vector<SpotShadowCandidate>& outCandidates);

  static constexpr uint32 MAX_SPOT_SHADOWS = 16;

 private:
  /**
   * A depth texture with one tile per shadow map, made again when its size changes.
   */
  struct ShadowAtlas
  {
    void
    update(uint32 newTileSize, uint32 newColumns, uint32 rows);

    NODISCARD RGTextureHandle
    import(RenderGraph& graph, const ANSICHAR* name);

    SPtr<ITexture> texture;
    SPtr<ITextureView> view;
    uint32 width = 0;
    uint32 height = 0;
    // What the texture was left in by the last frame that drew it.
    ResourceState state = ResourceState::Undefined;
    // Width and height in texels of one tile.
    uint32 tileSize = 0;
    uint32 columns = 1;
    // Imported into this frame's graph; invalid when no map of this kind is drawn.
    RGTextureHandle handle;
  };

  /**
   * The caster pipelines of one kind of light, built again when its depth bias changes.
   */
  struct CasterPipelines
  {
    void
    update(const SPtr<IShader>& vertexShader,
           const SPtr<IShader>& maskedFragmentShader,
           float newDepthBias,
           float newSlopeBias);

    MaterialPipelines pipelines;
    PipelineCache cache;
    float depthBias = 0.0f;
    float slopeBias = 0.0f;
  };

  void
  addDirectionalPasses(RenderGraph& graph,
                       const Scene& scene,
                       const RenderView& view,
                       const LightItem& light);

  void
  addSpotPasses(RenderGraph& graph, const Scene& scene, const RenderView& view);

  /**
   * Draws the casters into one tile of the atlas being rendered.
   */
  void
  recordTile(ICommandList& commandList,
             const MaterialPipelines& pipelines,
             const ShadowAtlas& atlas,
             uint32 tileIndex,
             const Matrix4& lightViewProjection,
             const Vector<const RenderItem*>& casters);

  /**
   * Builds the debug view pipeline again when the output format changes.
   */
  void
  updateDebugPipeline(Format outputFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_maskedFragmentShader;
  CasterPipelines m_directionalPipelines;
  CasterPipelines m_spotPipelines;

  ShadowAtlas m_directionalAtlas;
  ShadowAtlas m_spotAtlas;

  // One list per cascade and per spot light, kept between frames, so gathering does not
  // allocate.
  Array<Vector<const RenderItem*>, ShadowView::MAX_CASCADES> m_cascadeCasters;
  Array<Vector<const RenderItem*>, MAX_SPOT_SHADOWS> m_spotCasters;
  Vector<SpotShadowCandidate> m_spotCandidates;
  Vector<ShadowView> m_shadowViews;
  // Where the views of this frame are in m_shadowViews.
  uint32 m_directionalViewIndex = 0;
  uint32 m_firstSpotViewIndex = 0;
  uint32 m_spotViewCount = 0;
  // Near plane over range of each spot map, which the debug view needs to show depth.
  Array<float, MAX_SPOT_SHADOWS> m_spotNearRatios{};

  SPtr<IShader> m_debugVertexShader;
  SPtr<IShader> m_debugFragmentShader;
  PipelineCache m_debugPipelineCache;
  const IPipeline* m_debugPipeline = nullptr;
  Format m_debugPipelineFormat = Format::Unknown;

  // For items without a material.
  Material m_defaultMaterial;
  // Reads the base color alpha of masked materials.
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
