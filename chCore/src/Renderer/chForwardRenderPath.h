/************************************************************************/
/**
 * @file chForwardRenderPath.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws the scene items textured, in one pass, straight into the output.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIRenderPath.h"
#include "chPipelineCache.h"

namespace chEngineSDK {

/**
 * The simplest way to see the scene: one pass with a depth target that draws every item
 * with its texture and no lighting, until the deferred path exists.
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
  recordDraws(RenderPassContext& context, const Camera& camera,
              Span<const RenderItem* const> items);

  void
  updatePipeline(Format colorFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Looked up again only when the output format changes, because building the description
  // allocates (VertexLayout).
  const IPipeline* m_pipeline = nullptr;
  Format m_pipelineFormat = Format::Unknown;

  // Camera matrices read by the shader through the bindless heap. One per frame in flight,
  // because the CPU writes them while the GPU may still read the previous frame's.
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_cameraBuffers;

  // 1x1 white, for items without a texture.
  SPtr<ITexture> m_defaultTexture;
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
