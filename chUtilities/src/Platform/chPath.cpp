
/************************************************************************/
/**
 * @file chPath.cpp
 * @author AccelMR
 * @date 2022/06/23
 * @brief Path that is generic to platform so it can be used along the engine with
 *        no problem.
 *
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPath.h"

#include <algorithm>
#include <filesystem>

namespace chEngineSDK {
namespace fs = std::filesystem;

namespace {
String
normalize(String path)
{
  std::replace(path.begin(), path.end(), '\\', '/');
  return path;
}

Path
fromFsPath(const fs::path& path)
{
  return Path(path.generic_string());
}
} // namespace

Path Path::EMPTY = Path("");

Path::Path(const String& path)
  : m_path(normalize(path))
{}

Path::Path(const ANSICHAR* path)
  : m_path(normalize(path))
{}

Path::Path(const Vector<Path>& pathsToConcat)
{
  for (const auto& path : pathsToConcat) {
    *this = join(path);
  }
}

bool
Path::isRelative() const
{
  return fs::path(m_path).is_relative();
}

#if USING(CH_PLATFORM_WIN32)
WString
#else
String
#endif
Path::getPlatformString() const
{
#if USING(CH_PLATFORM_WIN32)
  return fs::path(m_path).generic_wstring();
#else
  return m_path;
#endif
}

String
Path::toString() const
{
  return m_path;
}

String
Path::getFileName(bool withExtension) const
{
  const fs::path path(m_path);
  return withExtension ? path.filename().string() : path.stem().string();
}

String
Path::getExtension() const
{
  return fs::path(m_path).extension().string();
}

Path
Path::getDirectory() const
{
  return fromFsPath(fs::path(m_path).parent_path());
}

void
Path::setPath(const String& newPath)
{
  m_path = normalize(newPath);
}

Path
Path::join(const Path& other) const
{
  return fromFsPath(fs::path(m_path) / other.m_path);
}

bool
Path::operator<(const Path& other) const
{
  return m_path < other.m_path;
}

Path
Path::operator/(const String& other) const
{
  return join(Path(other));
}

Path
Path::operator/(const Path& other) const
{
  return join(other);
}

}  // namespace chEngineSDK
