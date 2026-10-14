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
#include "chMaterial.h"
#include "chPipelineCache.h"

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
  // Must match LightData in chLights.hlsli.
  struct GpuLight
  {
    float position[3];
    uint32 type;
    float direction[3];
    float range;
    float color[3];
    float spotScale;
    float spotOffset;
    float padding[3];
  };
  static_assert(sizeof(GpuLight) == 64);

  void
  updateViewData(const RenderView& view);

  void
  recordDraws(RenderPassContext& context, Span<const RenderItem* const> items);

  void
  updatePipeline(Format colorFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Looked up again only when the output format changes, because building the description
  // allocates (VertexLayout).
  const IPipeline* m_pipeline = nullptr;
  Format m_pipelineFormat = Format::Unknown;

  // Camera and light data read by the shader through the bindless heap. One per frame in
  // flight, because the CPU writes them while the GPU may still read the previous frame's.
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_viewBuffers;
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBuffers;
  // Lights each buffer of m_lightBuffers holds; a buffer is made again, bigger, when a
  // frame has more.
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBufferCapacities{};
  // Kept between frames, so converting the lights does not allocate.
  Vector<GpuLight> m_gpuLights;

  // For items without a material.
  Material m_defaultMaterial;
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
