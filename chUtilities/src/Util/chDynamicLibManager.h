/************************************************************************/
/**
 * @file chDynamicLibManager.h
 * @author AccelMR
 * @date 2022/06/14
 * @brief This is a Dynamic Library Manager. This will keep every reference of each
 *        dll loaded.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include <chPrerequisitesUtilities.h>

#include "chModule.h"

#include "chDynamicLibrary.h"

namespace chEngineSDK{
using std::less;

/*
 * Description: 
 *     This manager will keep track of every dynamic-library loaded.
 *
 * Sample usage:
 *  WeakPtr<DynamicLibrary> TestLib = g_DynLibManager().load("LibraryName");
 *  auto Pointer = TestLib.lock();
 *  if(Pointer){ 
 *    Here you can use the actual pointer.
 *  }
 *
 */
class CH_UTILITY_EXPORT DynamicLibraryManager : public Module<DynamicLibraryManager>
{
 public:
  
  /** 
   *   Loads a Dynamic Library and returns a soft pointer.
   * 
   * NOTE: You'll be forced to check if the library is still alive before using the pointer.
   * Use .lock() to get the actuall SharedPointer.
   * 
   * @param name
   *  The name of the library to be loaded.
   * 
   * @return
   *  Weak pointer to loaded library. Pointer after lock shall be null if something failed
   *  during the load, please check output for more information.
   **/
  WPtr<DynamicLibrary>
  loadDynLibrary(const String& name);

  /** 
   *   Unloads a loaded library. If library does not exist then it'll do nothing. 
   * 
   * @param library
   *  Weak pointer to the already loaded library
   **/
  void 
  unloadDynLibrary(WPtr<DynamicLibrary> library);

  WPtr<DynamicLibrary> 
  getLibrary(const String &name);

 private:
  String 
  sanitizeName(const String& libName);

 private:
  Map<String, SPtr<DynamicLibrary>> m_loadedLibraries;
};
}

