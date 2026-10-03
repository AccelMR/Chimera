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
   * opening the log file. Reads -project from CommandLine.
   * Throws if the engine root or the requested project cannot be found.
   * Every path is worked out here once, so the references returned by the getters
   * stay valid until the program ends.
   */
  static void
  initialize();

  NODISCARD static const Path&
  getEngineRootDirectory();

  NODISCARD static const Path&
  getProjectDirectory();

  /**
   * Real folder of the graphics API and other plugins: the executable folder.
   */
  NODISCARD static const Path&
  getPluginDirectory();

  /**
   * Real folder of the asset codec plugins.
   */
  NODISCARD static const Path&
  getCodecDirectory();

  /**
   * Virtual folder of the project assets, "/Game".
   */
  NODISCARD static const Path&
  getGameAssetDirectory();

  /**
   * Real folder where new project assets are written, <Project>/Assets.
   */
  NODISCARD static const Path&
  getAbsoluteGameAssetDirectory();

  NODISCARD static const Path&
  getEngineAssetDirectory();

  NODISCARD static const Path&
  getEngineShaderDirectory();

  NODISCARD static const Path&
  getEditorContentDirectory();

  NODISCARD static const Path&
  getSavedDirectory();

  NODISCARD static const Path&
  getLogDirectory();

  NODISCARD static const Path&
  getConfigDirectory();

  NODISCARD static constexpr StringView
  getEngineAssetExtension() noexcept
  {
    return ".chAss";
  }
};

} // namespace chEngineSDK
