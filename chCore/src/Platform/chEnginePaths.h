/************************************************************************/
/**
 * @file chEnginePaths.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This file contains the paths used by the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Finds where the engine and the open project live, and mounts them in FileSystem,
 * so the rest of the engine uses the same virtual paths in debug, release, the
 * editor and a packaged game:
 *   /Engine  engine content (read only)
 *   /Editor  editor content (read only, editor builds only)
 *   /Game    project assets
 *   /Saved   logs, config and cache of the project
 *
 * The engine root is the first folder, starting at the executable and going up,
 * that has chCore/Content. The project is the folder given with -project=<path>
 * (a folder with a .chproject file, or the file itself). Without it the editor
 * opens <EngineRoot>/Projects/Sandbox, and a game uses the executable folder.
 * The project folder is also the FileSystem base directory.
 */
class CH_CORE_EXPORT EnginePaths
{
 public:
  /**
   * Finds the folders and mounts them. Every getter calls it the first time, so it
   * only needs to be called directly to choose when it happens, for example before
   * opening the log file. Reads -project from CommandParser if it has started.
   * Throws if the engine root or the requested project cannot be found.
   */
  static void
  initialize();

  static Path
  getEngineRootDirectory();

  static Path
  getProjectDirectory();

  /**
   * Real folder of the graphics API and other plugins: the executable folder.
   */
  static Path
  getPluginDirectory();

  /**
   * Real folder of the asset codec plugins.
   */
  static Path
  getCodecDirectory();

  /**
   * Virtual folder of the project assets, "/Game".
   */
  static Path
  getGameAssetDirectory();

  /**
   * Real folder where new project assets are written, <Project>/Assets.
   */
  static Path
  getAbsoluteGameAssetDirectory();

  static Path
  getEngineAssetDirectory();

  static Path
  getEngineShaderDirectory();

  static Path
  getEditorContentDirectory();

  static Path
  getSavedDirectory();

  static Path
  getLogDirectory();

  static Path
  getConfigDirectory();

  static String
  getEngineAssetExtension();
};

} // namespace chEngineSDK
