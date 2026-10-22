/************************************************************************/
/**
 * @file chPlatformSystem.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Operating system services that are not about windows or input.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chPath.h"

namespace chEngineSDK {

/**
 * Exists so chCore can start programs and find the user's folders without naming the
 * platform layer; it is implemented next to the other SDL code.
 */
class PlatformSystem
{
 public:
  /**
   * Starts arguments[0] with the rest as its command line and returns without waiting for
   * it. The new process outlives this one.
   */
  NODISCARD static bool
  launchProcess(const Vector<String>& arguments);

  /**
   * Folder where this user's data for the application can be written (AppData on Windows,
   * ~/.local/share on Linux). It is created if missing; empty when it cannot be.
   */
  NODISCARD static Path
  getUserDataDirectory(StringView organization, StringView application);
};

} // namespace chEngineSDK
