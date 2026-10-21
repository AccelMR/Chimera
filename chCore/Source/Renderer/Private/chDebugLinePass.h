/************************************************************************/
/**
 * @file chDebugLinePass.h
 * @author AccelMR
 * @date 2026/10/18
 * @brief Draws the lines of DebugDraw over the image.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chPipelineCache.h"
#include "chRenderGraph.h"

#if USING(CH_DEBUG_DRAW)
namespace chEngineSDK {
class DebugDraw;

/**
 * Exists so debug lines need nothing from whoever draws them: every frame it copies the
 * lines of the given DebugDraws into a storage buffer of the frame slot (made bigger when
 * a frame has more) and draws them as a line list, those marked Tested against the scene
 * depth, the others on top.
 */
class CH_CORE_EXPORT DebugLinePass
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  DebugLinePass();

  ~DebugLinePass();

  /**
   * Draws the lines of every list (null entries are skipped) into output, after the
   * tonemap. Without a valid depth every line shows on top. Adds nothing when there are no
   * lines. The camera is read here, not when the graph runs.
   */
  void
  addPass(RenderGraph& graph,
          const Camera& camera,
          Span<const DebugDraw* const> lists,
          RGTextureHandle output,
          RGTextureHandle depth);

 private:
  struct PipelineEntry
  {
    const IPipeline* pipeline = nullptr;
    Format colorFormat = Format::Unknown;
    Format depthFormat = Format::Unknown;
  };

  /**
   * bTested lines compare with the depth; the others ignore it, even when it is bound.
   */
  NODISCARD const IPipeline&
  getPipeline(bool bTested, Format colorFormat, Format depthFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  PipelineEntry m_testedPipeline;
  PipelineEntry m_onTopPipeline;
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_vertexBuffers;
  // In vertices.
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_capacities{};
};

} // namespace chEngineSDK
#endif // USING(CH_DEBUG_DRAW)
