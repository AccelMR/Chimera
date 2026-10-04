/************************************************************************/
/**
 * @file chDisplaySurfaceSDL3.cpp
 * @author AccelMR
 * @date 2025/07/07
 *  SDL3 specific implementation of DisplaySurface.
 *  Core remains agnostic - this file handles all SDL3 specifics.
 */
/************************************************************************/
#include "chDisplaySurface.h"

#if USING (CH_DISPLAY_SDL3)

#include <SDL3/SDL.h>
//#include <SDL3/SDL_video.h>

#include "chLogger.h"

CH_LOG_DECLARE_STATIC(DisplaySurfaceLog, All);

namespace chEngineSDK {

namespace {
// Platform initialization tracking
static bool s_sdlInitialized = false;
static uint32 s_windowCount = 0;

bool
initializeSDL3() {
  if (s_sdlInitialized) {
    return true;
  }

  uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

  // TODO: Add more flags based on CVar settings
  if (false) {
    sdlFlags |= SDL_INIT_AUDIO;
  }

  if (!SDL_Init(sdlFlags)) {
    return false;
  }

  s_sdlInitialized = true;
  return true;
}

void
shutdownSDL3() {
  if (s_windowCount == 0 && s_sdlInitialized) {
    SDL_Quit();
    s_sdlInitialized = false;
  }
}
}

/*
*/
NODISCARD bool
DisplaySurface::init(ScreenDescriptor desc, SPtr<DisplayEventHandle> eventHandler) {
  if (!initializeSDL3()) {
    return false;
  }

  SDL_PropertiesID properties;

  // Create SDL3 properties for advanced window creation
  properties = SDL_CreateProperties();

  SDL_SetStringProperty(properties,
                       SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                       desc.title.c_str());

  //TODO: probably change this? this set a max width and height, but you can scale smaller for a reason
  SDL_SetNumberProperty(properties,
                       SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                       desc.width);
  SDL_SetNumberProperty(properties,
                       SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                       desc.height);

  SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
  SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, desc.resizable);

  // TODO: Add more capabilities based on descriptor
  // if (true) {
  //   SDL_SetBooleanProperty(platformHandle->properties,
  //                         SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN,
  //                         true);
  // }

  // Create window with properties
  m_displayHandle = SDL_CreateWindowWithProperties(properties);
  if (!m_displayHandle) {
    CH_LOG_FATAL(DisplaySurfaceLog, "Failed to create SDL3 window: {0}", SDL_GetError());
    return false;
  }

  ++s_windowCount;

  if (eventHandler) {
    eventHandler->m_mainWindowId = SDL_GetWindowID(m_displayHandle);
  }

  return true;
}

/*
*/
void
DisplaySurface::close() {
  if (m_displayHandle) {
    SDL_DestroyWindow(m_displayHandle);
    m_displayHandle = nullptr;

    --s_windowCount;
    shutdownSDL3();
  }
}

/*
*/
uint64
DisplaySurface::getPlatformHandlerAsInteger() const {
  return reinterpret_cast<uint64>(m_displayHandle);
}

/*
 */
bool
DisplaySurface::isMinimized() const
{
  return (SDL_GetWindowFlags(m_displayHandle) & SDL_WINDOW_MINIMIZED) != 0;
}

/*
 */
uint32
DisplaySurface::getWidth() const
{
  int32 width = 0;
  SDL_GetWindowSizeInPixels(m_displayHandle, &width, nullptr);
  return static_cast<uint32>(width);
}

/*
 */
uint32
DisplaySurface::getHeight() const
{
  int32 height = 0;
  SDL_GetWindowSizeInPixels(m_displayHandle, nullptr, &height);
  return static_cast<uint32>(height);
}

} // namespace chEngineSDK
#endif // CH_DISPLAY_SDL3
