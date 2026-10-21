/************************************************************************/
/**
 * @file chTonemapPass.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Turns the HDR scene color into the colors the screen shows.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPipelineCache.h"
#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * Exists because lighting works in linear values without an upper limit, while the screen
 * takes [0, 1] in sRGB: it applies the exposure (Renderer.Exposure), the ACES curve and the
 * sRGB encoding, in one pass shared by every render path.
 */
class CH_CORE_EXPORT TonemapPass
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  TonemapPass();

  ~TonemapPass();

  /**
   * Reads sceneColor and overwrites output, which must have the same size.
   */
  void
  addPass(RenderGraph& graph, RGTextureHandle sceneColor, RGTextureHandle output);

 private:
  void
  updatePipeline(Format outputFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Looked up again only when the output format changes.
  const IPipeline* m_pipeline = nullptr;
  Format m_pipelineFormat = Format::Unknown;
};

} // namespace chEngineSDK
