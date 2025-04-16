/************************************************************************/
/**
 * @file chDisplayEventHandleWin32.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief Win32 specific implementation of DisplaySurface event handle.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDisplayEventHandle.h"

#if USING(CH_PLATFORM_WIN32)
#include "Win32/chWindows.h"

#include "chLogger.h"

namespace chEngineSDK{
using chKeyBoard::Key;

static int32 prevMouseX = 0;
static int32 prevMouseY = 0;

/*
*/
void
mouseMove(DisplayEventHandle* seh, HWND hwnd, LPARAM lParam) {
  MouseMoveData data;
  int x = static_cast<short>(LOWORD(lParam));
  int y = static_cast<short>(HIWORD(lParam));

  RECT area;
  GetClientRect(hwnd, &area);

  data.x = static_cast<uint32>(area.left <= x && x <= area.right ? x - area.left : 0xFFFFFFFF);
  data.y = static_cast<uint32>(area.top <= y && y <= area.bottom ? y - area.top : 0xFFFFFFFF);
  data.screenx = static_cast<uint32>(x);
  data.screeny = static_cast<uint32>(y);
  data.deltax = static_cast<int32>(x - prevMouseX);
  data.deltay = static_cast<int32>(y - prevMouseY);

  prevMouseX = static_cast<uint32>(x);
  prevMouseY = static_cast<uint32>(y);

  seh->addInputEvent(DisplayEvent(PLATFORM_EVENT_TYPE::kMOUSE_MOVE, data));
}

/*
*/
void
keyboardButtonChange(DisplayEventHandle* seh, WPARAM wParam, PLATFORM_EVENT_TYPE type) {
  KeyBoardData kbData;
  switch (wParam) {
  case VK_ESCAPE:
    kbData.key = Key::Num0;
    break;
  case 0x30:
    kbData.key = Key::Num0;
    break;
  case 0x31:
    kbData.key = Key::Num1;
    break;
  case 0x32:
    kbData.key = Key::Num2;
    break;
  case 0x33:
    kbData.key = Key::Num3;
    break;
  case 0x34:
    kbData.key = Key::Num4;
    break;
  case 0x35:
    kbData.key = Key::Num5;
    break;
  case 0x36:
    kbData.key = Key::Num6;
    break;
  case 0x37:
    kbData.key = Key::Num7;
    break;
  case 0x38:
    kbData.key = Key::Num8;
    break;
  case 0x39:
    kbData.key = Key::Num9;
    break;
  case 0x41:
    kbData.key = Key::A;
    break;
  case 0x42:
    kbData.key = Key::B;
    break;
  case 0x43:
    kbData.key = Key::C;
    break;
  case 0x44:
    kbData.key = Key::D;
    break;
  case 0x45:
    kbData.key = Key::E;
    break;
  case 0x46:
    kbData.key = Key::F;
    break;
  case 0x47:
    kbData.key = Key::G;
    break;
  case 0x48:
    kbData.key = Key::H;
    break;
  case 0x49:
    kbData.key = Key::I;
    break;
  case 0x4A:
    kbData.key = Key::J;
    break;
  case 0x4B:
    kbData.key = Key::K;
    break;
  case 0x4C:
    kbData.key = Key::L;
    break;
  case 0x4D:
    kbData.key = Key::M;
    break;
  case 0x4E:
    kbData.key = Key::N;
    break;
  case 0x4F:
    kbData.key = Key::O;
    break;
  case 0x50:
    kbData.key = Key::P;
    break;
  case 0x51:
    kbData.key = Key::Q;
    break;
  case 0x52:
    kbData.key = Key::R;
    break;
  case 0x53:
    kbData.key = Key::S;
    break;
  case 0x54:
    kbData.key = Key::T;
    break;
  case 0x55:
    kbData.key = Key::U;
    break;
  case 0x56:
    kbData.key = Key::V;
    break;
  case 0x57:
    kbData.key = Key::W;
    break;
  case 0x58:
    kbData.key = Key::X;
    break;
  case 0x59:
    kbData.key = Key::Y;
    break;
  case 0x5A:
    kbData.key = Key::Z;
    break;
  case VK_SUBTRACT:
  case VK_OEM_MINUS:
    kbData.key = Key::Minus;
    break;
  case VK_ADD:
  case VK_OEM_PLUS:
    kbData.key = Key::Add;
    break;
  case VK_MULTIPLY:
    kbData.key = Key::Multiply;
    break;
  case VK_DIVIDE:
    kbData.key = Key::Divide;
    break;
  case VK_BACK:
    kbData.key = Key::Back;
    break;
  case VK_RETURN:
    kbData.key = Key::Enter;
    break;
  case VK_DELETE:
    kbData.key = Key::Del;
    break;
  case VK_TAB:
    kbData.key = Key::Tab;
    break;
  case VK_NUMPAD0:
    kbData.key = Key::Numpad0;
    break;
  case VK_NUMPAD1:
    kbData.key = Key::Numpad1;
    break;
  case VK_NUMPAD2:
    kbData.key = Key::Numpad2;
    break;
  case VK_NUMPAD3:
    kbData.key = Key::Numpad3;
    break;
  case VK_NUMPAD4:
    kbData.key = Key::Numpad4;
    break;
  case VK_NUMPAD5:
    kbData.key = Key::Numpad5;
    break;
  case VK_NUMPAD6:
    kbData.key = Key::Numpad6;
    break;
  case VK_NUMPAD7:
    kbData.key = Key::Numpad7;
    break;
  case VK_NUMPAD8:
    kbData.key = Key::Numpad8;
    break;
  case VK_NUMPAD9:
    kbData.key = Key::Numpad9;
    kbData.key = Key::Numpad9;
    break;
  case VK_UP:
    kbData.key = Key::Up;
    break;
  case VK_LEFT:
    kbData.key = Key::Left;
    break;
  case VK_DOWN:
    kbData.key = Key::Down;
    break;
  case VK_RIGHT:
    kbData.key = Key::Right;
    break;
  case VK_SPACE:
    kbData.key = Key::Space;
    break;
  case VK_HOME:
    kbData.key = Key::Home;
    break;
  case VK_F1:
    kbData.key = Key::F1;
    break;
  case VK_F2:
    kbData.key = Key::F2;
    break;
  case VK_F3:
    kbData.key = Key::F3;
    break;
  case VK_F4:
    kbData.key = Key::F4;
    break;
  case VK_F5:
    kbData.key = Key::F5;
    break;
  case VK_F6:
    kbData.key = Key::F6;
    break;
  case VK_F7:
    kbData.key = Key::F7;
    break;
  case VK_F8:
    kbData.key = Key::F8;
    break;
  case VK_F9:
    kbData.key = Key::F9;
    break;
  case VK_F10:
    kbData.key = Key::F10;
    break;
  case VK_F11:
    kbData.key = Key::F11;
    break;
  case VK_F12:
    kbData.key = Key::F12;
    break;
  case VK_SHIFT:
  case VK_LSHIFT:
  case VK_RSHIFT:
    kbData.key = Key::LShift;
    break;
  case VK_CONTROL:
  case VK_LCONTROL:
  case VK_RCONTROL:
    kbData.key = Key::LControl;
    break;
  case VK_MENU:
  case VK_LMENU:
  case VK_RMENU:
    kbData.key = Key::LAlt;
    break;
  case VK_OEM_PERIOD:
    kbData.key = Key::Period;
    break;
  case VK_OEM_COMMA:
    kbData.key = Key::Comma;
    break;
  case VK_OEM_1:
    kbData.key = Key::Semicolon;
    break;
  case VK_OEM_2:
    kbData.key = Key::Backslash;
    break;
  case VK_OEM_3:
    kbData.key = Key::Grave;
    break;
  case VK_OEM_4:
    kbData.key = Key::LBracket;
    break;
  case VK_OEM_6:
    kbData.key = Key::RBracket;
    break;
  case VK_OEM_7:
    kbData.key = Key::Apostrophe;
    break;
  default:
    kbData.key = Key::KeysMax;
    break;
  }

  seh->addInputEvent(DisplayEvent(type, kbData));
}

/*
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Retrieve instance pointer
  auto *screenEventHandler = reinterpret_cast<DisplayEventHandle *>(GetWindowLongPtrW(hwnd, 0));

  switch (msg)  {
  case WM_CLOSE:
    if (nullptr != screenEventHandler) {
      screenEventHandler->addSystemEvent(DisplayEvent(PLATFORM_EVENT_TYPE::kCLOSE));
    }
    break;

  case WM_MOUSEMOVE:
    if (nullptr != screenEventHandler) {
      mouseMove(screenEventHandler, hwnd, lParam);
    }
    break;

  case WM_KEYDOWN:
  case WM_CHAR:
  case WM_SYSKEYDOWN:
    if (nullptr != screenEventHandler) {
      keyboardButtonChange(screenEventHandler, wParam, PLATFORM_EVENT_TYPE::kKEY_DOWN);
    }
    break;

  case WM_KEYUP:
  case WM_SYSKEYUP:
    if (nullptr != screenEventHandler) {
      keyboardButtonChange(screenEventHandler, wParam, PLATFORM_EVENT_TYPE::kKEY_UP);
    }
    break;

  case WM_SIZE:
    if (nullptr != screenEventHandler) {
      ResizeData resizeData;
      resizeData.width = static_cast<uint32>(LOWORD(lParam));
      resizeData.height = static_cast<uint32>(HIWORD(lParam));
      screenEventHandler->addSystemEvent(DisplayEvent(PLATFORM_EVENT_TYPE::kRESIZE, resizeData));    
    }
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc( hwnd, msg, wParam, lParam );
  }
  return 0;
}

/*
*/
void
  DisplayEventHandle::update() {
  MSG msg = { };
  while ( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) ) {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
}

/*
*/
PlatformPtr*
DisplayEventHandle::getPlatformPtr() {
  return reinterpret_cast<PlatformPtr*>(WndProc);
}
}

#endif //#if USING(CH_PLATFORM_WIN32)
