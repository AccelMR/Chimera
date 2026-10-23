/************************************************************************/
/**
 * @file chDirectoryWatcher.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Tells which files of a folder changed, as the operating system reports them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include "chPath.h"

namespace chEngineSDK {

enum class FileChangeType : uint8
{
  Added,
  Modified,
  Removed
};

struct FileChange
{
  // Relative to the watched folder, with '/' separators.
  Path path;
  FileChangeType type = FileChangeType::Modified;
};

/**
 * Exists so tools react to files changed outside the engine (a script saved in a code editor,
 * a texture exported again) without reading every file on a timer: the operating system
 * queues the changes and poll only takes what is waiting, without blocking or a thread. A
 * rename is reported as the old name removed and the new one added; one save can bring
 * several changes of the same file. Each platform has its own source file.
 */
class CH_UTILITY_EXPORT DirectoryWatcher
{
 public:
  DirectoryWatcher();

  ~DirectoryWatcher();

  DirectoryWatcher(const DirectoryWatcher&) = delete;

  DirectoryWatcher&
  operator=(const DirectoryWatcher&) = delete;

  /**
   * Starts watching an existing folder (virtual or real), and with bRecursive every folder
   * under it, also those made later. Stops what was watched before. False (logged) when the
   * system cannot watch it.
   */
  NODISCARD bool
  start(const Path& directory, bool bRecursive);

  void
  stop();

  NODISCARD bool
  isWatching() const noexcept;

  /**
   * Appends the changes reported since the last call. False when the system dropped some
   * (too many at once): anything under the folder may have changed.
   */
  NODISCARD bool
  poll(Vector<FileChange>& outChanges);

  /**
   * The real folder being watched.
   */
  NODISCARD FORCEINLINE const Path&
  getDirectory() const noexcept { return m_directory; }

 private:
  struct Impl;

  UniquePtr<Impl> m_impl;
  Path m_directory;
};

} // namespace chEngineSDK
