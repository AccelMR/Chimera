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

namespace chEngineSDK {
namespace fs = std::filesystem;

CH_LOG_DECLARE_STATIC(FileSystemLog, All);

namespace {
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
toAbsoluteFsPath(const Path& path)
{
  std::error_code error;
  fs::path result = fs::absolute(fs::path(path.toString()), error);
  if (error) {
    logError("make absolute", path, error);
    result = fs::path(path.toString());
  }

  result = result.lexically_normal();

  // lexically_normal keeps a trailing separator ("C:/a/"), which would make the same
  // directory compare different from "C:/a".
  if (!result.has_filename() && result.has_relative_path()) {
    result = result.parent_path();
  }
  return result;
}
} // namespace

bool
FileSystem::renameFile(const Path& oldPath, const Path& newPath)
{
  if (!isFile(oldPath)) {
    return false;
  }

  std::error_code error;
  fs::rename(oldPath.toString(), newPath.toString(), error);
  if (error) {
    logError("rename", oldPath, error);
    return false;
  }
  return true;
}

bool
FileSystem::removeFile(const Path& path)
{
  if (!isFile(path)) {
    return false;
  }
  return remove(path);
}

Path
FileSystem::absolutePath(const Path& path)
{
  return Path(toAbsoluteFsPath(path).generic_string());
}

bool
FileSystem::isFile(const Path& path)
{
  std::error_code error;
  return fs::is_regular_file(path.toString(), error);
}

bool
FileSystem::isDirectory(const Path& path)
{
  std::error_code error;
  return fs::is_directory(path.toString(), error);
}

bool
FileSystem::isSubPath(const Path& basePath, const Path& path)
{
  const fs::path relative = toAbsoluteFsPath(path).lexically_relative(
                              toAbsoluteFsPath(basePath));
  return !relative.empty() && *relative.begin() != "..";
}

bool
FileSystem::createDirectory(const Path& path)
{
  std::error_code error;
  fs::create_directory(path.toString(), error);
  if (error) {
    logError("create directory", path, error);
    return false;
  }
  return true;
}

bool
FileSystem::createDirectories(const Path& path)
{
  std::error_code error;
  fs::create_directories(path.toString(), error);
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
  return fs::exists(path.toString(), error);
}

SPtr<DataStream>
FileSystem::openFile(const Path& path, bool readOnly /*= true*/)
{
  AccesModeFlag accessMode(ACCESS_MODE::kREAD);
  if (!readOnly) {
    accessMode.set(ACCESS_MODE::kWRITE);
  }

  try {
    return chMakeShared<FileDataStream>(absolutePath(path), accessMode, true);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
    return nullptr;
  }
}

SPtr<DataStream>
FileSystem::createAndOpenFile(const Path& path)
{
  const Path fullPath = absolutePath(path);

  const Path parentDir = fullPath.getDirectory();
  if (!exists(parentDir) && !createDirectories(parentDir)) {
    return nullptr;
  }

  try {
    return chMakeShared<FileDataStream>(fullPath, ACCESS_MODE::kWRITE, true);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
    return nullptr;
  }
}

void
FileSystem::dumpMemStreamIntoFile(const SPtr<DataStream>& memStream, const Path& path)
{
  try {
    // The constructor writes the whole memory stream and the destructor closes the file.
    FileDataStream fileStream(path, memStream);
  }
  catch (const std::exception& e) {
    logError("FileSystem: " + String(e.what()));
  }
}

bool
FileSystem::remove(const Path& path)
{
  std::error_code error;
  const bool removed = fs::remove(path.toString(), error);
  if (error) {
    logError("remove", path, error);
    return false;
  }
  return removed;
}

bool
FileSystem::removeAll(const Path& path)
{
  std::error_code error;
  const std::uintmax_t removedCount = fs::remove_all(path.toString(), error);
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
  const fs::path fsPath = toAbsoluteFsPath(path);

  std::error_code error;
  fs::directory_iterator it(fsPath, fs::directory_options::skip_permission_denied, error);
  if (error) {
    logError("list", path, error);
    return;
  }

  for (const fs::directory_iterator end{}; it != end; it.increment(error)) {
    if (error) {
      logError("list", path, error);
      return;
    }

    std::error_code statusError;
    if (it->is_directory(statusError)) {
      directories.push_back(Path(it->path().generic_string()));
    }
    else if (it->is_regular_file(statusError) || it->is_other(statusError)) {
      files.push_back(Path(it->path().generic_string()));
    }
  }
}

void
FileSystem::forEachFileChildRecursive(const Path& path,
                                      const Function<void(const Path&)>& func)
{
  const fs::path fsPath = toAbsoluteFsPath(path);

  std::error_code error;
  fs::recursive_directory_iterator it(fsPath,
                                      fs::directory_options::skip_permission_denied,
                                      error);
  if (error) {
    logError("list", path, error);
    return;
  }

  for (const fs::recursive_directory_iterator end{}; it != end; it.increment(error)) {
    if (error) {
      logError("list", path, error);
      return;
    }

    std::error_code statusError;
    if (it->is_directory(statusError) || it->is_regular_file(statusError) ||
        it->is_other(statusError)) {
      func(Path(it->path().generic_string()));
    }
  }
}
} // namespace chEngineSDK
