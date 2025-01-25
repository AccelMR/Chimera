/************************************************************************/
/**
 * @file chScreenLinux.cpp
 * @author AccelMR
 * @date 2024/10/13
 *   Linux specific creation.
 */
 /************************************************************************/
 #include "chScreen.h"

#if USING(CH_SDL_WINDOW)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chDebug.h"

namespace chEngineSDK{

/*
*/
NODISCARD bool
Screen::init(SCREEN_DESC desc, SPtr<ScreenEventHandle> eventHandler) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    CH_EXCEPT(InternalErrorException, "Screen.init() - SDL_Init failed.");
    return false;
  }

  m_screenHandle = SDL_CreateWindow(desc.title.c_str(),
                                    desc.width,
                                    desc.height,
                                    SDL_WINDOW_VULKAN);

  if (m_screenHandle == nullptr) {
    CH_EXCEPT(InternalErrorException, "Screen.init() - SDL_CreateWindow failed.");
    return false;
  }

  m_width = desc.width;
  m_height = desc.height;

  LOG_INFO("Screen created successfully with Vulkan support.");
  return true;
}

/*
*/
void
Screen::close() {
  SDL_DestroyWindow(static_cast<SDL_Window*>(m_screenHandle));
  SDL_Quit();
}

} // namespace chEngineSDK

#endif