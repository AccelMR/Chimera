/************************************************************************/
/**
 * @file chDynamicLibrary.h
 * @author AccelMR
 * @date 2022/06/14
 * @brief One loaded dynamic library (.dll / .so).
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chPath.h"

namespace chEngineSDK {

using DynamicLibraryHandle = void*;

/**
 * Wraps one dynamic library, so plugins are loaded and their symbols read without
 * platform code. The library is loaded by the constructor; check isLoaded() before using
 * it. The destructor unloads it, so every object created by its code must be gone first.
 */
class CH_UTILITY_EXPORT DynamicLibrary
{
 public:
  explicit DynamicLibrary(Path path);

  ~DynamicLibrary();

  DynamicLibrary(const DynamicLibrary&) = delete;

  DynamicLibrary&
  operator=(const DynamicLibrary&) = delete;

  NODISCARD FORCEINLINE bool
  isLoaded() const noexcept
  {
    return m_handle != nullptr;
  }

  /**
   * Returns false and logs when the system refuses to unload the library.
   */
  bool
  unload();

  /**
   * Returns nullptr when the library is not loaded or has no such symbol.
   */
  NODISCARD void*
  getSymbol(const ANSICHAR* name) const;

  template<typename T>
  NODISCARD FORCEINLINE T
  getSymbol(const ANSICHAR* name) const
  {
    return reinterpret_cast<T>(getSymbol(name));
  }

  NODISCARD FORCEINLINE const Path&
  getPath() const noexcept
  {
    return m_path;
  }

 public:
#if USING(CH_PLATFORM_WIN32)
  static constexpr StringView EXTENSION = ".dll";
#else
  static constexpr StringView EXTENSION = ".so";
#endif

 private:
  Path m_path;
  DynamicLibraryHandle m_handle = nullptr;
};

} // namespace chEngineSDK
