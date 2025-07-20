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
using DynamicLibraryHandle = hInstance;
#elif USING( CH_PLATFORM_LINUX )
using DynamicLibraryHandle = void*;
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

  template <typename T>
  FORCEINLINE T
  getSymbol(const String& strName) {
    return reinterpret_cast<T>(getSymbol(strName));
  }

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
  DynamicLibraryHandle m_dynLibHandler;

};

}
