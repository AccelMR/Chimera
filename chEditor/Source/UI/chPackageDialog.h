/************************************************************************/
/**
 * @file chPackageDialog.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Asks how to package the project and runs the packager.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so File > Package Project can name the game, its startup scene and where the
 * package goes, find a game build to copy (or offer the editor's own binaries as a
 * development package when there is none) and show how it went.
 */
class PackageDialog
{
 public:
  static void
  open();

  /**
   * True while a package is being written on its worker thread.
   */
  NODISCARD static bool
  isPackaging();

  /**
   * Draws the options while open and the progress of a running package. Call once per frame,
   * after the panels.
   */
  static void
  render();

  /**
   * Cancels a running package and waits for its worker.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
