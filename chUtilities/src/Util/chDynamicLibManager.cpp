/************************************************************************/
/**
 * @file chDynamicLibManager.cpp
 * @author AccelMR
 * @date 2022/06/15
 * @brief This is a Dynamic Library Manager. This will keep every reference of each
 *        dll loaded.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDynamicLibManager.h"

#include "chDynamicLibrary.h"

namespace chEngineSDK{
/*
*/
void 
UtilUnloadLibrary(WeakPtr<DynamicLibrary> library) {
  auto RealPointer = library.lock();
  if (RealPointer) {
    RealPointer->unload();
  }
}

/*
*/
WeakPtr<DynamicLibrary>
DynamicLibraryManager::loadDynLibrary(const String& name, const Path& path)
{
  String fileName = sanitizeName(name);

  WeakPtr<DynamicLibrary> lib = getLibrary(fileName);

  if(lib.lock()){
    return lib;
  }
  
  Path fileWholeName(fileName);
  if (!path.empty()) {
    fileWholeName = path / fileName;
    std::cout << "Loading library: " << fileWholeName.toString() << std::endl;
  }

  SPtr<DynamicLibrary> newLib =  chMakeShared<DynamicLibrary>(fileWholeName.toString());
  m_loadedLibraries.emplace(fileName, newLib);

  return newLib;
}

/*
*/
void
DynamicLibraryManager::unloadDynLibrary(WeakPtr<DynamicLibrary> library)
{
  auto RealLibrary = library.lock();
  if (!RealLibrary) {
    CH_EXCEPT(InternalErrorException, "Could not unload library because pointer is null.");
    return;
  }

  const auto &iterFind = m_loadedLibraries.find(RealLibrary->getName());
  if (iterFind == m_loadedLibraries.end()) {
    CH_EXCEPT(InternalErrorException, "This Library was not added to the Dynamic Library Manager.");
  }
  
  m_loadedLibraries.erase(iterFind);
  UtilUnloadLibrary(library);
}

/*
*/
WeakPtr<DynamicLibrary>
DynamicLibraryManager::getLibrary(const String& name) {
  const auto &iterFind = m_loadedLibraries.lower_bound(name);
  if (iterFind != m_loadedLibraries.end() && iterFind->second->getName() == name) {
    return iterFind->second;
  }

  return SPtr<DynamicLibrary>(nullptr);
}

/*
*/
String
DynamicLibraryManager::sanitizeName(const String& libName) {
  String filename = libName;
  //Add the extension (.dll, .so, ...) if necessary.
  const SIZE_T length = filename.length();
  const String extension = String( "." ) + DynamicLibrary::EXTENSION;
  const SIZE_T extLength = extension.length();

  #if USING(CH_DEBUG_MODE)
  //Add the debug suffix if necessary.
  filename.append("d");
  #endif //USING(CH_DEBUG_MODE)

  if ( length <= extLength || filename.substr( length - extLength ) != extension ) {
    filename.append( extension );
  }

  if ( nullptr != DynamicLibrary::PREFIX ) {
    filename.insert( 0, DynamicLibrary::PREFIX );
  }

  return filename;
}

}


