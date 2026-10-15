/************************************************************************/
/**
 * @file chProfilerUI.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Profiler window and stats overlay of the editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

struct RenderFrameStats;

/**
 * Exists so the cost of a frame can be seen while the editor runs: a window with the frame
 * time graph, the CPU scope tree and what every render pass drew, and a small overlay in
 * the corner of the viewport with the main numbers. Both are kept open between sessions;
 * the profiler only measures while one of them is shown.
 */
class ProfilerUI
{
 public:
  /**
   * stats is the last frame the scene renderer drew.
   */
  static void
  render(const RenderFrameStats& stats);

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);

  NODISCARD static bool
  isOverlayVisible();

  static void
  setOverlayVisible(bool bVisible);
};

} // namespace chEngineSDK
