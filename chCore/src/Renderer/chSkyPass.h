/************************************************************************/
/**
 * @file chSkyPass.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Draws the environment image of the sky light behind the scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPipelineCache.h"
#include "chRenderGraph.h"

namespace chEngineSDK {
struct SkyLightItem;

/**
 * Exists so every lit path shows the same background: after a path drew the scene, it
 * fills the pixels where nothing was drawn with the sky light's image, in the HDR scene
 * color, so the tonemap and exposure apply to it like to the scene.
 */
class CH_CORE_EXPORT SkyPass
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  SkyPass();

  ~SkyPass();

  /**
   * Adds nothing when the sky light has no image or does not show the sky. sceneColor
   * keeps what the path drew; depth is the path's scene depth. The camera and the sky
   * light are read now, so they need not outlive the frame.
   */
  void
  addPass(RenderGraph& graph,
          const Camera& camera,
          const SkyLightItem* skyLight,
          RGTextureHandle sceneColor,
          RGTextureHandle depth);

 private:
  void
  updatePipeline(Format outputFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  SPtr<ISampler> m_sampler;
  PipelineCache m_pipelineCache;
  // Looked up again only when the output format changes.
  const IPipeline* m_pipeline = nullptr;
  Format m_pipelineFormat = Format::Unknown;
};

} // namespace chEngineSDK
