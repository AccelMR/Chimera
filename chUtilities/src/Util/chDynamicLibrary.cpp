/************************************************************************/
/**
 * @file chDynamicLibrary.cpp
 * @author AccelMR
 * @date 2022/06/15
 * @brief Class that holds the actual data of a loaded Dynamic Library.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDynamicLibrary.h"

#include "chFileSystem.h"
#include "chPath.h"


namespace chEngineSDK{
using std::move;

/************************************************************************/
/*
 * Platform specifics.
 */                                                                     
/************************************************************************/
#if USING(CH_PLATFORM_WIN32)
#include "Win32/chWindows.h"

const char* DynamicLibrary::EXTENSION = "dll"; 
const char* DynamicLibrary::PREFIX = nullptr;

#elif USING(CH_PLATFORM_LINUX)
#include <dlfcn.h>

const char* DynamicLibrary::EXTENSION = "so";
const char* DynamicLibrary::PREFIX = nullptr;
#endif // USING(CH_PLATFORM_WIN32)

namespace DynamicLibraryHelper
{
void* 
getSymbolPlatformSpecific(DynamicLibraryHandle handle, const char* name)
{
#if USING(CH_PLATFORM_LINUX)
  return dlsym(handle, name);
#elif USING(CH_PLATFORM_WIN32)
  return GetProcAddress(handle, name);
#endif
  CH_EXCEPT(InternalErrorException, "Could not get symbol " + String(name));
  return nullptr;
}

void*
loadLibraryPlatformSpecific(const char* name)
{
#if USING(CH_PLATFORM_LINUX)
  return dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
#elif USING(CH_PLATFORM_WIN32)
  return LoadLibraryEx(name, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif
  CH_EXCEPT(InternalErrorException, "Could not load library " + String(name));
  return nullptr;
}

bool
unloadLibraryPlatformSpecific(DynamicLibraryHandle handle)
{
#if USING(CH_PLATFORM_LINUX)
  return dlclose(handle);
#elif USING(CH_PLATFORM_WIN32)
  return FreeLibrary(handle);
#endif
  CH_EXCEPT(InternalErrorException, "Could not unload library");
}
} // namespace DynamicLibraryHelper
using namespace DynamicLibraryHelper;


/*
*/
DynamicLibrary::DynamicLibrary(String _name) 
  : m_name(std::move(_name)),
    m_dynLibHandler(nullptr)
{
  load();
}

/*
*/
void
DynamicLibrary::load()
{
  m_dynLibHandler = static_cast<DynamicLibraryHandle>(
                      loadLibraryPlatformSpecific(m_name.c_str()));
  if (!m_dynLibHandler) {
    CH_EXCEPT(InternalErrorException, "Could not load dynamic library " + m_name);
  }
}

/*
*/
void
DynamicLibrary::unload()
{
  if (unloadLibraryPlatformSpecific(m_dynLibHandler)) {
    CH_EXCEPT(InternalErrorException, "Could not unload dynamic library " + m_name);
  }
}

/*
*/
void*
DynamicLibrary::getSymbol(const String& strName)
{
  return static_cast<void *>(getSymbolPlatformSpecific(m_dynLibHandler, strName.c_str()));
}

} // namespace chEngineSDK


