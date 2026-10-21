/************************************************************************/
/**
 * @file chScreenSpaceAO.h
 * @author AccelMR
 * @date 2026/10/16
 * @brief Measures how much nearby surfaces hide each pixel from the ambient light.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * Exists so the ambient light (sky light or constant ambient) does not reach creases,
 * corners and the ground under objects as if nothing were around them. From the depth and
 * the world normals of the GBuffer, a compute pass finds, in a few directions around each
 * pixel, the horizon that nearby surfaces make (ground truth ambient occlusion, as in
 * Intel's XeGTAO), then a depth aware blur removes its noise and, at half resolution, a
 * depth aware scale up brings it to full size. Settings come from Renderer.SSAO*.
 *
 * Needs a started IGraphicsAPI and must be destroyed before it shuts down.
 */
class CH_CORE_EXPORT ScreenSpaceAO
{
 public:
  ScreenSpaceAO();

  ~ScreenSpaceAO();

  ScreenSpaceAO(const ScreenSpaceAO&) = delete;

  ScreenSpaceAO&
  operator=(const ScreenSpaceAO&) = delete;

  /**
   * depth and normal are the GBuffer's; viewIndex is the bindless index of the frame's
   * ViewData, which gives the projection.
   *
   * @return an R8 texture of the size of depth, 1 where nothing occludes; invalid when
   *         Renderer.SSAO is off.
   */
  NODISCARD RGTextureHandle
  addPasses(RenderGraph& graph, RGTextureHandle depth, RGTextureHandle normal, uint32 viewIndex);

 private:
  NODISCARD RGTextureHandle
  addBlurPass(RenderGraph& graph,
              StringView name,
              RGTextureHandle input,
              RGTextureHandle depth,
              uint32 viewIndex,
              uint32 resolutionScale,
              int32 directionX,
              int32 directionY);

  NODISCARD RGTextureHandle
  addUpsamplePass(RenderGraph& graph,
                  RGTextureHandle input,
                  RGTextureHandle depth,
                  uint32 viewIndex);

  SPtr<IPipeline> m_occlusionPipeline;
  SPtr<IPipeline> m_blurPipeline;
  SPtr<IPipeline> m_upsamplePipeline;
};

} // namespace chEngineSDK
