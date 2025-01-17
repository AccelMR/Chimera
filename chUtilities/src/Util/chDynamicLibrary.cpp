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

#if USING(CH_PLATFORM_WIN32)
#include "Win32/chWindows.h"
#endif

namespace chEngineSDK{
using std::move;

/************************************************************************/
/*
 * Platform specifics.
 */                                                                     
/************************************************************************/
#if USING(CH_PLATFORM_WIN32)
const char* DynamicLibrary::EXTENSION = "dll"; 
const char* DynamicLibrary::PREFIX = nullptr;

#elif USING(CH_PLATFORM_LINUX)
const char* DynamicLibrary::EXTENSION = "so";
const char* DynamicLibrary::PREFIX = nullptr;
#endif

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
  m_dynLibHandler = static_cast<DYNAMIC_LIBRARY_HANDLE>(DYNAMIC_LIBRARY_LOAD(m_name.c_str()));

  if (!m_dynLibHandler) {
#if USING(CH_PLATFORM_LINUX)
    std::cerr << "Error al cargar la biblioteca: " << dlerror() << std::endl;
#endif //USING(CH_PLATFORM_LINUX)
    CH_EXCEPT(InternalErrorException, "Could not load dynamic library " + m_name);
  }
}

/*
*/
void
DynamicLibrary::unload()
{
  if (DYNAMIC_LIBRARY_UNLOAD(m_dynLibHandler)) {
    CH_EXCEPT(InternalErrorException, "Could not unload dynamic library " + m_name);
  }
}

/*
*/
void*
DynamicLibrary::getSymbol(const String& strName)
{
  return static_cast<void *>(DYNAMIC_LIBRARY_GET_SYMBOL(m_dynLibHandler, strName.c_str()));
}

}


