
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
#include "chUnicode.h"



namespace chEngineSDK {
namespace fs = std::filesystem;

Path::Path(const String& path) {
    String normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    m_path = fs::path(normalizedPath);
}

bool 
Path::isRelative() const {
  return m_path.is_relative();
}

bool 
Path::isDirectory() const {
  return fs::is_directory(m_path);
}

bool 
Path::isFile() const {
  return fs::is_regular_file(m_path);
}

bool 
Path::exists() const {
  return fs::exists(m_path);
}

#if USING(CH_PLATFORM_WIN32)
WString
#else
String
#endif 
Path::getPlatformString() const {
#if USING(CH_PLATFORM_WIN32)
  return m_path.generic_wstring();
#else
  return m_path.generic_string();
#endif 
}

String 
Path::toString() const {
  return m_path.generic_string();
}

String 
Path::getFileName(bool withExtension) const {
  return withExtension ? m_path.filename().string() : m_path.stem().string();
}

String 
Path::getExtension() const {
  return m_path.extension().string();
}

String 
Path::getDirectory() const {
  return m_path.parent_path().string();
}

void 
Path::setPath(const String& newPath) {
  m_path = fs::path(newPath);
}

Path 
Path::join(const Path& other) const {
  return Path((m_path / other.m_path));
}

bool 
Path::operator<(const Path& other) const {
  return m_path < other.m_path;
}

Path
Path::operator+(const String& other) const {
  return Path((m_path.c_str() + other));
}

}  // namespace chEngineSDK
