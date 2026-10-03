/************************************************************************/
/**
 * @file chDynamicLibrary.cpp
 * @author AccelMR
 * @date 2022/06/15
 * @brief One loaded dynamic library (.dll / .so).
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chDynamicLibrary.h"

#include "chLogger.h"
#include "chStringUtils.h"

#if USING(CH_PLATFORM_WIN32)
# include "Win32/chWindows.h"
#else
# include <dlfcn.h>
#endif

CH_LOG_DECLARE_STATIC(DynamicLibraryLog, All);

namespace chEngineSDK {
namespace {

#if USING(CH_PLATFORM_WIN32)
String
lastErrorMessage()
{
  const DWORD code = GetLastError();
  String message = StringUtils::format("error {0}", static_cast<uint32>(code));

  LPSTR buffer = nullptr;
  const DWORD length =
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, code, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
  if (length > 0 && buffer) {
    message += ": ";
    message += StringUtils::trim(String(buffer, length));
    LocalFree(buffer);
  }
  return message;
}
#else
String
lastErrorMessage()
{
  const ANSICHAR* error = dlerror();
  return error ? String(error) : String("unknown error");
}
#endif

} // namespace

/*
 */
DynamicLibrary::DynamicLibrary(Path path)
 : m_path(std::move(path))
{
#if USING(CH_PLATFORM_WIN32)
  // LoadLibraryEx needs '\' separators: with '/' the dependencies of a library given by
  // absolute path are not searched next to it.
  const String windowsPath = StringUtils::replaceAllChars(m_path.toString(), '/', '\\');
  m_handle = LoadLibraryExA(windowsPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
  m_handle = dlopen(m_path.toString().c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif

  if (!m_handle) {
    // Read before logging, because writing the log can overwrite the system error.
    const String error = lastErrorMessage();
    CH_LOG_ERROR(DynamicLibraryLog, "Failed to load '{0}': {1}", m_path, error);
  }
}

/*
 */
DynamicLibrary::~DynamicLibrary()
{
  unload();
}

/*
 */
bool
DynamicLibrary::unload()
{
  if (!m_handle) {
    return true;
  }

#if USING(CH_PLATFORM_WIN32)
  const bool unloaded = FreeLibrary(static_cast<HMODULE>(m_handle)) != 0;
#else
  const bool unloaded = dlclose(m_handle) == 0;
#endif

  if (!unloaded) {
    const String error = lastErrorMessage();
    CH_LOG_ERROR(DynamicLibraryLog, "Failed to unload '{0}': {1}", m_path, error);
    return false;
  }

  m_handle = nullptr;
  CH_LOG_DEBUG(DynamicLibraryLog, "Unloaded '{0}'", m_path);
  return true;
}

/*
 */
void*
DynamicLibrary::getSymbol(const ANSICHAR* name) const
{
  if (!m_handle) {
    return nullptr;
  }

#if USING(CH_PLATFORM_WIN32)
  return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), name));
#else
  return dlsym(m_handle, name);
#endif
}

} // namespace chEngineSDK
