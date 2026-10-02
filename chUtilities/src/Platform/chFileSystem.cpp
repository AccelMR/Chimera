/************************************************************************/
/**
 * @file chFileSystem.cpp
 * @author AccelMR
 * @date 2022/06/27
 * @brief File system that is platform specific.
 *
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chFileSystem.h"

#include <filesystem>
#include <iostream>
#include <system_error>

#include "chLogger.h"
#include "chPath.h"
#include "chStringUtils.h"

#if USING(CH_PLATFORM_WIN32)
#include "Win32/chWindows.h"
#endif

namespace chEngineSDK {
namespace fs = std::filesystem;

CH_LOG_DECLARE_STATIC(FileSystemLog, All);

namespace {
enum class Access
{
  Read,
  Write
};

struct Mount
{
  String name;
  fs::path root;
  int32 priority;
  bool writable;
};

struct VirtualPath
{
  String name;
  fs::path rest;
};

// The Logger opens its own file through FileSystem, and FileSystem can run before
// the Logger starts, so errors fall back to std::cerr when there is no Logger.
void
logError(const String& message)
{
  if (Logger::isStarted()) {
    CH_LOG_ERROR(FileSystemLog, "{0}", message);
  }
  else {
    std::cerr << message << std::endl;
  }
}

void
logError(const String& action, const Path& path, const std::error_code& error)
{
  logError("FileSystem: failed to " + action + " '" + path.toString() + "': " +
           error.message());
}

fs::path
normalize(fs::path path)
{
  path = path.lexically_normal();

  // lexically_normal keeps a trailing separator ("C:/a/"), which would make the same
  // directory compare different from "C:/a".
  if (!path.has_filename() && path.has_relative_path()) {
    path = path.parent_path();
  }
  return path;
}

fs::path&
baseDirectory()
{
  static fs::path directory;
  return directory;
}

// Sorted from highest to lowest priority, so the first match is the one that wins.
Vector<Mount>&
mounts()
{
  static Vector<Mount> mountList;
  return mountList;
}

bool
isMountName(const String& name)
{
  for (const Mount& mount : mounts()) {
    if (mount.name == name) {
      return true;
    }
  }
  return false;
}

bool
splitVirtualPath(const Path& path, VirtualPath& output)
{
  const String& text = path.toString();
  if (text.size() < 2 || text[0] != '/') {
    return false;
  }

  // Normalized first so "/Game/../x" cannot reach outside the mount.
  const String normalized = normalize(fs::path(text)).generic_string();
  if (normalized.size() < 2 || normalized[0] != '/') {
    return false;
  }

  const SIZE_T nameEnd = normalized.find('/', 1);
  const String name = normalized.substr(1, nameEnd == String::npos ? String::npos
                                                                   : nameEnd - 1);
  if (!isMountName(name)) {
    return false;
  }

  output.name = name;
  output.rest = nameEnd == String::npos ? fs::path() : fs::path(normalized.substr(nameEnd + 1));
  return true;
}

String
toVirtualText(const VirtualPath& virtualPath)
{
  String text = "/" + virtualPath.name;
  if (!virtualPath.rest.empty()) {
    text += "/" + virtualPath.rest.generic_string();
  }
  return text;
}

fs::path
toDiskPath(const Path& path)
{
  const fs::path fsPath(path.toString());
  if (fsPath.is_absolute()) {
    return normalize(fsPath);
  }

  if (!baseDirectory().empty()) {
    return normalize(baseDirectory() / fsPath);
  }

  std::error_code error;
  fs::path result = fs::absolute(fsPath, error);
  if (error) {
    logError("make absolute", path, error);
    result = fsPath;
  }
  return normalize(result);
}

// Reading uses the highest layer that has the path. If no layer has it, the path in
// the highest writable layer is returned, which is where it would be created.
fs::path
resolveVirtualPath(const VirtualPath& virtualPath, Access access)
{
  const Mount* writableMount = nullptr;
  const Mount* topMount = nullptr;

  for (const Mount& mount : mounts()) {
    if (mount.name != virtualPath.name) {
      continue;
    }

    const fs::path candidate = normalize(mount.root / virtualPath.rest);
    if (access == Access::Read) {
      std::error_code error;
      if (fs::exists(candidate, error)) {
        return candidate;
      }
    }

    if (!topMount) {
      topMount = &mount;
    }
    if (!writableMount && mount.writable) {
      writableMount = &mount;
    }
  }

  if (writableMount) {
    return normalize(writableMount->root / virtualPath.rest);
  }

  if (access == Access::Write) {
    logError("FileSystem: '/" + virtualPath.name + "' has no writable mount");
    return fs::path();
  }
  return topMount ? normalize(topMount->root / virtualPath.rest) : fs::path();
}

fs::path
toRealPath(const Path& path, Access access)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    return resolveVirtualPath(virtualPath, access);
  }
  return toDiskPath(path);
}

String
diskToVirtualText(const fs::path& diskPath)
{
  for (const Mount& mount : mounts()) {
    const fs::path relative = diskPath.lexically_relative(mount.root);
    if (relative.empty() || *relative.begin() == "..") {
      continue;
    }
    VirtualPath virtualPath{mount.name, relative == "." ? fs::path() : relative};
    return toVirtualText(virtualPath);
  }
  return String();
}

// A virtual path stays virtual; a disk path becomes virtual when it is inside a
// mount, so both kinds can be compared with each other.
fs::path
toComparablePath(const Path& path)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    return fs::path(toVirtualText(virtualPath));
  }

  const fs::path diskPath = toDiskPath(path);
  const String virtualText = diskToVirtualText(diskPath);
  return virtualText.empty() ? diskPath : fs::path(virtualText);
}

bool
forEachEntry(const fs::path& directory,
             bool recursive,
             const Function<void(const fs::directory_entry&)>& func)
{
  std::error_code error;
  if (!fs::is_directory(directory, error)) {
    return false;
  }

  const auto options = fs::directory_options::skip_permission_denied;
  if (recursive) {
    fs::recursive_directory_iterator it(directory, options, error);
    for (const fs::recursive_directory_iterator end{}; !error && it != end;
         it.increment(error)) {
      func(*it);
    }
  }
  else {
    fs::directory_iterator it(directory, options, error);
    for (const fs::directory_iterator end{}; !error && it != end; it.increment(error)) {
      func(*it);
    }
  }

  if (error) {
    logError("list", Path(directory.generic_string()), error);
  }
  return true;
}

bool
isListable(const fs::directory_entry& entry)
{
  std::error_code error;
  return entry.is_directory(error) || entry.is_regular_file(error) || entry.is_other(error);
}

// Calls func with the virtual path of every entry, taking each name only from the
// highest layer that has it.
void
forEachVirtualEntry(const VirtualPath& virtualPath,
                    bool recursive,
                    const Function<void(const Path&, const fs::directory_entry&)>& func)
{
  const String prefix = toVirtualText(virtualPath);
  UnorderedSet<String> seen;
  bool listedAny = false;

  for (const Mount& mount : mounts()) {
    if (mount.name != virtualPath.name) {
      continue;
    }

    const fs::path layerDirectory = normalize(mount.root / virtualPath.rest);
    listedAny |= forEachEntry(layerDirectory, recursive, [&](const fs::directory_entry& entry) {
      const String relative = entry.path().lexically_relative(layerDirectory).generic_string();
      if (seen.insert(relative).second) {
        func(Path(prefix + "/" + relative), entry);
      }
    });
  }

  if (!listedAny) {
    logError("FileSystem: '" + prefix + "' is not a directory in any mount");
  }
}

void
forEachChild(const Path& path,
             bool recursive,
             const Function<void(const Path&, const fs::directory_entry&)>& func)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    forEachVirtualEntry(virtualPath, recursive, func);
    return;
  }

  const fs::path directory = toDiskPath(path);
  if (!forEachEntry(directory, recursive, [&](const fs::directory_entry& entry) {
        func(Path(entry.path().generic_string()), entry);
      })) {
    logError("FileSystem: '" + path.toString() + "' is not a directory");
  }
}
} // namespace

bool
FileSystem::mount(const String& name,
                  const Path& directory,
                  int32 priority /*= 0*/,
                  bool writable /*= false*/)
{
  if (name.empty() || name.find('/') != String::npos) {
    logError("FileSystem: invalid mount name '" + name + "'");
    return false;
  }

  Mount newMount{name, toRealPath(directory, Access::Read), priority, writable};

  // Inserted before mounts of the same priority, so a later mount (a mod) wins.
  Vector<Mount>& mountList = mounts();
  auto it = mountList.begin();
  while (it != mountList.end() && it->priority > priority) {
    ++it;
  }
  mountList.insert(it, std::move(newMount));
  return true;
}

bool
FileSystem::unmount(const String& name, const Path& directory)
{
  const fs::path root = toRealPath(directory, Access::Read);
  Vector<Mount>& mountList = mounts();
  for (auto it = mountList.begin(); it != mountList.end(); ++it) {
    if (it->name == name && it->root == root) {
      mountList.erase(it);
      return true;
    }
  }
  return false;
}

bool
FileSystem::isVirtual(const Path& path)
{
  VirtualPath virtualPath;
  return splitVirtualPath(path, virtualPath);
}

Path
FileSystem::toVirtualPath(const Path& path)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    return Path(toVirtualText(virtualPath));
  }
  return Path(diskToVirtualText(toDiskPath(path)));
}

void
FileSystem::setBaseDirectory(const Path& directory)
{
  // Cleared first so a relative directory is resolved against the working directory,
  // not against the previous base.
  baseDirectory().clear();
  baseDirectory() = toDiskPath(directory);
}

Path
FileSystem::getBaseDirectory()
{
  return Path(toDiskPath(Path(".")).generic_string());
}

Path
FileSystem::getExecutableDirectory()
{
#if USING(CH_PLATFORM_WIN32)
  std::wstring buffer(MAX_PATH, L'\0');
  DWORD length = 0;
  while (true) {
    length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
      logError("FileSystem: GetModuleFileNameW failed with error " +
               std::to_string(GetLastError()));
      return Path();
    }
    if (length < buffer.size()) {
      break;
    }
    buffer.resize(buffer.size() * 2);
  }
  buffer.resize(length);
  const fs::path executable(buffer);
#elif USING(CH_PLATFORM_LINUX)
  std::error_code error;
  const fs::path executable = fs::read_symlink("/proc/self/exe", error);
  if (error) {
    logError("read", Path("/proc/self/exe"), error);
    return Path();
  }
#else
#error "FileSystem::getExecutableDirectory is not implemented for this platform"
#endif
  return Path(normalize(executable.parent_path()).generic_string());
}

Path
FileSystem::toRelativePath(const Path& path)
{
  return Path(toRealPath(path, Access::Read).lexically_relative(toDiskPath(Path(".")))
                .generic_string());
}

bool
FileSystem::renameFile(const Path& oldPath, const Path& newPath)
{
  const fs::path oldRealPath = toRealPath(oldPath, Access::Write);
  const fs::path newRealPath = toRealPath(newPath, Access::Write);
  if (oldRealPath.empty() || newRealPath.empty()) {
    return false;
  }

  std::error_code error;
  if (!fs::is_regular_file(oldRealPath, error)) {
    return false;
  }

  fs::rename(oldRealPath, newRealPath, error);
  if (error) {
    logError("rename", oldPath, error);
    return false;
  }
  return true;
}

bool
FileSystem::removeFile(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  std::error_code error;
  if (realPath.empty() || !fs::is_regular_file(realPath, error)) {
    return false;
  }
  return remove(path);
}

Path
FileSystem::absolutePath(const Path& path)
{
  return Path(toRealPath(path, Access::Read).generic_string());
}

bool
FileSystem::isFile(const Path& path)
{
  std::error_code error;
  return fs::is_regular_file(toRealPath(path, Access::Read), error);
}

bool
FileSystem::isDirectory(const Path& path)
{
  std::error_code error;
  return fs::is_directory(toRealPath(path, Access::Read), error);
}

bool
FileSystem::isSubPath(const Path& basePath, const Path& path)
{
  const fs::path relative = toComparablePath(path).lexically_relative(
                              toComparablePath(basePath));
  return !relative.empty() && *relative.begin() != "..";
}

bool
FileSystem::createDirectory(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  fs::create_directory(realPath, error);
  if (error) {
    logError("create directory", path, error);
    return false;
  }
  return true;
}

bool
FileSystem::createDirectories(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  fs::create_directories(realPath, error);
  if (error) {
    logError("create directories", path, error);
    return false;
  }
  return true;
}

bool
FileSystem::exists(const Path& path)
{
  std::error_code error;
  return fs::exists(toRealPath(path, Access::Read), error);
}

SPtr<DataStream>
FileSystem::openFile(const Path& path, bool readOnly /*= true*/)
{
  const fs::path realPath = toRealPath(path, readOnly ? Access::Read : Access::Write);
  if (realPath.empty()) {
    return nullptr;
  }

  AccesModeFlag accessMode(ACCESS_MODE::kREAD);
  if (!readOnly) {
    accessMode.set(ACCESS_MODE::kWRITE);
  }

  try {
    return chMakeShared<FileDataStream>(Path(realPath.generic_string()), accessMode, true);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
    return nullptr;
  }
}

SPtr<DataStream>
FileSystem::createAndOpenFile(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return nullptr;
  }

  std::error_code error;
  fs::create_directories(realPath.parent_path(), error);
  if (error) {
    logError("create directories for", path, error);
    return nullptr;
  }

  try {
    return chMakeShared<FileDataStream>(Path(realPath.generic_string()),
                                        ACCESS_MODE::kWRITE,
                                        true);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
    return nullptr;
  }
}

void
FileSystem::dumpMemStreamIntoFile(const SPtr<DataStream>& memStream, const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return;
  }

  try {
    // The constructor writes the whole memory stream and the destructor closes the file.
    FileDataStream fileStream(Path(realPath.generic_string()), memStream);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
  }
}

bool
FileSystem::remove(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  const bool removed = fs::remove(realPath, error);
  if (error) {
    logError("remove", path, error);
    return false;
  }
  return removed;
}

bool
FileSystem::removeAll(const Path& path)
{
  const fs::path realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  const std::uintmax_t removedCount = fs::remove_all(realPath, error);
  if (error) {
    logError("remove all", path, error);
    return false;
  }
  return removedCount > 0;
}

Vector<uint8>
FileSystem::fastRead(const Path& path)
{
  Vector<uint8> result;

  SPtr<DataStream> fileData = openFile(path);
  if (!fileData) {
    return result;
  }

  result.resize(fileData->size());
  if (!result.empty()) {
    fileData->read(result.data(), result.size());
  }
  fileData->close();

  return result;
}

void
FileSystem::getChildren(const Path& path, Vector<Path>& files, Vector<Path>& directories)
{
  forEachChild(path, false, [&](const Path& child, const fs::directory_entry& entry) {
    std::error_code error;
    if (entry.is_directory(error)) {
      directories.push_back(child);
    }
    else if (isListable(entry)) {
      files.push_back(child);
    }
  });
}

void
FileSystem::forEachFileChildRecursive(const Path& path,
                                      const Function<void(const Path&)>& func)
{
  forEachChild(path, true, [&](const Path& child, const fs::directory_entry& entry) {
    if (isListable(entry)) {
      func(child);
    }
  });
}
} // namespace chEngineSDK
