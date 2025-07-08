/************************************************************************/
/**
 * @file chDisplayEventHandleSDL3.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @brief
 * SDL3 specific implementation of DisplayEventHandle.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"

#if USING (CH_DISPLAY_SDL3)
#include <SDL3/SDL.h>

namespace chEngineSDK {
/*
*/
void
DisplayEventHandle::update() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        // Handle quit event
        addEvent(PlatformEventType::Close);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        addEvent(PlatformEventType::Resize, std::move(ResizeData{
          static_cast<uint32>(event.window.data1),
          static_cast<uint32>(event.window.data2)
        }));
        break;
        case SDL_EVENT_KEY_DOWN:
        break;
      default:
        // Handle other events if necessary
        break;
    }
  }
}

}  // namespace chEngineSDK

#endif // USING(CH_DISPLAY_SDL3)
