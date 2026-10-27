/************************************************************************/
/**
 * @file chLevelsUI.h
 * @author AccelMR
 * @date 2026/10/26
 * @brief The sublevels of the open map, edited and tried in the editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so the sublevels a map lists can be picked without code: the Levels window shows
 * them with their "load at start" flag, takes scenes dragged from the Content Browser and
 * removes them, each change one undo step. During Play it shows which levels are loaded
 * (also those a script loaded) and loads or unloads them, as a script would.
 */
class LevelsUI
{
 public:
  /**
   * The Levels window, when shown.
   */
  static void
  render();

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);
};

} // namespace chEngineSDK
