/************************************************************************/
/**
 * @file chIRenderPath.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Interface for the ways of drawing a scene view into a render graph.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLinearColor.h"
#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * What one camera sees in one frame. The camera and the items are read when the graph
 * runs, so they must stay alive until then.
 */
struct RenderView
{
  const Camera& camera;
  Span<const RenderItem* const> items;
  LinearColor clearColor = LinearColor::Black;
};

/**
 * Exists so the way a scene is drawn (forward, deferred, a debug view) can be swapped at
 * runtime: each path only adds its passes to the frame's graph, and the graph works out
 * the textures and barriers between them.
 */
class CH_CORE_EXPORT IRenderPath
{
 public:
  virtual ~IRenderPath() = default;

  /**
   * Adds the passes that draw the view into output, which they overwrite completely.
   */
  virtual void
  addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output) = 0;
};

} // namespace chEngineSDK
