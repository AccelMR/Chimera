/************************************************************************/
/**
 * @file chScreenWin32.cpp
 * @author AccelMR
 * @date 2022/09/11
 *   Window x86 specific creation.
 */
 /************************************************************************/
#include "chScreen.h"

#if USING(CH_PLATFORM_WIN32)

#include "Win32/chWindows.h"
#include "chDebug.h"

namespace chEngineSDK{
using WinProcType = LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

/*
*/
NODISCARD bool
Screen::init(SCREEN_DESC desc, SPtr<ScreenEventHandle> eventHandler) {

  HINSTANCE hinstance = GetModuleHandle(nullptr);
  
  WNDCLASS wc = { };
  wc.lpfnWndProc = reinterpret_cast<WinProcType>(eventHandler->getPlatformCallBack());
  wc.hInstance = hinstance;
  wc.lpszClassName = desc.name.c_str();    
  // Reserve space to store the instance pointer
  wc.cbWndExtra = sizeof(ScreenEventHandle*);

  RegisterClass(&wc);

  int32 nCaptionHeight = ::GetSystemMetrics( SM_CYCAPTION );
  int32 nCaptionWidth = ::GetSystemMetrics( SM_CXBORDER );

  // Create the window.
  int32 nStyle = WS_OVERLAPPEDWINDOW;
  HWND hwnd = CreateWindowExA(WS_EX_ACCEPTFILES,
                              desc.name.c_str(),
                              desc.title.c_str(),
                              nStyle,
                              0, 0,
                              desc.width + nCaptionWidth, desc.height + nCaptionHeight,
                              NULL,
                              NULL,
                              hinstance,
                              NULL);

  if (hwnd== NULL) {
    CH_EXCEPT(InternalErrorException, "Screen.init()");
    return false;
  }

  ShowWindow(hwnd, SW_SHOW);

  // Brings the window at font [the thread just created]
  SetForegroundWindow(hwnd);
  // Sets the keyboard focus to the specified window
  SetFocus(hwnd);

  m_screenHandle = hwnd;
  m_width = desc.width;
  m_height = desc.height;
  
  // Store instance pointer
  SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(eventHandler.get()));

  return true;
}

/*
*/
void
Screen::close() {
  DestroyWindow(static_cast<HWND>(m_screenHandle));
}

}

#endif //#if USING(CH_PLATFORM_WIN32)
