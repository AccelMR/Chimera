/************************************************************************/
/**
 * @file chPlatformSystemSDL3.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  SDL3 implementation of PlatformSystem.
 */
/************************************************************************/
#include "chPlatformSystem.h"

#if USING(CH_DISPLAY_SDL3)

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_stdinc.h>

#include "chLogger.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(PlatformSystemLog, All);

/*
 */
bool
PlatformSystem::launchProcess(const Vector<String>& arguments)
{
  if (arguments.empty()) {
    return false;
  }

  Vector<const ANSICHAR*> argumentPointers;
  argumentPointers.reserve(arguments.size() + 1);
  for (const String& argument : arguments) {
    argumentPointers.push_back(argument.c_str());
  }
  argumentPointers.push_back(nullptr);

  SDL_Process* process = SDL_CreateProcess(argumentPointers.data(), false);
  if (!process) {
    CH_LOG_ERROR(PlatformSystemLog, "Could not start {0}: {1}", arguments[0], SDL_GetError());
    return false;
  }
  // Only the handle goes; the process keeps running.
  SDL_DestroyProcess(process);
  return true;
}

/*
 */
Path
PlatformSystem::getUserDataDirectory(StringView organization, StringView application)
{
  const String organizationText(organization);
  const String applicationText(application);
  ANSICHAR* folder = SDL_GetPrefPath(organizationText.c_str(), applicationText.c_str());
  if (!folder) {
    CH_LOG_ERROR(PlatformSystemLog, "Could not find the user data folder: {0}",
                 SDL_GetError());
    return Path();
  }
  String text(folder);
  SDL_free(folder);
  // SDL ends the folder with a separator, which joined paths would double.
  while (text.size() > 1 && (text.back() == '/' || text.back() == '\\')) {
    text.pop_back();
  }
  return Path(std::move(text));
}

} // namespace chEngineSDK

#endif // USING(CH_DISPLAY_SDL3)
