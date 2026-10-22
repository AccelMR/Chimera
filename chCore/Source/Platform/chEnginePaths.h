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

#include "chProjectDescriptor.h"

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
 * that has chCore/Content. A packaged game instead has a .chproject file and an Engine
 * folder next to the executable: the executable folder is then both the engine root and
 * the project, and /Engine is that Engine folder. The project is the folder given with
 * -project=<path> (a folder with a .chproject file, or the file itself). Without it a
 * game uses the executable folder, and the editor opens no project: /Game is not mounted
 * and /Saved is the user's editor folder, until the user picks a project.
 * The project folder (else that user folder) is also the FileSystem base directory.
 */
class CH_CORE_EXPORT EnginePaths
{
 public:
  /**
   * Finds the folders and mounts them. Every getter calls it the first time, so it
   * only needs to be called directly to choose when it happens, for example before
   * opening the log file. Reads -project from CommandLine.
   * The project is, in order: -project; the executable folder when it holds a
   * .chproject file (a packaged game); defaultProject when it holds one; then none when
   * bProjectOptional (editor builds only; ignored without the editor), else the
   * executable folder.
   * Throws if the engine root or the requested project cannot be found.
   * Every path is worked out here once, so the references returned by the getters
   * stay valid until the program ends.
   */
  static void
  initialize(StringView defaultProject = StringView(), bool bProjectOptional = false);

  /**
   * False only when the program started without a project and allowed it (the editor
   * before the user picks one); then every project folder getter gives an empty path.
   */
  NODISCARD static bool
  hasProject();

  /**
   * The project's .chproject file; empty when the project has none.
   */
  NODISCARD static const Path&
  getProjectFile();

  NODISCARD static const ProjectDescriptor&
  getProjectDescriptor();

#if USING(CH_EDITOR)
  /**
   * Real folder of this user's editor data, shared by every project (the recent projects;
   * logs and config while no project is open). Made the first time it is asked for.
   */
  NODISCARD static const Path&
  getEditorUserDirectory();
#endif // USING(CH_EDITOR)

  NODISCARD static const Path&
  getEngineRootDirectory();

  /**
   * Real folder mounted as /Engine: <EngineRoot>/chCore/Content, or the Engine folder of
   * a package.
   */
  NODISCARD static const Path&
  getEngineContentDirectory();

  /**
   * True when the executable runs from a package made by the editor's Package Project.
   */
  NODISCARD static bool
  isPackaged();

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

  /**
   * Real folder of the compiled shaders, <exe folder>/Shaders, with one subfolder per format
   * (IGraphicsAPI::getShaderBinaryFormat). The build writes them there from the
   * Content/Shaders folders.
   */
  NODISCARD static const Path&
  getShaderBinaryDirectory();

  NODISCARD static const Path&
  getEditorContentDirectory();

  NODISCARD static const Path&
  getSavedDirectory();

  NODISCARD static const Path&
  getLogDirectory();

  /**
   * Virtual folder of the user's config, "/Saved/Config": what the program writes.
   */
  NODISCARD static const Path&
  getConfigDirectory();

  /**
   * Real folder of the project's config, <Project>/Config: what the project ships.
   */
  NODISCARD static const Path&
  getProjectConfigDirectory();

  NODISCARD static constexpr StringView
  getEngineAssetExtension() noexcept
  {
    return ".chAss";
  }

  static constexpr StringView PROJECT_EXTENSION = ".chproject";

  /**
   * Folder next to a packaged executable that holds the engine content.
   */
  static constexpr StringView PACKAGE_ENGINE_FOLDER = "Engine";
};

} // namespace chEngineSDK
