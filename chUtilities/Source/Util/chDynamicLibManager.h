/************************************************************************/
/**
 * @file chDynamicLibManager.h
 * @author AccelMR
 * @date 2022/06/14
 * @brief Keeps every dynamic library loaded by the engine.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chDynamicLibrary.h"
#include "chModule.h"
#include "chPath.h"

namespace chEngineSDK {

/**
 * Keeps every dynamic library loaded by the engine, so asking twice for the same plugin
 * returns the library already loaded instead of loading it again.
 *
 * A name without extension gets the platform extension, and in debug builds the "d"
 * suffix CMake gives debug libraries ("chVulkan" -> "chVulkand.dll"). A name that already
 * ends with the extension is taken as the exact file name.
 *
 * Shutting the manager down unloads the libraries in reverse load order, so everything
 * created by a plugin must be destroyed before.
 */
class CH_UTILITY_EXPORT DynamicLibraryManager : public Module<DynamicLibraryManager>
{
 public:
  /**
   * Returns an empty pointer when the library cannot be loaded; the reason is logged.
   */
  WeakPtr<DynamicLibrary>
  loadDynLibrary(const String& name, const Path& directory = Path());

  /**
   * Logs a warning and does nothing for a library this manager did not load.
   */
  void
  unloadDynLibrary(const WeakPtr<DynamicLibrary>& library);

  NODISCARD WeakPtr<DynamicLibrary>
  getLibrary(const String& name) const;

 protected:
  void
  onShutDown() override;

 private:
  NODISCARD static String
  getFileName(const String& name);

  NODISCARD SPtr<DynamicLibrary>
  findByFileName(const String& fileName) const;

 private:
  // In load order, so they can be unloaded in reverse. There are only a few libraries and
  // they are looked up only when loading, so a linear search is enough.
  Vector<Pair<String, SPtr<DynamicLibrary>>> m_loadedLibraries;
};

} // namespace chEngineSDK
