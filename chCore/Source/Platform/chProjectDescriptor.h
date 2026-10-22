/************************************************************************/
/**
 * @file chProjectDescriptor.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  What a .chproject file says about its project.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPath.h"

namespace chEngineSDK {

/**
 * Exists so the .chproject file can say what a project is before any config is read: its
 * name, the engine version it was saved with and the game code it loads. Settings stay in
 * the Config .ini files, so a player or a project can change them without touching it.
 *
 * The file is an .ini:
 *   [Project] Name, EngineVersion, Description
 *   [Modules] GameModules (comma list of game code plugins)
 * An empty file is a valid project named after the file.
 */
struct CH_CORE_EXPORT ProjectDescriptor
{
  /**
   * Reads file. A missing key keeps its default, and a missing name becomes the file name.
   * False when the file cannot be read.
   */
  NODISCARD bool
  load(const Path& file);

  NODISCARD bool
  save(const Path& file) const;

#if USING(CH_EDITOR)
  /**
   * Makes <parentDirectory>/<name> with <name>.chproject, Assets and Config, refusing a
   * folder that already exists and is not empty. Writes the project file's path into
   * outProjectFile and, on failure, the reason into outError.
   */
  NODISCARD static bool
  create(const Path& parentDirectory,
         StringView name,
         Path& outProjectFile,
         String& outError);
#endif // USING(CH_EDITOR)

  /**
   * The .chproject file directly inside directory; empty when there is none.
   */
  NODISCARD static Path
  findProjectFile(const Path& directory);

  /**
   * True for a name a project folder and file can have: not empty, no path separators and
   * none of the characters Windows forbids in file names.
   */
  NODISCARD static bool
  isValidName(StringView name);

  String name{};
  // CH_ENGINE_VERSION_STRING of the engine that wrote the file.
  String engineVersion{};
  String description{};
  // Base names of the plugins with the project's game code, loaded like any plugin.
  Vector<String> gameModules{};
};

} // namespace chEngineSDK
