/************************************************************************/
/**
 * @file chViewportGrid.h
 * @author AccelMR
 * @date 2026/10/16
 * @brief
 *  The ground grid of the editor viewport.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPipelineCache.h"
#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * Exists to show the ground plane (z = 0) in the viewport, so placement and scale can be
 * judged: lines every Editor.GridSpacing that grow ten times at a time with the distance when
 * Editor.GridAdaptive is on, as in Unreal, and the X and Y axes in red and green. It is
 * drawn over the finished image, behind what the scene drew in front of it, and is never
 * part of a game build.
 */
class ViewportGrid
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  ViewportGrid();

  ~ViewportGrid();

  /**
   * Adds nothing when the grid is off or there is no scene depth (view modes without one).
   * The camera is read now, so it need not outlive the frame.
   */
  void
  addPass(RenderGraph& graph,
          const Camera& camera,
          RGTextureHandle output,
          RGTextureHandle depth);

  NODISCARD static bool
  isVisible();

  /**
   * Saved to /Saved/Config/Editor.ini.
   */
  static void
  setVisible(bool bVisible);

  /**
   * The grid settings, for a popup or a menu; edits are saved once they end.
   */
  static void
  renderSettings();

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
