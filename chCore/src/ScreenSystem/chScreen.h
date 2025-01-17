/************************************************************************/
/**
 * @file chScreen.h
 * @author AccelMR
 * @date 2022/09/11
 *   Screen wrapper that handles a window internally.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chScreenEventHandle.h"

#if USING(CH_PLATFORM_WIN32)
using PlatformScreen = HWND*; //TODO: Change this to HWND
#elif USING(CH_PLATFORM_LINUX)
struct SDL_Window;
using PlatformScreen = SDL_Window*;
#endif

namespace chEngineSDK{
/*
 * Description: 
 *     Descriptor of how to create a Screen.
 */
struct CH_CORE_EXPORT SCREEN_DESC {
  String name = "chimeraDefault";
  String title = "chimeraDEfaultTitle";
  uint32 width = 1280;
  uint32 height = 720;
};

/*
 * Description: 
 *     Screen platform agnostic.
 *
 * Sample usage:
 * You must avoid creating Screen from out of ScreenModule.
 * 
 *  ScreenModule::startUp();
 * 
 *  SCREEN_DESC winDesc;
 *  winDesc.name = "ChimeraCoreUnitTest";
 *  winDesc.title = "Chimera Core Unit Test";
 * 
 *  auto Eventhandler = ch_shared_ptr_new<ScreenEventHandle>();
 *  auto screen = ScreenModule::instance().createScreen(winDesc, Eventhandler);
 * 
 *  bool running = true;
 *  while (running) {
 *    Eventhandler->update();
 * 
 *    while (!Eventhandler->empty()) {
 *      ScreenEvent event = Eventhandler->frontPop();
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
class CH_CORE_EXPORT Screen
{
 public:
 /*
  *   Default constructor
  */
  FORCEINLINE Screen() = default;

 /*
  *   Default destructor
  */
  FORCEINLINE ~Screen();

  /** 
   *   Closes this window and cleans memory.
   **/
  void
  close();

  FORCEINLINE uint32
  getWidth(){ return m_width; }

  FORCEINLINE uint32
  getHeight() { return m_height; }

  FORCEINLINE PlatformScreen
  getPlatformHandler() { return m_screenHandle; }

protected:
  friend class ScreenModule;

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
  init(SCREEN_DESC desc, SPtr<ScreenEventHandle> eventHandler);

 private:
 //Pointer to the actual screen.
  PlatformScreen m_screenHandle;

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
Screen::~Screen() {
  close();
}


}