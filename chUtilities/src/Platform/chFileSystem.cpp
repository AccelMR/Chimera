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

// Roots and the base directory are kept as normalized text with '/' so most
// queries can be answered without building a std::filesystem::path.
struct Mount
{
  String name;
  String root;
  int32 priority;
  bool writable;
};

// name and rest point into the original path text, or into storage when the text
// had to be normalized first, so a VirtualPath must not be copied or moved.
struct VirtualPath
{
  StringView name;
  StringView rest;
  String storage;
};

void
logError(const String& message)
{
  CH_LOG_ERROR(FileSystemLog, "{0}", message);
}

void
logError(const String& action, const Path& path, const std::error_code& error)
{
  logError("FileSystem: failed to " + action + " '" + path.toString() + "': " +
           error.message());
}

String
normalize(const fs::path& path)
{
  fs::path result = path.lexically_normal();

  // lexically_normal keeps a trailing separator ("C:/a/"), which would make the same
  // directory compare different from "C:/a".
  if (!result.has_filename() && result.has_relative_path()) {
    result = result.parent_path();
  }
  return result.generic_string();
}

// True when normalize() would return the same text: no "." or ".." segment, no
// "//" and no trailing '/'.
bool
isNormalized(StringView text) noexcept
{
  SIZE_T segmentStart = 0;
  for (SIZE_T i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != '/') {
      continue;
    }

    const StringView segment = text.substr(segmentStart, i - segmentStart);
    if (segment == "." || segment == ".." || (segment.empty() && segmentStart != 0)) {
      return false;
    }
    segmentStart = i + 1;
  }
  return true;
}

// Text that has a root ("/a", or "C:a" on Windows) cannot just be appended to the
// base directory.
bool
hasRoot(StringView text) noexcept
{
#if USING(CH_PLATFORM_WIN32)
  if (text.size() >= 2 && text[1] == ':') {
    return true;
  }
#endif
  return !text.empty() && text[0] == '/';
}

String
joinText(StringView directory, StringView rest)
{
  String result;
  result.reserve(directory.size() + rest.size() + 1);
  result.append(directory);
  if (!rest.empty()) {
    if (!result.empty() && result.back() != '/') {
      result.push_back('/');
    }
    result.append(rest);
  }
  return result;
}

// Both texts must be normalized. "C:/a" is inside "C:/a" and "C:/", not "C:/ab".
bool
isInside(StringView directory, StringView path) noexcept
{
  if (path.substr(0, directory.size()) != directory) {
    return false;
  }
  return path.size() == directory.size() || directory.back() == '/' ||
         path[directory.size()] == '/';
}

String&
baseDirectory()
{
  static String directory;
  return directory;
}

// Sorted from highest to lowest priority, so the first match is the one that wins.
Vector<Mount>&
mounts()
{
  static Vector<Mount> mountList;
  return mountList;
}

uint32
countLayers(StringView name) noexcept
{
  uint32 count = 0;
  for (const Mount& mount : mounts()) {
    if (mount.name == name) {
      ++count;
    }
  }
  return count;
}

bool
splitVirtualPath(const Path& path, VirtualPath& output)
{
  StringView text = path.toString();
  if (text.size() < 2 || text[0] != '/') {
    return false;
  }

  // Normalized first so "/Game/../x" cannot reach outside the mount.
  if (!isNormalized(text)) {
    output.storage = normalize(fs::path(path.toString()));
    text = output.storage;
    if (text.size() < 2 || text[0] != '/') {
      return false;
    }
  }

  const SIZE_T nameEnd = text.find('/', 1);
  const StringView name = text.substr(1, nameEnd == StringView::npos ? StringView::npos
                                                                     : nameEnd - 1);
  if (countLayers(name) == 0) {
    return false;
  }

  output.name = name;
  output.rest = nameEnd == StringView::npos ? StringView() : text.substr(nameEnd + 1);
  return true;
}

String
toVirtualText(StringView name, StringView rest)
{
  String text;
  text.reserve(name.size() + rest.size() + 2);
  text.push_back('/');
  text.append(name);
  if (!rest.empty()) {
    text.push_back('/');
    text.append(rest);
  }
  return text;
}

String
toDiskPath(const Path& path)
{
  const String& text = path.toString();
  if (isNormalized(text)) {
    if (!path.isRelative()) {
      return text;
    }
    if (!baseDirectory().empty() && !hasRoot(text)) {
      return joinText(baseDirectory(), text);
    }
  }

  const fs::path fsPath(text);
  if (fsPath.is_absolute()) {
    return normalize(fsPath);
  }

  if (!baseDirectory().empty()) {
    return normalize(fs::path(baseDirectory()) / fsPath);
  }

  std::error_code error;
  const fs::path result = fs::absolute(fsPath, error);
  if (error) {
    logError("make absolute", path, error);
    return normalize(fsPath);
  }
  return normalize(result);
}

// Reading uses the highest layer that has the path. If no layer has it, the path in
// the highest writable layer is returned, which is where it would be created. With a
// single layer the answer is the same either way, so the disk is not checked.
String
resolveVirtualPath(const VirtualPath& virtualPath, Access access)
{
  const bool checkDisk = access == Access::Read && countLayers(virtualPath.name) > 1;
  const Mount* writableMount = nullptr;
  const Mount* topMount = nullptr;

  for (const Mount& mount : mounts()) {
    if (mount.name != virtualPath.name) {
      continue;
    }

    if (checkDisk) {
      String candidate = joinText(mount.root, virtualPath.rest);
      std::error_code error;
      if (fs::exists(fs::path(candidate), error)) {
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
    return joinText(writableMount->root, virtualPath.rest);
  }

  if (access == Access::Write) {
    logError("FileSystem: '/" + String(virtualPath.name) + "' has no writable mount");
    return String();
  }
  return topMount ? joinText(topMount->root, virtualPath.rest) : String();
}

String
toRealPath(const Path& path, Access access)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    return resolveVirtualPath(virtualPath, access);
  }
  return toDiskPath(path);
}

String
diskToVirtualText(StringView diskPath)
{
  for (const Mount& mount : mounts()) {
    if (!isInside(mount.root, diskPath)) {
      continue;
    }

    StringView rest = diskPath.substr(mount.root.size());
    if (!rest.empty() && rest[0] == '/') {
      rest.remove_prefix(1);
    }
    return toVirtualText(mount.name, rest);
  }
  return String();
}

// A virtual path stays virtual; a disk path becomes virtual when it is inside a
// mount, so both kinds can be compared with each other.
String
toComparablePath(const Path& path)
{
  VirtualPath virtualPath;
  if (splitVirtualPath(path, virtualPath)) {
    return toVirtualText(virtualPath.name, virtualPath.rest);
  }

  String diskPath = toDiskPath(path);
  String virtualText = diskToVirtualText(diskPath);
  return virtualText.empty() ? diskPath : virtualText;
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
  const String prefix = toVirtualText(virtualPath.name, virtualPath.rest);
  const bool mergeLayers = countLayers(virtualPath.name) > 1;
  UnorderedSet<String> seen;
  bool listedAny = false;

  for (const Mount& mount : mounts()) {
    if (mount.name != virtualPath.name) {
      continue;
    }

    const String layerDirectory = joinText(mount.root, virtualPath.rest);
    listedAny |= forEachEntry(fs::path(layerDirectory), recursive,
                              [&](const fs::directory_entry& entry) {
      // Entries are built from layerDirectory, so their text starts with it.
      const String entryText = entry.path().generic_string();
      StringView relative = StringView(entryText).substr(layerDirectory.size());
      if (!relative.empty() && relative[0] == '/') {
        relative.remove_prefix(1);
      }

      if (mergeLayers && !seen.emplace(relative).second) {
        return;
      }

      String childText;
      childText.reserve(prefix.size() + relative.size() + 1);
      childText.append(prefix);
      childText.push_back('/');
      childText.append(relative);
      func(Path(std::move(childText)), entry);
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

  const String directory = toDiskPath(path);
  if (!forEachEntry(fs::path(directory), recursive, [&](const fs::directory_entry& entry) {
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
  const String root = toRealPath(directory, Access::Read);
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
    if (virtualPath.storage.empty()) {
      return path;
    }
    return Path(toVirtualText(virtualPath.name, virtualPath.rest));
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
  if (!baseDirectory().empty()) {
    return Path(baseDirectory());
  }
  return Path(toDiskPath(Path(".")));
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
  return Path(normalize(executable.parent_path()));
}

Path
FileSystem::toRelativePath(const Path& path)
{
  const fs::path realPath(toRealPath(path, Access::Read));
  return Path(realPath.lexically_relative(fs::path(toDiskPath(Path(".")))).generic_string());
}

bool
FileSystem::renameFile(const Path& oldPath, const Path& newPath)
{
  const String oldRealPath = toRealPath(oldPath, Access::Write);
  const String newRealPath = toRealPath(newPath, Access::Write);
  if (oldRealPath.empty() || newRealPath.empty()) {
    return false;
  }

  std::error_code error;
  if (!fs::is_regular_file(fs::path(oldRealPath), error)) {
    return false;
  }

  fs::rename(fs::path(oldRealPath), fs::path(newRealPath), error);
  if (error) {
    logError("rename", oldPath, error);
    return false;
  }
  return true;
}

bool
FileSystem::removeFile(const Path& path)
{
  const String realPath = toRealPath(path, Access::Write);
  std::error_code error;
  if (realPath.empty() || !fs::is_regular_file(fs::path(realPath), error)) {
    return false;
  }
  return remove(path);
}

Path
FileSystem::absolutePath(const Path& path)
{
  return Path(toRealPath(path, Access::Read));
}

bool
FileSystem::isFile(const Path& path)
{
  std::error_code error;
  return fs::is_regular_file(fs::path(toRealPath(path, Access::Read)), error);
}

bool
FileSystem::isDirectory(const Path& path)
{
  std::error_code error;
  return fs::is_directory(fs::path(toRealPath(path, Access::Read)), error);
}

bool
FileSystem::isSubPath(const Path& basePath, const Path& path)
{
  return isInside(toComparablePath(basePath), toComparablePath(path));
}

bool
FileSystem::createDirectory(const Path& path)
{
  const String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  fs::create_directory(fs::path(realPath), error);
  if (error) {
    logError("create directory", path, error);
    return false;
  }
  return true;
}

bool
FileSystem::createDirectories(const Path& path)
{
  const String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  fs::create_directories(fs::path(realPath), error);
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
  return fs::exists(fs::path(toRealPath(path, Access::Read)), error);
}

SPtr<DataStream>
FileSystem::openFile(const Path& path, bool readOnly /*= true*/)
{
  String realPath = toRealPath(path, readOnly ? Access::Read : Access::Write);
  if (realPath.empty()) {
    return nullptr;
  }

  AccesModeFlag accessMode(ACCESS_MODE::kREAD);
  if (!readOnly) {
    accessMode.set(ACCESS_MODE::kWRITE);
  }

  auto stream = chMakeShared<FileDataStream>(Path(std::move(realPath)), accessMode, true);
  if (!stream->isOpen()) {
    logError("FileSystem: failed to open '" + path.toString() + "'");
    return nullptr;
  }
  return stream;
}

SPtr<DataStream>
FileSystem::createAndOpenFile(const Path& path)
{
  String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return nullptr;
  }

  std::error_code error;
  fs::create_directories(fs::path(realPath).parent_path(), error);
  if (error) {
    logError("create directories for", path, error);
    return nullptr;
  }

  auto stream =
      chMakeShared<FileDataStream>(Path(std::move(realPath)), ACCESS_MODE::kWRITE, true);
  if (!stream->isOpen()) {
    logError("FileSystem: failed to create '" + path.toString() + "'");
    return nullptr;
  }
  return stream;
}

void
FileSystem::dumpMemStreamIntoFile(const SPtr<DataStream>& memStream, const Path& path)
{
  String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return;
  }

  // The constructor writes the whole memory stream and the destructor closes the file.
  FileDataStream fileStream(Path(std::move(realPath)), memStream);
  if (!fileStream.isOpen()) {
    logError("FileSystem: failed to write '" + path.toString() + "'");
  }
}

bool
FileSystem::remove(const Path& path)
{
  const String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  const bool removed = fs::remove(fs::path(realPath), error);
  if (error) {
    logError("remove", path, error);
    return false;
  }
  return removed;
}

bool
FileSystem::removeAll(const Path& path)
{
  const String realPath = toRealPath(path, Access::Write);
  if (realPath.empty()) {
    return false;
  }

  std::error_code error;
  const std::uintmax_t removedCount = fs::remove_all(fs::path(realPath), error);
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
