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

#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * What the lit passes of a frame read from ImageBasedLighting. A handle is only valid on
 * the frame that wrote its texture, and a pass that reads the texture that frame must
 * declare the read, so the graph puts the barrier; on other frames the texture is already
 * in the ShaderRead state and the index alone is enough.
 */
struct ImageBasedLightingView
{
  RGTextureHandle brdfLut;
  uint32 brdfLutIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
};

/**
 * Exists so the lit paths can light surfaces with the environment without integrating it
 * every frame: it keeps the precomputed textures and adds the compute passes that build
 * them again, only on the frames their settings change. Today it holds the lookup table of
 * the split sum (scale and bias of the specular reflectance per NdotV and roughness), sized
 * by Renderer.IBLBrdfLutSize and integrated with Renderer.IBLBrdfLutSamples samples.
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

  NODISCARD ImageBasedLightingView
  addPasses(RenderGraph& graph);

 private:
  void
  addBrdfLutPass(RenderGraph& graph, ImageBasedLightingView& view);

  SPtr<IPipeline> m_brdfLutPipeline;
  SPtr<ITexture> m_brdfLut;
  SPtr<ITextureView> m_brdfLutView;
  // The settings the table was built with.
  uint32 m_brdfLutSize = 0;
  uint32 m_brdfLutSamples = 0;
};

} // namespace chEngineSDK
