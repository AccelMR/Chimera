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

#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace fs = std::filesystem;

/*
*/
/**
 * @brief Converts a path to an absolute path.
 */
Path
FileSystem::absolutePath(const Path& path) {
  return Path(fs::absolute(path.m_path));
}

/*
*/
/**
 * @brief Checks if a path is a file.
 */
bool
FileSystem::isFile(const Path& path) {
  fs::path fsPath(path.toString());
  return fs::is_regular_file(fsPath);
}

/**
 * @brief Creates a directory at a given path.
 */
bool
FileSystem::createDirectory(const Path& path) {
  fs::path fsPath(path.toString());
  return fs::create_directory(fsPath);
}

/**
 * @brief Creates directories recursively.
 */
bool
FileSystem::createDirectories(const Path& path) {
  fs::path fsPath(path.toString());
  return fs::create_directories(fsPath);
}

/**
 * @brief Checks if a path exists.
 */
bool
FileSystem::exists(const Path& path) {
  fs::path fsPath(path.toString());
  return fs::exists(fsPath);
}

/**
 * @brief Opens a file and returns a stream.
 */
SPtr<DataStream>
FileSystem::openFile(const Path& path, bool readOnly /*= true*/) {
  const Path fullPath = path.isRelative() ? 
    Path(fs::absolute(path.toString()).generic_string()) : 
    path;

  AccesModeFlag accessMode(ACCESS_MODE::kREAD);
  if (!readOnly) {
    accessMode.set(ACCESS_MODE::kWRITE);
  }

  return chMakeShared<FileDataStream>(fullPath, accessMode, true);
}

/**
 * @brief Creates and opens a new file.
 */
SPtr<DataStream>
FileSystem::createAndOpenFile(const Path& path) {
  // Get the parent directory of the file path
  Path parentDir = path.getDirectory();

  // Check if the directory exists, if not, create it
  if (!FileSystem::exists(parentDir)) {
    FileSystem::createDirectories(parentDir);
  }

  return chMakeShared<FileDataStream>(path, ACCESS_MODE::kWRITE, true);
}

/**
 * @brief Dumps the content of a memory data stream into a file.
 */
void
FileSystem::dumpMemStreamIntoFile(const SPtr<DataStream>& memStream, const Path& path) {
  auto fileStream = chMakeShared<FileDataStream>(path, memStream);
}

/**
 * @brief Deletes a file or directory.
 */
bool
FileSystem::remove(const Path &path) {
  return fs::remove(path.toString());
}

/**
 * @brief Deletes all contents within a directory recursively.
 */
bool
FileSystem::removeAll(const Path& path) {
  return fs::remove_all(path.toString());
}

/**
 * @brief Reads a file into a byte array.
 */
Vector<uint8>
FileSystem::fastRead(const Path& path) {
  Vector<uint8> ret;

  auto fileData = FileSystem::openFile(path);
  auto size = fileData->size();

  ret.resize(size);

  fileData->read(&ret[0], size);

  fileData->close();

  return ret;
}

/**
 * @brief Checks if a path is relative to another path.
 */
bool 
FileSystem::isPathRelative(const Path& basePath, const Path& targetPath) {
  // Check if both paths are absolute or relative
  if (basePath.isRelative() == targetPath.isRelative()) {
    const auto& basePathStr = basePath.toString();
    const auto& targetPathStr = targetPath.toString();

    // Check if basePathStr is a prefix of targetPathStr
    return targetPathStr.find(basePathStr) == 0;
  }

  return false;
}

/**
 * @brief Gets the files and directories under a certain path.
 */
void
FileSystem::getChildren(const Path& path, Vector<Path>& files, Vector<Path>& directories) {
  fs::path fsPath(path.toString());

  if (!fs::is_directory(fsPath)) {
    return;
  }

  for (const auto& entry : fs::directory_iterator(fsPath)) {
    Path childPath(entry.path().generic_string());
    if (entry.is_directory()) {
      directories.push_back(childPath);
    }
    else {
      files.push_back(childPath);
    }
  }
}
}