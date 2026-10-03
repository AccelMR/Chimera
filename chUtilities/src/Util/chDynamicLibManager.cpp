/************************************************************************/
/**
 * @file chDynamicLibManager.cpp
 * @author AccelMR
 * @date 2022/06/15
 * @brief Keeps every dynamic library loaded by the engine.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chDynamicLibManager.h"

#include "chDynamicLibrary.h"
#include "chLogger.h"

CH_LOG_DECLARE_STATIC(DynamicLibraryManagerLog, All);

namespace chEngineSDK {

/*
 */
WeakPtr<DynamicLibrary>
DynamicLibraryManager::loadDynLibrary(const String& name, const Path& directory)
{
  const String fileName = getFileName(name);

  const auto found = m_loadedLibraries.find(fileName);
  if (found != m_loadedLibraries.end()) {
    return found->second;
  }

  const Path filePath = directory.empty() ? Path(fileName) : directory / fileName;
  SPtr<DynamicLibrary> library = chMakeShared<DynamicLibrary>(filePath);
  if (!library->isLoaded()) {
    return {};
  }

  m_loadedLibraries.emplace(fileName, library);
  return library;
}

/*
 */
void
DynamicLibraryManager::unloadDynLibrary(const WeakPtr<DynamicLibrary>& library)
{
  const SPtr<DynamicLibrary> target = library.lock();
  for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it) {
    if (it->second == target) {
      target->unload();
      m_loadedLibraries.erase(it);
      return;
    }
  }

  CH_LOG_WARNING(DynamicLibraryManagerLog,
                 "Cannot unload a library that was not loaded by this manager.");
}

/*
 */
WeakPtr<DynamicLibrary>
DynamicLibraryManager::getLibrary(const String& name) const
{
  const auto found = m_loadedLibraries.find(getFileName(name));
  if (found != m_loadedLibraries.end()) {
    return found->second;
  }
  return {};
}

/*
 */
String
DynamicLibraryManager::getFileName(const String& name)
{
  if (name.ends_with(DynamicLibrary::EXTENSION)) {
    return name;
  }

  String fileName = name;
#if USING(CH_DEBUG_MODE)
  fileName += 'd';
#endif
  fileName += DynamicLibrary::EXTENSION;
  return fileName;
}

} // namespace chEngineSDK
