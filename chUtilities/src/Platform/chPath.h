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

namespace chEngineSDK {

/**
 * Path stored as a string that always uses '/' as separator. It is kept as a
 * string so this header does not need <filesystem>, which is heavy. Every
 * operation works on the text and gives the same result as std::filesystem
 * on the current platform.
 */
class CH_UTILITY_EXPORT Path
{
 public:
  /**
   * Default constructor.
   */
  FORCEINLINE
  Path() = default;

  /**
   * Constructor from a simple string.
   *
   * @param path
   *    As string.
   */
  explicit Path(String path);

  /**
   * Constructor from a C-string.
   *
   * @param path
   *    As C-string.
  */
  explicit Path(const ANSICHAR* path);

  /**
   * Constructor that joins every path in the list, in order.
   *
   * @param pathsToConcat
   *    Paths to join.
   */
  Path(const Vector<Path>& pathsToConcat);

  /**
   * Constructor that joins every given path, in order.
   * @param paths
   *    Paths to join.
   */
  template<typename... Paths,
           typename = std::enable_if_t<(std::is_same_v<Paths, Path> && ...)>>
  explicit Path(const Paths&... paths)
  {
    m_path.reserve((paths.m_path.size() + ...) + sizeof...(Paths));
    ((*this /= paths), ...);
  }

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
  NODISCARD bool
  isRelative() const noexcept;

  /**
   * Returns this path as a string.
   *
   * @return Path as a string.
   */
  NODISCARD FORCEINLINE const String&
  toString() const noexcept
  {
    return m_path;
  }

  /**
   * Constructs a wide string from this path, platform-specific.
   *
   * @return Path as a wide string (Windows) or regular string (other platforms).
   */
  NODISCARD
#if USING(CH_PLATFORM_WIN32)
  WString
#else
  String
#endif
  getPlatformString() const;

  /**
   * Sets and sanitizes the internal path.
   *
   * @param path
   *    The new string to be a path.
   */
  void
  setPath(String path);

  /**
   * Returns the file name of this path.
   *
   * @param extension
   *    Include extension if true.
   * @return File name as a string.
   */
  NODISCARD String
  getFileName(bool extension = true) const;

  /**
   * Returns the extension of this path.
   *
   * @return Extension as a string.
   */
  NODISCARD String
  getExtension() const;

  /**
   * Returns the directory of this path.
   *
   * @return Directory as a string.
   */
  NODISCARD Path
  getDirectory() const;

  /**
   * Joins this path with another path.
   *
   * @param rhs
   *    The path to join with.
   * @return The joined path.
   */
  NODISCARD Path
  join(const Path& rhs) const;

  /**
   * Joins another path to this one in place.
   *
   * @param rhs
   *    The path to join with.
   * @return This path.
   */
  Path&
  operator/=(const Path& rhs);

  /**
   * Operator for less than, required for sorting, maps, etc.
   *
   * @param other
   *    The path to compare to.
   * @return True if this path is less than the other path.
   */
  NODISCARD FORCEINLINE bool
  operator<(const Path& other) const noexcept
  {
    return m_path < other.m_path;
  }

  /**
   * Same as join(Path(other)).
   */
  NODISCARD Path
  operator/(const String& other) const;

  NODISCARD Path
  operator/(const Path& other) const;

  /**
   * Operator for equality check.
   *
   * @param other
   *    The path to compare to.
   * @return True if paths are equal, false otherwise.
   */
  NODISCARD FORCEINLINE bool
  operator==(const Path& other) const noexcept
  {
    return m_path == other.m_path;
  }

  NODISCARD FORCEINLINE bool
  empty() const noexcept
  {
    return m_path.empty();
  }

  static Path EMPTY;

 protected:
  void
  append(StringView other);

  friend class FileSystem;
  friend class FileDataStream;

  String m_path;
};
}  // namespace chEngineSDK
