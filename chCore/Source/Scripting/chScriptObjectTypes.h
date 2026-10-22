/************************************************************************/
/**
 * @file chScriptObjectTypes.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The scene objects scripts reach through handles.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chScriptBinder.h"

namespace chEngineSDK {
class GameObject;
class Transform;

CH_SCRIPT_HANDLE_TYPE(GameObject, "GameObject");
CH_SCRIPT_HANDLE_TYPE(Transform, "Transform");

/**
 * Each GameObject has one handle, made the first time a script sees it and kept until the
 * object dies, so the same object is always the same Lua value (== and table keys work).
 * check raises a script error when the object was destroyed or left its scene.
 */
template<>
struct CH_CORE_EXPORT ScriptTraits<GameObject>
{
  static void
  push(lua_State* state, GameObject& gameObject);

  NODISCARD static GameObject&
  check(lua_State* state, int32 index);

  /**
   * Called when the object dies: scripts that still hold its handle get an error from then
   * on instead of a dangling pointer.
   */
  static void
  release(GameObject& gameObject) noexcept;
};

/**
 * The transform of a GameObject: a handle that shares the object's, so it dies with it.
 */
template<>
struct CH_CORE_EXPORT ScriptTraits<Transform>
{
  /**
   * Pushes the transform of gameObject (a Transform alone does not know its object).
   */
  static void
  pushOf(lua_State* state, GameObject& gameObject);

  NODISCARD static Transform&
  check(lua_State* state, int32 index);

  /**
   * The object whose transform is at index.
   */
  NODISCARD static GameObject&
  checkOwner(lua_State* state, int32 index);
};

} // namespace chEngineSDK
