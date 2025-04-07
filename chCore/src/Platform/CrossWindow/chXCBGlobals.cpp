/************************************************************************/
/**
 * @file chXCBGlobals.cpp
 * @author AccelMR
 * @date 2025/04/06
 * @brief XCB specific globals.
 * @details This file contains XCB specific globals and functions.
 *         It is used because in Linux CrossWindow doesn't work well with inputs,
 *          so we need to use XCB directly.
 */
/************************************************************************/
#include "chXCBGlobals.h"

#ifdef CH_CROSS_WINDOW
#if USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {

static xcb_connection_t* g_connection = nullptr;
static xcb_screen_t* g_screen = nullptr;
static xcb_key_symbols_t* g_keysyms = nullptr;

xcb_connection_t* 
getXCBConnection() {
  return g_connection;
}

xcb_screen_t*
getXCBScreen() {
  return g_screen;
}

void 
setXCBConnection(xcb_connection_t* connection) {
  g_connection = connection;
}

void 
setXCBScreen(xcb_screen_t* screen) {
  g_screen = screen;
}

xcb_key_symbols_t* 
getXCBKeySymbols() {
  return g_keysyms;
}

void
initXCBKeySymbols() {
  if (g_connection && !g_keysyms) {
    g_keysyms = xcb_key_symbols_alloc(g_connection);
  }
}

void
cleanupXCBKeySymbols() {
  if (g_keysyms) {
    xcb_key_symbols_free(g_keysyms);
    g_keysyms = nullptr;
  }
}

void
cleanupXCB() {
  if (g_connection) {
    xcb_disconnect(g_connection);
    g_connection = nullptr;
  }

  if (g_screen) {
    g_screen = nullptr;
  }
}

} // namespace chEngineSDK

#endif //USING(CH_PLATFORM_LINUX)
#endif //CH_CROSS_WINDOW
