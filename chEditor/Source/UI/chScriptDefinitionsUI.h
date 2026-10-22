/************************************************************************/
/**
 * @file chScriptDefinitionsUI.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Writes the script API for code editors when the project opens.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so code editors complete and check the project's scripts: when the project opens it
 * writes the LuaLS definitions of everything the engine gives scripts into
 * <Project>/Saved/ScriptDefinitions, and a .luarc.json that points LuaLS there when the
 * project has none, with a small window in the corner that says it is happening. Scripts may
 * live in any folder; LuaLS reads the definitions for the whole project folder.
 */
class ScriptDefinitionsUI
{
 public:
  /**
   * Writes the files on the next frames that draw the UI.
   */
  static void
  requestUpdate();

  /**
   * Call once per frame, after the panels.
   */
  static void
  render();
};

} // namespace chEngineSDK
