/************************************************************************/
/**
 * @file chDisplaySurface.cpp
 * @author AccelMR
 * @date 2025/04/05
 *  SDL specific implementation of DisplaySurface.
 */
 /************************************************************************/
 #include "chDisplaySurface.h"

#if USING(CH_SDL_WINDOW)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chDebug.h"

namespace chEngineSDK{

/*
*/
NODISCARD bool
DisplaySurface::init(SCREEN_DESC desc, SPtr<DisplayEventHandle> eventHandler) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - SDL_Init failed.");
    return false;
  }

  m_displayHandle = SDL_CreateWindow(desc.title.c_str(),
                                    desc.width,
                                    desc.height,
                                    SDL_EVENT_WINDOW_SHOWN);

  if (m_displayHandle == nullptr) {
    CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - SDL_CreateWindow failed.");
    return false;
  }

  m_width = desc.width;
  m_height = desc.height;

  CH_LOG_INFO("DisplaySurface created successfully with Vulkan support.");
  return true;
}

/*
*/
void
DisplaySurface::close() {
  SDL_DestroyWindow(static_cast<SDL_Window*>(m_displayHandle));
  SDL_Quit();
}

} // namespace chEngineSDK

#endif