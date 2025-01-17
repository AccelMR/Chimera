/************************************************************************/
/**
 * @file chScreenLinux.cpp
 * @author AccelMR
 * @date 2024/10/13
 *   Linux specific creation.
 */
 /************************************************************************/
 #include "chScreen.h"

#if USING(CH_PLATFORM_LINUX)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chDebug.h"

namespace chEngineSDK{

/*
*/
NODISCARD bool
Screen::init(SCREEN_DESC desc, SPtr<ScreenEventHandle> eventHandler) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    CH_EXCEPT(InternalErrorException, "Screen.init()");
    return false;
  }

  m_screenHandle = SDL_CreateWindow(desc.title.c_str(),
                                    desc.width,
                                    desc.height,
                                    0);

  if (m_screenHandle == nullptr) {
    CH_EXCEPT(InternalErrorException, "Screen.init()");
    return false;
  }
      SDL_Surface* screenSurface = NULL;

  LOG_INFO("Screen being create in Wayland.");
  
    SDL_Renderer* r = SDL_CreateRenderer(m_screenHandle, NULL);

//TODO: delete this
  SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
  SDL_RenderClear(r);
  SDL_RenderPresent(r);
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