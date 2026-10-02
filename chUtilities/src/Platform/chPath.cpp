
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

#include <filesystem>

namespace chEngineSDK {
namespace {
void
normalize(String& path) noexcept
{
  for (ANSICHAR& c : path) {
    if (c == '\\') {
      c = '/';
    }
  }
}

// Same root name rules as std::filesystem: on Windows a drive ("C:"), a network
// name ("//server") or a device prefix ("//?/", "//./", "/??/"); other platforms
// have no root names.
SIZE_T
rootNameEnd(StringView path) noexcept
{
#if USING(CH_PLATFORM_WIN32)
  const SIZE_T size = path.size();
  if (size < 2) {
    return 0;
  }

  const ANSICHAR first = path[0];
  if (path[1] == ':' && ((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z'))) {
    return 2;
  }

  if (first != '/') {
    return 0;
  }

  if (size >= 4 && path[3] == '/' && (size == 4 || path[4] != '/') &&
      ((path[1] == '/' && (path[2] == '?' || path[2] == '.')) ||
       (path[1] == '?' && path[2] == '?'))) {
    return 3;
  }

  if (size >= 3 && path[1] == '/' && path[2] != '/') {
    const SIZE_T separator = path.find('/', 3);
    return separator == StringView::npos ? size : separator;
  }

  return 0;
#else
  CH_PARAMETER_UNUSED(path);
  return 0;
#endif
}

bool
isAbsolute(StringView path) noexcept
{
#if USING(CH_PLATFORM_WIN32)
  // A drive needs a root directory ("C:/a"), "C:a" is relative to that drive's
  // current folder. Every other root name is absolute on its own.
  const SIZE_T rootEnd = rootNameEnd(path);
  if (rootEnd == 2) {
    return path.size() > 2 && path[2] == '/';
  }
  return rootEnd != 0;
#else
  return !path.empty() && path[0] == '/';
#endif
}

SIZE_T
relativePathStart(StringView path) noexcept
{
  SIZE_T start = rootNameEnd(path);
  while (start < path.size() && path[start] == '/') {
    ++start;
  }
  return start;
}

// On Windows the root directory takes every leading separator, elsewhere only
// the first one, so "//a" has the parent "/" there.
SIZE_T
rootDirectoryEnd(StringView path) noexcept
{
#if USING(CH_PLATFORM_WIN32)
  return relativePathStart(path);
#else
  return !path.empty() && path[0] == '/' ? 1 : 0;
#endif
}

StringView
fileNameOf(StringView path) noexcept
{
  const SIZE_T relativeStart = relativePathStart(path);
  const SIZE_T separator = path.rfind('/');
  if (separator == StringView::npos || separator < relativeStart) {
    return path.substr(relativeStart);
  }
  return path.substr(separator + 1);
}

// "." and ".." have no extension, and neither does a name whose only dot is the
// first character (".bashrc").
SIZE_T
extensionStart(StringView fileName) noexcept
{
  if (fileName == "..") {
    return StringView::npos;
  }
  const SIZE_T dot = fileName.rfind('.');
  return dot == 0 ? StringView::npos : dot;
}
} // namespace

Path Path::EMPTY;

Path::Path(String path)
  : m_path(std::move(path))
{
  normalize(m_path);
}

Path::Path(const ANSICHAR* path)
  : m_path(path)
{
  normalize(m_path);
}

Path::Path(const Vector<Path>& pathsToConcat)
{
  SIZE_T size = 0;
  for (const auto& path : pathsToConcat) {
    size += path.m_path.size() + 1;
  }
  m_path.reserve(size);

  for (const auto& path : pathsToConcat) {
    *this /= path;
  }
}

bool
Path::isRelative() const noexcept
{
  return !isAbsolute(m_path);
}

#if USING(CH_PLATFORM_WIN32)
WString
#else
String
#endif
Path::getPlatformString() const
{
#if USING(CH_PLATFORM_WIN32)
  return std::filesystem::path(m_path).generic_wstring();
#else
  return m_path;
#endif
}

String
Path::getFileName(bool withExtension) const
{
  StringView fileName = fileNameOf(m_path);
  if (!withExtension) {
    fileName = fileName.substr(0, extensionStart(fileName));
  }
  return String(fileName);
}

String
Path::getExtension() const
{
  const StringView fileName = fileNameOf(m_path);
  const SIZE_T dot = extensionStart(fileName);
  return dot == StringView::npos ? String() : String(fileName.substr(dot));
}

Path
Path::getDirectory() const
{
  const StringView path(m_path);
  const SIZE_T relativeStart = relativePathStart(path);
  if (relativeStart == path.size()) {
    return *this;
  }

  SIZE_T end = path.size();
  while (end > relativeStart && path[end - 1] != '/') {
    --end;
  }
  while (end > relativeStart && path[end - 1] == '/') {
    --end;
  }
  if (end == relativeStart) {
    end = rootDirectoryEnd(path);
  }

  Path directory;
  directory.m_path.assign(path.substr(0, end));
  return directory;
}

void
Path::setPath(String newPath)
{
  m_path = std::move(newPath);
  normalize(m_path);
}

Path
Path::join(const Path& other) const
{
  Path result;
  result.m_path.reserve(m_path.size() + other.m_path.size() + 1);
  result.m_path.append(m_path);
  result.append(other.m_path);
  return result;
}

// Follows std::filesystem::path::operator/=.
void
Path::append(StringView other)
{
  if (isAbsolute(other)) {
    m_path.assign(other);
    return;
  }

  const SIZE_T rootEnd = rootNameEnd(m_path);
  const SIZE_T otherRootEnd = rootNameEnd(other);
  if (otherRootEnd != 0 &&
      StringView(m_path).substr(0, rootEnd) != other.substr(0, otherRootEnd)) {
    m_path.assign(other);
    return;
  }

  if (otherRootEnd < other.size() && other[otherRootEnd] == '/') {
    m_path.erase(rootEnd);
  }
  else if (rootEnd == m_path.size()) {
    // A network name ("//server") is the only root name that needs a separator
    // before the relative part; "" and "C:" do not.
    if (rootEnd >= 3) {
      m_path.push_back('/');
    }
  }
  else if (m_path.back() != '/') {
    m_path.push_back('/');
  }

  m_path.append(other.substr(otherRootEnd));
}

Path&
Path::operator/=(const Path& other)
{
  if (&other == this) {
    const String copy = other.m_path;
    append(copy);
  }
  else {
    append(other.m_path);
  }
  return *this;
}

Path
Path::operator/(const String& other) const
{
  Path result;
  result.m_path.reserve(m_path.size() + other.size() + 1);
  result.m_path.append(m_path);
  if (other.find('\\') == String::npos) {
    result.append(other);
  }
  else {
    result.append(Path(other).m_path);
  }
  return result;
}

Path
Path::operator/(const Path& other) const
{
  return join(other);
}

}  // namespace chEngineSDK
