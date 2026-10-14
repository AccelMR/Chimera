/************************************************************************/
/**
 * @file chDeferredRenderPath.h
 * @author AccelMR
 * @date 2026/10/13
 * @brief Draws the scene surfaces into a GBuffer, then lights every pixel once.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIRenderPath.h"
#include "chMaterial.h"
#include "chMaterialPipelines.h"
#include "chPipelineCache.h"
#include "chRenderSettings.h"
#include "chSceneViewBuffers.h"

namespace chEngineSDK {

/**
 * Exists so the cost of the lights does not grow with how many fragments the items draw:
 * a GBuffer pass writes the material of the nearest surface of each pixel, and a full
 * screen pass evaluates every light once per pixel with the same BRDF as the forward path.
 * The GBuffer can also be shown channel by channel (the GBuffer view modes).
 */
class CH_CORE_EXPORT DeferredRenderPath : public IRenderPath
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  DeferredRenderPath();

  ~DeferredRenderPath() override;

  RGTextureHandle
  addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output) override;

  /**
   * Draws the GBuffer and shows the channel of a GBuffer view mode (BaseColor to Emissive)
   * in output, encoded for a back buffer.
   *
   * @return the scene depth.
   */
  RGTextureHandle
  addGBufferViewPasses(RenderGraph& graph,
                       const RenderView& view,
                       RGTextureHandle output,
                       ViewMode mode);

 private:
  struct GBuffer
  {
    RGTextureHandle baseColor;
    RGTextureHandle normal;
    RGTextureHandle surface;
    RGTextureHandle emissive;
    RGTextureHandle depth;
  };

  struct FullscreenPipeline
  {
    SPtr<IShader> vertexShader;
    SPtr<IShader> fragmentShader;
    const IPipeline* pipeline = nullptr;
    Format colorFormat = Format::Unknown;
  };

  NODISCARD GBuffer
  addGBufferPass(RenderGraph& graph,
                 const RenderView& view,
                 uint32 viewIndex,
                 uint32 width,
                 uint32 height);

  void
  recordGBufferDraws(RenderPassContext& context,
                     Span<const RenderItem* const> items,
                     uint32 viewIndex);

  /**
   * Builds the GBuffer pipeline the first time; its formats never change.
   */
  void
  updateGBufferPipeline();

  void
  updateFullscreenPipeline(FullscreenPipeline& fullscreen, Format colorFormat);

  SPtr<IShader> m_gbufferVertexShader;
  SPtr<IShader> m_gbufferFragmentShader;
  SPtr<IShader> m_gbufferMaskedFragmentShader;
  MaterialPipelines m_gbufferPipelines;
  FullscreenPipeline m_lighting;
  FullscreenPipeline m_gbufferView;
  PipelineCache m_pipelineCache;

  SceneViewBuffers m_viewBuffers;

  // For items without a material.
  Material m_defaultMaterial;
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
