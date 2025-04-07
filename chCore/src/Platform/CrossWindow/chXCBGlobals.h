/************************************************************************/
/**
 * @file chXCBGlobals.h
 * @author AccelMR
 * @date 2025/04/06
 * @brief XCB specific globals.
 * @details This file contains XCB specific globals and functions.
 *         It is used because in Linux CrossWindow doesn't work well with inputs,
 *          so we need to use XCB directly.
 */
/************************************************************************/
#pragma once
#include "chPrerequisitesCore.h"

#ifdef CH_CROSS_WINDOW
#if USING(CH_PLATFORM_LINUX)
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

namespace chEngineSDK {
  xcb_connection_t* 
  getXCBConnection();

  xcb_screen_t* 
  getXCBScreen();

  void 
  setXCBConnection(xcb_connection_t* connection);

  void 
  setXCBScreen(xcb_screen_t* screen);

  xcb_key_symbols_t* 
  getXCBKeySymbols();

  void 
  initXCBKeySymbols();

  void 
  cleanupXCBKeySymbols();

  void
  cleanupXCB();
}
#endif //USING(CH_PLATFORM_LINUX)
#endif //CH_CROSS_WINDOW