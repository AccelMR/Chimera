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
 * Exists so a script saved in an outside code editor runs at once, also during Play: every
 * Editor.ScriptReloadInterval seconds, and when the editor window gets the focus back, it
 * compares the write time of every loaded script with the last one seen and asks the
 * ScriptManager to read the changed ones again.
 */
class ScriptHotReload
{
 public:
  /**
   * Listens for the window focus. Needs the event dispatcher.
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
