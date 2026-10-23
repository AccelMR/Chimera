/************************************************************************/
/**
 * @file chScriptHotReload.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Reads scripts again when their files change.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so a script saved in an outside code editor runs at once, also during Play: the
 * system reports changed .lua files under /Game, and each loaded one is read again by the
 * ScriptManager once Editor.ScriptReloadDelay passed without more changes. When the window
 * gets the focus back, when the system dropped changes, or every few seconds when the folder
 * cannot be watched, the write time of every loaded script is compared instead.
 */
class ScriptHotReload
{
 public:
  /**
   * Watches /Game and listens for the window focus. Needs the event dispatcher.
   */
  static void
  initialize();

  static void
  shutDown();

  /**
   * Call once per frame.
   */
  static void
  update(float deltaTime);
};

} // namespace chEngineSDK
