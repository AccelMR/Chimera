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

enum class ProjectSettingsSection : uint8
{
  Input,
  Packaging,
  COUNT
};

/**
 * Exists so the settings a project ships in its Config folder are edited in the editor
 * instead of by hand. It has one section per topic: Input edits the actions of
 * <Project>/Config/Input.ini, loading them again after every change so it also reaches a
 * running Play; Packaging edits what Config/Package.ini packages. Each saves at every change.
 * The window stays open between sessions.
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
   * Shows the window on that section.
   */
  static void
  showSection(ProjectSettingsSection section);

  /**
   * Cancels a key capture the window is waiting for, before the input manager stops.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
