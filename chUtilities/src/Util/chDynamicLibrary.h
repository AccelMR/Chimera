/************************************************************************/
/**
 * @file chDynamicLibrary.h
 * @author AccelMR
 * @date 2022/06/14
 * @brief Class that holds the actual data of a loaded Dynamic Library.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#if USING(CH_PLATFORM_WIN32)
struct HINSTANCE__;
using hInstance = struct HINSTANCE__ *;

#elif USING(CH_PLATFORM_LINUX)
#include <dlfcn.h>

#endif// CH_PLATFORM_WIN32

#if USING(CH_PLATFORM_WIN32)
# define DYNAMIC_LIBRARY_HANDLE hInstance
# define DYNAMIC_LIBRARY_LOAD(x) LoadLibraryEx(x, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)
# define DYNAMIC_LIBRARY_GET_SYMBOL(x, y) GetProcAddress(x, y)
# define DYNAMIC_LIBRARY_UNLOAD(x) !FreeLibrary(x)

#elif USING( CH_PLATFORM_LINUX )
# define DYNAMIC_LIBRARY_HANDLE void*
# define DYNAMIC_LIBRARY_LOAD(a) dlopen(a, RTLD_LAZY | RTLD_GLOBAL)
# define DYNAMIC_LIBRARY_GET_SYMBOL(a, b) dlsym(a, b)
# define DYNAMIC_LIBRARY_UNLOAD(a) dlclose(a)

#endif

namespace chEngineSDK{
/*
 * Description: 
 *     Dynamic Library wrapper that is Platform Independent. 
 *
 * Sample usage:
 *
 */
class CH_UTILITY_EXPORT DynamicLibrary
{
 public:

 /*
  * @brief Default constructor
  */
  DynamicLibrary() = delete;
   
  /** 
   *   Constructs a Dynamic Library depending on its name.
   **/
  DynamicLibrary(String _name);
  
  /** 
   *   Loads a dynamic library based on its name.
   **/
  void 
  load();

 /*
  * @brief Default destructor
  */
  ~DynamicLibrary() = default;

  /** 
   *   Unloads this dynamic library based on its name.
   **/
  void 
  unload();

  /** 
   *   Returns the address of the given symbol from loaded library.
   * 
   * @param strName
   *   The name of the symbol to search.
   * 
   * @return
   *  Returns the handle to the needed symbol, nullptr if not found.
   **/
  void*
  getSymbol(const String& strName);

  /** 
   *   Returns the name of this dynamic library.
   **/
  FORCEINLINE const String& 
  getName() const {
    return m_name;
  }

 protected:
 friend class DynamicLibraryManager;

public:
  static const char *EXTENSION;
  static const char *PREFIX;

protected:
  String m_name;
  DYNAMIC_LIBRARY_HANDLE m_dynLibHandler; 

};

}

