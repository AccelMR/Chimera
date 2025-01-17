/************************************************************************/
/**
 * @file chScreenEventHandleSDL.h
 * @author AccelMR
 * @date 2024/11/27
 * @brief SDL specific implementation of Screen event handle.
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
#include "chScreenEventHandle.h"

#if USING(CH_PLATFORM_LINUX)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


namespace chEngineSDK{
using chKeyBoard::Key;

/*
*/
void
mouseMove(ScreenEventHandle* seh, const SDL_Event& event, const PLATFORM_EVENT_TYPE& type) {
  MouseMoveData data;
  int x = event.motion.x;
  int y = event.motion.y;

  data.x = static_cast<uint32>(x);
  data.y = static_cast<uint32>(y);
  data.screenx = static_cast<uint32>(x);
  data.screeny = static_cast<uint32>(y);
  data.deltax = static_cast<int32>(x);
  data.deltay = static_cast<int32>(y);

  seh->addInputEvent(ScreenEvent(PLATFORM_EVENT_TYPE::kMOUSE_MOVE, data));
}

/*
*/
void
keyboardButtonChange(ScreenEventHandle* seh, const SDL_Event& event, const PLATFORM_EVENT_TYPE& type) {
  KeyBoardData kbData;
  switch (event.key.key) {
  case SDLK_ESCAPE:
    kbData.key = Key::Num0;
    break;
  case SDLK_W:
    kbData.key = Key::W;
    break;
  case SDLK_A:
    kbData.key = Key::A;
    break;
  case SDLK_S:
    kbData.key = Key::S;
    break;
  case SDLK_D:
    kbData.key = Key::D;
    break;
  default:
    kbData.key = Key::Num0;
    break;
  }

  seh->addInputEvent(ScreenEvent(type, kbData));
}

/*
*/
void 
ScreenEventHandle::update() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        addSystemEvent(ScreenEvent(PLATFORM_EVENT_TYPE::kCLOSE));
        break;
      case SDL_EVENT_KEY_DOWN:
        keyboardButtonChange(this, event, PLATFORM_EVENT_TYPE::kKEY_DOWN);
        break;
      case SDL_EVENT_KEY_UP:
        keyboardButtonChange(this, event, PLATFORM_EVENT_TYPE::kKEY_UP);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        mouseMove(this, event, PLATFORM_EVENT_TYPE::kMOUSE_MOVE);
        break;
      default:
        break;
    }
  }
}

}
#endif //USING(CH_PLATFORM_LINUX)