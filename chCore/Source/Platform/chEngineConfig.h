/************************************************************************/
/**
 * @file chEngineConfig.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief Loads the engine, game and editor .ini files.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chConfigFile.h"

namespace chEngineSDK {

/**
 * Loads the config files once at start up and hands their values to the console
 * variables, so settings can change without rebuilding.
 *
 * Files: Engine.ini, Game.ini and, in editor builds, Editor.ini. Each one is read from
 * three folders, each one over the one before:
 *   /Engine/Config (/Editor/Config for Editor.ini)  defaults shipped with the engine
 *   <Project>/Config                                settings of the project
 *   /Saved/Config                                   settings of this user and machine
 * Every file is optional. An entry Key=Value in [Section] sets the console variable
 * "Section.Key"; the command line goes over all of them.
 */
class CH_CORE_EXPORT EngineConfig
{
 public:
  /**
   * Needs EnginePaths. Runs once; later calls do nothing.
   */
  static void
  initialize();

  /**
   * The merged layers of a file, by name without extension ("Engine", "Game", "Editor"),
   * for settings that are not console variables. An unknown name gives an empty file.
   */
  NODISCARD static const ConfigFile&
  getFile(StringView name);

  /**
   * Writes one value into the user's copy of the file (/Saved/Config/<fileName>.ini),
   * keeping the rest of that file. Console variables are not changed.
   */
  static bool
  setUserValue(StringView fileName, StringView section, StringView key, StringView value);
};

} // namespace chEngineSDK
