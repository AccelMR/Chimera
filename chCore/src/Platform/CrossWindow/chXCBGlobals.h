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
namespace XCBGlobals {
  CH_CORE_EXPORT xcb_connection_t* 
  getXCBConnection();

  CH_CORE_EXPORT xcb_screen_t* 
  getXCBScreen();

  CH_CORE_EXPORT xcb_window_t
  getXCBWindow();

  CH_CORE_EXPORT void
  setXCBWindow(xcb_window_t window);

  CH_CORE_EXPORT void 
  setXCBConnection(xcb_connection_t* connection);

  CH_CORE_EXPORT void 
  setXCBScreen(xcb_screen_t* screen);

  CH_CORE_EXPORT xcb_key_symbols_t* 
  getXCBKeySymbols();

  CH_CORE_EXPORT bool 
  initXCBKeySymbols();

  CH_CORE_EXPORT void 
  cleanupXCBKeySymbols();

  CH_CORE_EXPORT void
  cleanupXCB();
} // namespace XCBGlobals
} // namespace chEngineSDK
#endif //USING(CH_PLATFORM_LINUX)
#endif //CH_CROSS_WINDOW