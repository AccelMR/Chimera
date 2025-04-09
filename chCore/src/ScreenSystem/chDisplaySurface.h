/************************************************************************/
/**
 * @file chDisplay.h
 * @author AccelMR
 * @date 2022/09/11
 *   DisplaySurface wrapper that handles a window internally.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chDisplayEventHandle.h"

#ifdef CH_CROSS_WINDOW
namespace xwin{
  class Window;
}
using PlatformDisplay = xwin::Window*;
#else 
#define CH_SDL_WINDOW USE_IF(USING(CH_PLATFORM_LINUX))
#if USING(CH_PLATFORM_WIN32)
struct HWND__;
using PlatformDisplay = HWND__*;
#elif USING(CH_PLATFORM_LINUX)
struct SDL_Window;
using PlatformDisplay = SDL_Window*;
#endif

#endif //CH_CROSS_WINDOW

namespace chEngineSDK{
/*
 * Description: 
 *     Descriptor of how to create a DisplaySurface.
 */
struct CH_CORE_EXPORT ScreenDescriptor {
  String name = "chimeraDefault";
  String title = "chimeraDEfaultTitle";
  uint32 width = 1280;
  uint32 height = 720;
};

/*
 * Description: 
 *     DisplaySurface platform agnostic.
 *
 * Sample usage:
 * You must avoid creating DisplaySurface from out of DisplayManager.
 * 
 *  DisplayManager::startUp();
 * 
 *  ScreenDescriptor winDesc;
 *  winDesc.name = "ChimeraCoreUnitTest";
 *  winDesc.title = "Chimera Core Unit Test";
 * 
 *  auto Eventhandler = chMakeShared<DisplayEventHandle>();
 *  auto screen = DisplayManager::instance().createDisplay(winDesc, Eventhandler);
 * 
 *  bool running = true;
 *  while (running) {
 *    Eventhandler->update();
 * 
 *    while (!Eventhandler->empty()) {
 *      DisplayEvent event = Eventhandler->frontPop();
 *      switch (event.getType())
 *      {
 *      case PLATFORM_EVENT_TYPE::kCLOSE:
 *        screen->close();
 *        running = false;
 *      break;
 *        break;
 *      default:
 *        break;
 *      }
 *    }
 *  }
 *
 */
class CH_CORE_EXPORT DisplaySurface
{
 public:
 /*
  *   Default constructor
  */
  FORCEINLINE DisplaySurface() = default;

 /*
  *   Default destructor
  */
  FORCEINLINE ~DisplaySurface();

  /** 
   *   Closes this window and cleans memory.
   **/
  void
  close();

  FORCEINLINE uint32
  getWidth(){ return m_width; }

  FORCEINLINE uint32
  getHeight() { return m_height; }

  FORCEINLINE PlatformDisplay
  getPlatformHandler() { return m_displayHandle; }

  /** 
   *   Do not use unless you know what you are doing.
   *  This function is used to get very specific platform handler.
   **/
  uint32
  getPlatformHandlerInt();

protected:
  friend class DisplayManager;

  /** 
   *   Initialize this screen, from a descriptor using a proper handler.
   * 
   * @param desc
   *  Descriptor how to create the scree.
   * 
   * @param eventHandler
   *  The event to handle all messages coming from the screen.
   * 
   * @return bool
   *  True if initialization went good.
   **/
  NODISCARD  bool
  init(ScreenDescriptor desc, SPtr<DisplayEventHandle> eventHandler);

 private:
 //Pointer to the actual screen.
  PlatformDisplay m_displayHandle;

  uint32 m_width;
  uint32 m_height;
 
};
/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/
/*
*/
DisplaySurface::~DisplaySurface() {
  close();
}


}