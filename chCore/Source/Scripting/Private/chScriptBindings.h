/************************************************************************/
/**
 * @file chScriptBindings.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The engine types and functions every script sees.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

struct lua_State;

namespace chEngineSDK {

/**
 * Exists so each area of the engine registers its script API in its own file, and the script
 * runtime only calls them when it starts.
 */
class ScriptBindings
{
 public:
  /**
   * Vector2, Vector3, Vector4, Quaternion, Rotator, Color.
   */
  static void
  registerMath(lua_State* state);

  /**
   * GameObject, Transform and Scene.find.
   */
  static void
  registerObjects(lua_State* state);
};

} // namespace chEngineSDK
