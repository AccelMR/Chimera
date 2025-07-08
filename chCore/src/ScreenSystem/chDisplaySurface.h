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

#if USING(CH_DISPLAY_SDL3)
struct SDL_Window;
using PlatformDisplay = SDL_Window*;
#else
// Placeholder for other platforms, if needed
using PlatformDisplay = void*; // This should be replaced with the actual platform-specific type
#endif // USING(CH_DISPLAY_SDL3)



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
 *      case PlatformEventType::Close:
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
  uint64
  getPlatformHandlerAsInteger() const;

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

} // namespace chEngineSDK
