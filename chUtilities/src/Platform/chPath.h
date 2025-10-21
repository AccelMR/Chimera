/************************************************************************/
/**
 * @file chPath.h
 * @author AccelMR
 * @date 2022/06/23
 * @brief Path that is generic to platform so it can be used along the engine with
 *        no problem.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"
#include <filesystem>  // for std::filesystem

namespace chEngineSDK {
namespace fs = std::filesystem;

class CH_UTILITY_EXPORT Path {
 public:
  /**
   * Default constructor.
   */
  FORCEINLINE
  Path()
    : m_path() {}

  /**
   * Constructor from a simple string.
   *
   * @param path
   *    As string.
   */
  explicit Path(const String& path);

  /**
   * Constructor from a C-string.
   *
   * @param path
   *    As C-string.
  */
  explicit Path(const ANSICHAR* path)
    : m_path(path) {}

  FORCEINLINE Path(const Vector<Path>& pathsToConcat) {
    for (const auto& path : pathsToConcat) {
      m_path /= path.m_path;
    }
  }

  /**
   * Constructor from a vector of paths.
   * @param paths
   *    Vector of paths to join.
   */
  template<typename... Paths,
           typename = std::enable_if_t<(std::is_same_v<Paths, Path> && ...)>>
  explicit Path(const Paths&... paths) : m_path() {
    (m_path /= ... /= paths.m_path); // Fold expression to join all paths
  }

#if USING(CH_CPP17_OR_LATER)
  /**
   * Constructor from a filesystem path.
   *
   * @param path
   *    As filesystem path.
   */
  FORCEINLINE explicit
  Path(const fs::path& path)
    : m_path(path) {}

  /**
   * Conversion operator to fs::path.
   */
  FORCEINLINE
  operator fs::path() const {
    return m_path;
  }
#endif // #if USING(CH_CPP17_OR_LATER)

#if USING(CH_PLATFORM_WIN32)
  /**
   * Constructor from a wide string. Windows Only constructor.
   *
   * @param path
   *    As Wide string.
   */
  Path(const WString& path);
#endif

  /**
   * Default destructor.
   */
  FORCEINLINE
  ~Path() = default;

  /**
   * Checks if path is relative.
   *
   * @return True if path is relative, false for absolute path.
   */
  bool
  isRelative() const;

  /**
   * Constructs a string from this path.
   *
   * @return Path as a string.
   */
  String
  toString() const;

  /*
  * Check if path exists
  */
  bool
  exists() const;

  /**
   * Constructs a wide string from this path, platform-specific.
   *
   * @return Path as a wide string (Windows) or regular string (other platforms).
   */
#if USING(CH_PLATFORM_WIN32)
  WString
#else
  String
#endif
  getPlatformString() const;

  /**
   * Checks if this Path exists in the system.
   *
   * @return True if exists, false otherwise.
   */
  bool
  exist() const;

  /**
   * Checks if path is a directory.
   *
   * @return True if directory, false otherwise.
   */
  FORCEINLINE bool
  isDirectory() const;

  /**
   * Checks if path is a file.
   *
   * @return True if file, false otherwise.
   */
  FORCEINLINE bool
  isFile() const;

  /**
   * Sets and sanitizes the internal path.
   *
   * @param path
   *    The new string to be a path.
   */
  FORCEINLINE void
  setPath(const String& path);

  /**
   * Returns the file name of this path.
   *
   * @param extension
   *    Include extension if true.
   * @return File name as a string.
   */
  String
  getFileName(bool extension = true) const;

  /**
   * Returns the extension of this path.
   *
   * @return Extension as a string.
   */
  String
  getExtension() const;

  /**
   * Returns the directory of this path.
   *
   * @return Directory as a string.
   */
  Path
  getDirectory() const;

  /**
   * Joins this path with another path.
   *
   * @param rhs
   *    The path to join with.
   * @return The joined path.
   */
  Path
  join(const Path& rhs) const;

  /**
   * Operator for less than, required for sorting, maps, etc.
   *
   * @param other
   *    The path to compare to.
   * @return True if this path is less than the other path.
   */
  bool
  operator<(const Path& other) const;

  /**
   * Operator for adding a string to this path.
   *
   * @param other
   *   The string to add.
   * @return The new path.
   */
  Path
  operator+(const String& other) const;

  Path
  operator/(const String& other) const;

  Path
  operator/(const Path& other) const;

  /**
   * Operator for equality check.
   *
   * @param other
   *    The path to compare to.
   * @return True if paths are equal, false otherwise.
   */
  FORCEINLINE bool
  operator==(const Path& other) const{
    return m_path == other.m_path;
  }

  NODISCARD bool
  empty() const { return m_path.empty(); }

  /**
   * Returns the underlying filesystem path.
   *
   * @return The underlying filesystem path.
   */
  friend std::ostream& operator<<(std::ostream& os, const Path& path) {
    os << path.toString();
    return os;
  }

  static Path EMPTY;

 protected:
  friend class FileSystem;
  friend class FileDataStream;

  fs::path m_path;  // Using C++17 std::filesystem::path
};
}  // namespace chEngineSDK
