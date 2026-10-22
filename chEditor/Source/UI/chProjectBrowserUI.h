/************************************************************************/
/**
 * @file chProjectBrowserUI.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Lists the recent projects and makes or opens one.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPath.h"

namespace chEngineSDK {

/**
 * Exists so the editor can pick its project: the window it shows when it starts without one
 * and File > Open Project draw the same recent list, Open and New Project. The choice is
 * opened by starting the editor again with -project, so a run only ever has one project.
 */
class ProjectBrowserUI
{
 public:
  /**
   * Opens the browser as a modal popup over the editor; renderPopup draws it.
   */
  static void
  open();

  static void
  renderPopup();

  /**
   * Draws the browser over the whole main window, for the run without a project.
   */
  static void
  renderWindow();

  /**
   * True once, with the .chproject file the user picked or made.
   */
  NODISCARD static bool
  takeChosenProject(Path& outProjectFile);

  /**
   * Puts projectFile first in this user's recent projects.
   */
  static void
  addRecentProject(const Path& projectFile);

  /**
   * The command line for the run that opens projectFile: this run's options, without its
   * -project, plus -project=<projectFile>.
   */
  NODISCARD static Vector<String>
  getLaunchArguments(const Path& projectFile);
};

} // namespace chEngineSDK
