/************************************************************************/
/**
 * @file chDirectoryWatcherWin32.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  DirectoryWatcher on Windows, through ReadDirectoryChangesW.
 */
/************************************************************************/
#include "chDirectoryWatcher.h"

#if USING(CH_PLATFORM_WIN32)

#include "chFileSystem.h"
#include "chLogger.h"
#include "chUnicode.h"
#include "chWindows.h"

CH_LOG_DECLARE_STATIC(DirectoryWatcherLog, All);

namespace chEngineSDK {
namespace {
// The most a network share accepts; also enough for a burst of local changes.
constexpr DWORD kBufferSize = 64 * 1024;
constexpr DWORD kNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                FILE_NOTIFY_CHANGE_LAST_WRITE;

FileChangeType
toChangeType(const DWORD action)
{
  switch (action) {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_RENAMED_NEW_NAME:
      return FileChangeType::Added;
    case FILE_ACTION_REMOVED:
    case FILE_ACTION_RENAMED_OLD_NAME:
      return FileChangeType::Removed;
    default:
      return FileChangeType::Modified;
  }
}
} // namespace

struct DirectoryWatcher::Impl
{
  HANDLE directory = INVALID_HANDLE_VALUE;
  HANDLE event = nullptr;
  OVERLAPPED overlapped{};
  bool bRecursive = false;
  bool bPending = false;
  // ReadDirectoryChangesW writes DWORD aligned records.
  alignas(DWORD) uint8 buffer[kBufferSize];

  /*
   * Asks for the next batch of changes; the call returns at once.
   */
  bool
  requestChanges()
  {
    overlapped = OVERLAPPED{};
    overlapped.hEvent = event;
    bPending = ReadDirectoryChangesW(directory, buffer, kBufferSize, bRecursive ? TRUE : FALSE,
                                     kNotifyFilter, nullptr, &overlapped, nullptr) != FALSE;
    return bPending;
  }

  void
  close()
  {
    if (bPending) {
      // The system writes into buffer until the request is really cancelled.
      CancelIoEx(directory, &overlapped);
      DWORD bytes = 0;
      GetOverlappedResult(directory, &overlapped, &bytes, TRUE);
      bPending = false;
    }
    if (directory != INVALID_HANDLE_VALUE) {
      CloseHandle(directory);
      directory = INVALID_HANDLE_VALUE;
    }
    if (event) {
      CloseHandle(event);
      event = nullptr;
    }
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
  impl->directory =
      CreateFileW(realDirectory.getPlatformString().c_str(), FILE_LIST_DIRECTORY,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  impl->event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (impl->directory == INVALID_HANDLE_VALUE || !impl->event || !impl->requestChanges()) {
    CH_LOG_ERROR(DirectoryWatcherLog, "Cannot watch {0} (error {1}).", realDirectory,
                 static_cast<uint32>(GetLastError()));
    impl->close();
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
    m_impl->close();
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
  // Read in user mode, so a frame without changes makes no system call.
  if (!m_impl || !m_impl->bPending || !HasOverlappedIoCompleted(&m_impl->overlapped)) {
    return true;
  }
  Impl& impl = *m_impl;
  DWORD bytes = 0;
  const bool bDone = GetOverlappedResult(impl.directory, &impl.overlapped, &bytes, FALSE) != FALSE;
  impl.bPending = false;

  // No bytes means the buffer was too small and the system dropped the changes.
  bool bComplete = bDone && bytes > 0;
  if (bComplete) {
    SIZE_T offset = 0;
    for (;;) {
      const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(impl.buffer + offset);
      const WString name(info->FileName, info->FileNameLength / sizeof(WCHAR));
      outChanges.push_back(FileChange{.path = Path(UTF8::fromWide(name)),
                                      .type = toChangeType(info->Action)});
      if (info->NextEntryOffset == 0) {
        break;
      }
      offset += info->NextEntryOffset;
    }
  }

  if (!impl.requestChanges()) {
    CH_LOG_ERROR(DirectoryWatcherLog, "Stopped watching {0} (error {1}).", m_directory,
                 static_cast<uint32>(GetLastError()));
    stop();
    bComplete = false;
  }
  return bComplete;
}

} // namespace chEngineSDK

#endif // USING(CH_PLATFORM_WIN32)
