/************************************************************************/
/**
 * @file chForwardRenderPath.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws the scene items lit by the scene lights, in one pass, into the output.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIRenderPath.h"
#include "chMaterial.h"
#include "chMaterialPipelines.h"
#include "chPipelineCache.h"
#include "chSceneViewBuffers.h"

namespace chEngineSDK {

/**
 * The simplest way to light the scene: one pass with a depth target that draws every item
 * with its material and evaluates every light per pixel. Costs grow with lights times
 * pixels drawn, which the deferred path avoids.
 */
class CH_CORE_EXPORT ForwardRenderPath : public IRenderPath
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  ForwardRenderPath();

  ~ForwardRenderPath() override;

  RGTextureHandle
  addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output) override;

 private:
  void
  recordDraws(RenderPassContext& context,
              Span<const RenderItem* const> items,
              uint32 viewIndex);

  void
  updatePipeline(Format colorFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  SPtr<IShader> m_maskedFragmentShader;
  PipelineCache m_pipelineCache;
  // Looked up again only when the output format changes, because building the description
  // allocates (VertexLayout).
  MaterialPipelines m_pipelines;
  Format m_pipelineFormat = Format::Unknown;

  SceneViewBuffers m_viewBuffers;

  // For items without a material.
  Material m_defaultMaterial;
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
