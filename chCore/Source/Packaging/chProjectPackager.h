/************************************************************************/
/**
 * @file chProjectPackager.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Writes a folder that plays a project without the editor or the engine sources.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chPackageRules.h"
#include "chPath.h"
#include "chSTDThreading.h"
#include "chUUID.h"

namespace chEngineSDK {

/**
 * What one package is made from. Every path is a real folder or file on disk.
 */
struct PackageDesc
{
  // Name of the executable and the .chproject file of the package.
  String gameName{};
  // Folder that becomes the package. A previous package there is replaced; any other folder
  // that is not empty is refused.
  Path outputDirectory{};
  // Build folder that holds the game executable, its shared libraries and Shaders.
  Path binaryDirectory{};
  // Engine content (/Engine). Everything but its shader sources goes into the package.
  Path engineContentDirectory{};
  // Project folder: its Config files, except the editor's, go into the package.
  Path projectDirectory{};
  // Asset files under <projectDirectory>/Assets, copied to the same place in the package.
  Vector<Path> assetFiles{};
  // Written as Game.StartupScene in the package's Config/Game.ini.
  String startupScene{};
};

struct PackageResult
{
  bool bSucceeded = false;
  bool bCancelled = false;
  // Why it failed, for the user.
  String error;
  uint32 binaryFileCount = 0;
  uint32 assetFileCount = 0;
};

/**
 * Read by another thread while package runs on its own.
 */
struct PackageProgress
{
  Atomic<uint32> copiedFiles{0};
  // Zero until the files to copy are listed.
  Atomic<uint32> totalFiles{0};
  // Checked before every file; a cancelled package deletes its output folder.
  Atomic<bool> bCancelRequested{false};
};

/**
 * Exists so a project can leave the editor: it copies the game runtime of a build, the
 * engine content it reads, the project's config and the assets a game needs into a folder
 * that runs alone, with the runtime renamed to the game. The runtime finds that layout
 * through EnginePaths. It never builds anything; the binaries come from a finished build.
 */
class CH_CORE_EXPORT ProjectPackager
{
 public:
  /**
   * Copies everything described. Stops at the first failure; what was copied until then
   * stays in the output folder. Touches only the disk, so it may run on any thread; progress
   * may be null.
   */
  NODISCARD static PackageResult
  package(const PackageDesc& desc, PackageProgress* progress = nullptr);

  /**
   * True when the folder holds the game executable of a build.
   */
  NODISCARD static bool
  isGameBinaryDirectory(const Path& directory);

  /**
   * True when the folder holds the debug engine library (its name ends in "d"), so the
   * build was made in Debug.
   */
  NODISCARD static bool
  isDebugBuild(const Path& directory);

  /**
   * True for a name the package can give its executable and project file: not empty, no
   * path separators and none of the characters Windows forbids in file names.
   */
  NODISCARD static bool
  isValidGameName(StringView name);

  /**
   * Fills outAssets with the roots and every asset they reach through getReferences, each
   * once, the first root first. getReferences returns null for an asset that does not
   * exist; such assets go to outMissing instead.
   */
  static void
  collectAssets(Span<const UUID> roots,
                const Function<const Vector<UUID>*(const UUID&)>& getReferences,
                Vector<UUID>& outAssets,
                Vector<UUID>& outMissing);

  /**
   * Every root of content and where its package rules are: the project's /Game today. A
   * plugin with content will add its own, with its own Config/Package.ini.
   */
  NODISCARD static Vector<PackageContentSource>
  getContentSources();

  static constexpr StringView GAME_EXECUTABLE_NAME = "chGame";

#if USING(CH_PLATFORM_WIN32)
  static constexpr StringView EXECUTABLE_EXTENSION = ".exe";
#else
  static constexpr StringView EXECUTABLE_EXTENSION = "";
#endif
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
