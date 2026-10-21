/************************************************************************/
/**
 * @file chImageBasedLighting.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Precomputes the data the lit paths read to light surfaces with the environment.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chMatrix4.h"
#include "chRenderGraph.h"

namespace chEngineSDK {
struct SkyLightItem;

/**
 * What the lit passes of a frame read from ImageBasedLighting. A handle is only valid on
 * the frame that wrote its texture, and a pass that reads the texture that frame must
 * declare the read (declareReads), so the graph puts the barrier; on other frames the
 * texture is already in the ShaderRead state and the index alone is enough.
 */
struct ImageBasedLightingView
{
  /**
   * False without a sky light image (or with Renderer.IBL off): the constant ambient is
   * used then.
   */
  NODISCARD constexpr bool
  hasEnvironment() const noexcept
  {
    return irradianceIndex != GraphicsLimits::INVALID_BINDLESS_INDEX;
  }

  void
  declareReads(RenderPassBuilder& pass) const
  {
    for (const RGTextureHandle texture : {brdfLut, irradiance, prefiltered}) {
      if (texture.isValid()) {
        pass.read(texture);
      }
    }
  }

  RGTextureHandle brdfLut;
  RGTextureHandle irradiance;
  RGTextureHandle prefiltered;
  uint32 brdfLutIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 irradianceIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 prefilteredIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 samplerIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  float intensity = 0.0f;
  // Mip of the prefiltered cube that roughness 1 reads.
  float prefilteredMaxMip = 0.0f;
};

/**
 * Exists so the lit paths can light surfaces with the environment without integrating it
 * every frame: it keeps the precomputed textures and adds the compute passes that build
 * them again only when their source or settings change. From the sky light's
 * equirectangular image it builds an environment cube in world space (turned by the sky
 * light's rotation) with its mip chain, then the irradiance cube (diffuse light) and the
 * prefiltered cube (reflections, one roughness per mip), plus once the lookup table of the
 * split sum. The sky light's intensity is applied when the lighting reads them, so changing
 * it builds nothing. Sizes and samples come from the Renderer.IBL* settings.
 *
 * Needs a started IGraphicsAPI and must be destroyed before it shuts down.
 */
class CH_CORE_EXPORT ImageBasedLighting
{
 public:
  ImageBasedLighting();

  ~ImageBasedLighting();

  ImageBasedLighting(const ImageBasedLighting&) = delete;

  ImageBasedLighting&
  operator=(const ImageBasedLighting&) = delete;

  /**
   * skyLight may be null. It is read now, so it need not outlive the frame.
   */
  NODISCARD ImageBasedLightingView
  addPasses(RenderGraph& graph, const SkyLightItem* skyLight);

 private:
  /**
   * A cube texture with a view that samples every mip and one storage view per mip.
   */
  struct CubeTexture
  {
    SPtr<ITexture> texture;
    SPtr<ITextureView> view;
    Vector<SPtr<ITextureView>> mipViews;
    uint32 size = 0;
    uint32 mipCount = 0;
  };

  /**
   * The settings and source the cubes were built from; any change builds them again.
   */
  struct BuildKey
  {
    bool
    operator==(const BuildKey& other) const = default;

    const ITexture* sky = nullptr;
    Matrix4 worldToSky = Matrix4::IDENTITY;
    uint32 cubeSize = 0;
    uint32 irradianceSize = 0;
    uint32 irradianceSamples = 0;
    uint32 prefilterSize = 0;
    uint32 prefilterMips = 0;
    uint32 prefilterSamples = 0;
  };

  void
  addBrdfLutPass(RenderGraph& graph, ImageBasedLightingView& view);

  void
  addEnvironmentPasses(RenderGraph& graph, const BuildKey& key, ImageBasedLightingView& view);

  /**
   * Makes the cube again when its size or mip count changed.
   */
  static void
  updateCube(CubeTexture& cube, uint32 size, uint32 mipCount);

  SPtr<IPipeline> m_brdfLutPipeline;
  SPtr<IPipeline> m_environmentPipeline;
  SPtr<IPipeline> m_downsamplePipeline;
  SPtr<IPipeline> m_irradiancePipeline;
  SPtr<IPipeline> m_prefilterPipeline;
  // Linear with mips and clamped, for every lookup of the lighting.
  SPtr<ISampler> m_sampler;
  // The sky image wraps around its longitude.
  SPtr<ISampler> m_skySampler;

  SPtr<ITexture> m_brdfLut;
  SPtr<ITextureView> m_brdfLutView;
  // The settings the table was built with.
  uint32 m_brdfLutSize = 0;
  uint32 m_brdfLutSamples = 0;

  CubeTexture m_environment;
  CubeTexture m_irradiance;
  CubeTexture m_prefiltered;
  // What the cubes hold; a null sky means they hold nothing yet.
  BuildKey m_builtKey;
};

} // namespace chEngineSDK
