/************************************************************************/
/**
 * @file chDirectoryWatcherLinux.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  DirectoryWatcher on Linux, through inotify.
 */
/************************************************************************/
#include "chDirectoryWatcher.h"

#if USING(CH_PLATFORM_LINUX)

#include "chFileSystem.h"
#include "chLogger.h"

#include <cerrno>
#include <cstring>
#include <sys/inotify.h>
#include <unistd.h>

CH_LOG_DECLARE_STATIC(DirectoryWatcherLog, All);

namespace chEngineSDK {
namespace {
constexpr SIZE_T kBufferSize = 16 * 1024;
// Modifications are reported once the writer closes the file, so a file is never read
// while half written.
constexpr uint32 kWatchMask = IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_FROM |
                              IN_MOVED_TO | IN_ONLYDIR;

String
lastErrorText()
{
  return String(std::strerror(errno));
}
} // namespace

struct DirectoryWatcher::Impl
{
  int32 fd = -1;
  bool bRecursive = false;
  String root;
  // Folder of each watch relative to root: empty for root, else ending with '/'. inotify
  // watches one folder at a time, so a recursive watcher has one per folder.
  UnorderedMap<int32, String> folders;
  alignas(alignof(inotify_event)) uint8 buffer[kBufferSize];

  bool
  addWatch(const String& relativeFolder)
  {
    const String fullPath = relativeFolder.empty() ? root : root + "/" + relativeFolder;
    const int32 watch = inotify_add_watch(fd, fullPath.c_str(), kWatchMask);
    if (watch < 0) {
      CH_LOG_WARNING(DirectoryWatcherLog, "Cannot watch {0}: {1}", fullPath, lastErrorText());
      return false;
    }
    folders[watch] = relativeFolder;

    if (bRecursive) {
      Vector<Path> files;
      Vector<Path> directories;
      FileSystem::getChildren(Path(fullPath), files, directories);
      for (const Path& directory : directories) {
        // A folder that cannot be watched only misses its own changes.
        static_cast<void>(addWatch(relativeFolder + directory.getFileName() + "/"));
      }
    }
    return true;
  }

  /*
   * A folder moved away keeps its watches under the old name, so they go.
   */
  void
  removeWatchesUnder(const String& relativeFolder)
  {
    for (auto it = folders.begin(); it != folders.end();) {
      if (it->second.starts_with(relativeFolder)) {
        inotify_rm_watch(fd, it->first);
        it = folders.erase(it);
      }
      else {
        ++it;
      }
    }
  }

  void
  release()
  {
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
    folders.clear();
  }
};

/*
 */
DirectoryWatcher::DirectoryWatcher() = default;

/*
 */
DirectoryWatcher::~DirectoryWatcher()
{
  stop();
}

/*
 */
bool
DirectoryWatcher::start(const Path& directory, const bool bRecursive)
{
  stop();
  const Path realDirectory = FileSystem::absolutePath(directory);
  if (!FileSystem::isDirectory(realDirectory)) {
    CH_LOG_ERROR(DirectoryWatcherLog, "Cannot watch {0}: it is not a folder.", directory);
    return false;
  }

  UniquePtr<Impl> impl = chMakeUnique<Impl>();
  impl->bRecursive = bRecursive;
  impl->root = realDirectory.toString();
  impl->fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (impl->fd < 0) {
    CH_LOG_ERROR(DirectoryWatcherLog, "Cannot watch {0}: {1}", realDirectory, lastErrorText());
    return false;
  }
  if (!impl->addWatch(String())) {
    impl->release();
    return false;
  }
  m_impl = std::move(impl);
  m_directory = realDirectory;
  return true;
}

/*
 */
void
DirectoryWatcher::stop()
{
  if (m_impl) {
    m_impl->release();
    m_impl.reset();
  }
  m_directory = Path();
}

/*
 */
bool
DirectoryWatcher::isWatching() const noexcept
{
  return m_impl != nullptr;
}

/*
 */
bool
DirectoryWatcher::poll(Vector<FileChange>& outChanges)
{
  if (!m_impl) {
    return true;
  }
  Impl& impl = *m_impl;
  bool bComplete = true;
  for (;;) {
    // Non blocking: nothing waiting gives EAGAIN at once.
    const int64 length = static_cast<int64>(read(impl.fd, impl.buffer, kBufferSize));
    if (length <= 0) {
      if (length < 0 && errno != EAGAIN && errno != EINTR) {
        CH_LOG_ERROR(DirectoryWatcherLog, "Stopped reading changes of {0}: {1}", m_directory,
                     lastErrorText());
        bComplete = false;
      }
      break;
    }

    SIZE_T offset = 0;
    while (offset < static_cast<SIZE_T>(length)) {
      const auto* event = reinterpret_cast<const inotify_event*>(impl.buffer + offset);
      offset += sizeof(inotify_event) + event->len;
      if ((event->mask & IN_Q_OVERFLOW) != 0) {
        bComplete = false;
        continue;
      }
      const auto it = impl.folders.find(event->wd);
      if (it == impl.folders.end()) {
        continue;
      }
      // The folder of this watch is gone.
      if ((event->mask & IN_IGNORED) != 0) {
        impl.folders.erase(it);
        continue;
      }
      if (event->len == 0) {
        continue;
      }
      // Copied first: adding a watch below may move the map's entries.
      const String relative = it->second + event->name;

      const bool bAdded = (event->mask & (IN_CREATE | IN_MOVED_TO)) != 0;
      const bool bRemoved = (event->mask & (IN_DELETE | IN_MOVED_FROM)) != 0;
      if (impl.bRecursive && (event->mask & IN_ISDIR) != 0) {
        if (bAdded) {
          static_cast<void>(impl.addWatch(relative + "/"));
        }
        else if ((event->mask & IN_MOVED_FROM) != 0) {
          impl.removeWatchesUnder(relative + "/");
        }
      }
      FileChangeType type = FileChangeType::Modified;
      if (bAdded) {
        type = FileChangeType::Added;
      }
      else if (bRemoved) {
        type = FileChangeType::Removed;
      }
      outChanges.push_back(FileChange{.path = Path(relative), .type = type});
    }
  }
  return bComplete;
}

} // namespace chEngineSDK

#endif // USING(CH_PLATFORM_LINUX)
