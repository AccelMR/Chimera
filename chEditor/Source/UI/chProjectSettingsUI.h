/************************************************************************/
/**
 * @file chProjectSettingsUI.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief Window that edits the settings of the open project.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so the settings a project ships in its Config folder are edited in the editor
 * instead of by hand. It has one section per topic; the first one edits the input actions of
 * <Project>/Config/Input.ini, saving at every change and loading the actions again, so a
 * change also reaches a running Play. The window stays open between sessions.
 */
class ProjectSettingsUI
{
 public:
  static void
  render();

  NODISCARD static bool
  isWindowVisible();

  /**
   * Showing it reads the files again and brings the window to the front.
   */
  static void
  setWindowVisible(bool bVisible);

  /**
   * Cancels a key capture the window is waiting for, before the input manager stops.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
