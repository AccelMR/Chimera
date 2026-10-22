/************************************************************************/
/**
 * @file chFileSystem.h
 * @author AccelMR
 * @date 2022/06/27
 * @brief File system that is platform specific.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chFileStream.h"

namespace chEngineSDK{

/**
 * Single entry point to the disk for the whole engine, so no other file needs
 * <filesystem>. No function throws: failures return false (or an empty result)
 * and are logged.
 *
 * Paths can be virtual or real:
 * - A virtual path starts with a mount name, like "/Game/Textures/wood.chAss".
 *   Several directories can be mounted on the same name (for example a game and
 *   its mods). Reading uses the highest priority directory that has the file;
 *   writing uses the highest priority writable directory. Listing merges them.
 * - Any other path is a real path. Relative real paths are resolved against the
 *   base directory, not the working directory, so the engine finds the same files
 *   no matter where it is launched from. Until setBaseDirectory is called the base
 *   is the working directory.
 *
 * Mounts and the base directory are meant to be set at startup, before other
 * threads use FileSystem; changing them is not thread safe.
 */
class CH_UTILITY_EXPORT FileSystem
{
 public:
  /**
   *   Mounts a real directory under "/name". With the same priority, the directory
   *   mounted last wins.
   *
   * @param name
   *   Mount name, without '/'.
   * @param writable
   *   True if new files written under "/name" may go to this directory.
   **/
  static bool
  mount(const String& name, const Path& directory, int32 priority = 0, bool writable = false);

  static bool
  unmount(const String& name, const Path& directory);

  NODISCARD static bool
  isVirtual(const Path& path);

  /**
   *   Returns the virtual path for path, or an empty path if it is a real path
   *   outside every mount.
   **/
  NODISCARD static Path
  toVirtualPath(const Path& path);

  static void
  setBaseDirectory(const Path& directory);

  NODISCARD static Path
  getBaseDirectory();

  /**
   *   Returns the running executable file, asked to the operating system.
   **/
  NODISCARD static Path
  getExecutablePath();

  /**
   *   Returns the folder that holds the running executable, asked to the operating
   *   system.
   **/
  NODISCARD static Path
  getExecutableDirectory();

  /**
   *   Returns path relative to the base directory. A path outside the base
   *   directory starts with "..".
   **/
  NODISCARD static Path
  toRelativePath(const Path& path);

  /**
   *   Renames a regular file. Fails if oldPath is not a regular file.
   **/
  NODISCARD static bool
  renameFile(const Path& oldPath, const Path& newPath);

  /**
   *   Removes a regular file. Fails if path is not a regular file.
   **/
  NODISCARD static bool
  removeFile(const Path& path);

  /**
   *   Copies a regular file, creating every missing parent of destination and replacing
   *   a file already there.
   **/
  NODISCARD static bool
  copyFile(const Path& source, const Path& destination);

  /**
   *   Returns the real absolute path on disk, with "." and ".." removed. A virtual
   *   path gives the file in the layer that would be read.
   **/
  NODISCARD static Path
  absolutePath(const Path& path);

  /**
   *   Returns path written from basePath ("../Art/rock.png"), both resolved first like
   *   absolutePath. Empty when there is no such form (another drive on Windows).
   **/
  NODISCARD static Path
  relativePath(const Path& basePath, const Path& path);

  NODISCARD static bool
  isFile(const Path& path);

  NODISCARD static bool
  isDirectory(const Path& path);

  /**
   *   Returns true if path is basePath itself or is inside it. Virtual and real
   *   paths can be mixed, as can relative and absolute ones.
   **/
  NODISCARD static bool
  isSubPath(const Path& basePath, const Path& path);

  /**
   *   Creates a directory. Its parent must already exist.
   *
   * @return bool
   *   True if the directory exists after the call.
   **/
  static bool
  createDirectory(const Path& path);

  /**
   *   Creates a directory and every missing parent.
   *
   * @return bool
   *   True if the directory exists after the call.
   **/
  static bool
  createDirectories(const Path& path);

  NODISCARD static bool
  exists(const Path& path);

  /**
   *   Opens an existing file with the read point at its start. readOnly false opens it
   *   for reading and writing without emptying it.
   **/
  NODISCARD static SPtr<DataStream>
  openFile(const Path& path, bool readOnly = true);

  /**
   *   Creates a file, or empties it if it exists, with every missing parent directory.
   *   Returns nullptr if it could not be created.
   **/
  NODISCARD static SPtr<DataStream>
  createAndOpenFile(const Path& path);

  /**
   *   Writes the whole source stream, from its start, into a new or emptied file.
   **/
  NODISCARD static bool
  dumpMemStreamIntoFile(const SPtr<DataStream>& source, const Path& path);

  /**
   *   Removes a file or an empty directory. Fails on a directory that is not empty.
   **/
  static bool
  remove(const Path& path);

  /**
   *   Removes a file or a directory with everything inside it. Returns true if something
   *   was deleted.
   **/
  static bool
  removeAll(const Path& path);

  /**
   *   Reads a whole file. Empty if the file could not be read.
   **/
  NODISCARD static Vector<uint8>
  fastRead(const Path& path);

  /**
   *   Fills files and directories with the direct children of path. A virtual path
   *   gives virtual children merged from every layer; a real path gives absolute paths.
   **/
  static void
  getChildren(const Path& path, Vector<Path>& files, Vector<Path>& directories);

  /**
   *   Calls func for every file and directory under path, at any depth. Symbolic
   *   links to directories are not followed, so link cycles cannot loop forever.
   **/
  static void
  forEachFileChildRecursive(const Path& path, const Function<void(const Path&)>& func);
};
}
