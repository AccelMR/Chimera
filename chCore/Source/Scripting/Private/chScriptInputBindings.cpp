/************************************************************************/
/**
 * @file chScriptInputBindings.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Game input for scripts: actions, keys, rebinding, rumble and cursor.
 */
/************************************************************************/
#include "chScriptBindings.h"

#include "chInputManager.h"
#include "chScriptManager.h"
#include "chScriptMathTypes.h"

namespace chEngineSDK {
namespace {
// Indexed by the enums' values.
const ANSICHAR* const kActionTypeNames[] = {"Button", "Axis1D", "Axis2D"};
const ANSICHAR* const kGamepadTypeNames[] = {"Unknown", "Standard", "Xbox", "PlayStation",
                                             "Nintendo"};
const ANSICHAR* const kCursorModeNames[] = {"Normal", "Hidden", "Locked", nullptr};

InputManager&
getInput(lua_State* state)
{
  InputManager* const input = ScriptManager::instance().getInputManager();
  if (!input) {
    luaL_error(state, "input is not available");
  }
  return *input;
}

/*
 * The id findAction gave, or the action's name (a search, for scripts that do not keep ids).
 */
InputActionId
checkAction(lua_State* state, const InputManager& input, const int32 index)
{
  if (lua_type(state, index) == LUA_TNUMBER) {
    const lua_Integer id = luaL_checkinteger(state, index);
    luaL_argcheck(state, id >= 0 && id < static_cast<lua_Integer>(input.getActionCount()),
                  index, "unknown input action id");
    return static_cast<InputActionId>(id);
  }
  const ANSICHAR* const name = luaL_checkstring(state, index);
  const InputActionId id = input.findAction(name);
  if (id == InputManager::INVALID_ACTION) {
    luaL_error(state, "no input action named '%s'", name);
  }
  return id;
}

/*
 * Scripts count players from 1, as Lua counts; missing means the first.
 */
uint32
checkPlayer(lua_State* state, const int32 index)
{
  if (lua_isnoneornil(state, index)) {
    return 0;
  }
  const lua_Integer player = luaL_checkinteger(state, index);
  luaL_argcheck(state, player >= 1 && player <= InputManager::MAX_PLAYERS, index,
                "players go from 1 to 4");
  return static_cast<uint32>(player - 1);
}

/*
 * A number from the Key table, or the key's name (a search).
 */
InputKey
checkKey(lua_State* state, const int32 index)
{
  if (lua_type(state, index) == LUA_TNUMBER) {
    const lua_Integer key = luaL_checkinteger(state, index);
    luaL_argcheck(state, key > 0 && key < InputKeyUtils::KEY_COUNT, index, "unknown key");
    return static_cast<InputKey>(key);
  }
  const ANSICHAR* const name = luaL_checkstring(state, index);
  const InputKey key = InputKeyUtils::fromName(name);
  if (key == InputKey::None) {
    luaL_error(state, "no key named '%s'", name);
  }
  return key;
}

uint32
checkGamepadSlot(lua_State* state, const int32 index)
{
  const lua_Integer slot = luaL_checkinteger(state, index);
  luaL_argcheck(state, slot >= 1 && slot <= InputState::MAX_GAMEPADS, index,
                "gamepad slots go from 1 to 8");
  return static_cast<uint32>(slot - 1);
}

/*
 * One gamepad slot, from 1; missing means every gamepad.
 */
uint32
checkGamepadMask(lua_State* state, const int32 index)
{
  if (lua_isnoneornil(state, index)) {
    return InputState::ALL_GAMEPADS;
  }
  return 1u << checkGamepadSlot(state, index);
}

/*
 * The optional arguments after the first one, in any order: a number is a player or a gamepad
 * slot, a Vector2 is filled and returned instead of a new one (no allocation every frame).
 */
struct Vector2Arguments
{
  int32 numberIndex = 0;
  int32 outIndex = 0;
};

Vector2Arguments
readVector2Arguments(lua_State* state, const int32 first)
{
  Vector2Arguments arguments;
  const int32 top = lua_gettop(state);
  for (int32 index = first; index <= top; ++index) {
    const int32 type = lua_type(state, index);
    if (type == LUA_TNUMBER) {
      arguments.numberIndex = index;
    }
    else if (type == LUA_TUSERDATA) {
      arguments.outIndex = index;
    }
    else if (type != LUA_TNIL) {
      luaL_argerror(state, index, "expected a number or a Vector2");
    }
  }
  return arguments;
}

int32
pushVector2(lua_State* state, const Vector2& value, const int32 outIndex)
{
  if (outIndex != 0) {
    ScriptTraits<Vector2>::check(state, outIndex) = value;
    lua_pushvalue(state, outIndex);
    return 1;
  }
  ScriptTraits<Vector2>::push(state, value);
  return 1;
}

/*
 * Actions.
 */
int32
findAction(lua_State* state)
{
  const InputActionId id = getInput(state).findAction(ScriptTraits<StringView>::check(state, 1));
  if (id == InputManager::INVALID_ACTION) {
    lua_pushnil(state);
  }
  else {
    lua_pushinteger(state, static_cast<lua_Integer>(id));
  }
  return 1;
}

int32
getActionCount(lua_State* state)
{
  lua_pushinteger(state, static_cast<lua_Integer>(getInput(state).getActionCount()));
  return 1;
}

int32
getActionName(lua_State* state)
{
  const InputManager& input = getInput(state);
  ScriptTraits<String>::push(state, input.getActionName(checkAction(state, input, 1)));
  return 1;
}

int32
getActionType(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputActionType type = input.getActionType(checkAction(state, input, 1));
  lua_pushstring(state, kActionTypeNames[static_cast<SIZE_T>(type)]);
  return 1;
}

int32
getActionContext(lua_State* state)
{
  const InputManager& input = getInput(state);
  ScriptTraits<String>::push(state, input.getActionContext(checkAction(state, input, 1)));
  return 1;
}

int32
isActionDown(lua_State* state)
{
  const InputManager& input = getInput(state);
  lua_pushboolean(state, input.isActionDown(checkAction(state, input, 1),
                                            checkPlayer(state, 2)) ? 1 : 0);
  return 1;
}

int32
wasActionPressed(lua_State* state)
{
  const InputManager& input = getInput(state);
  lua_pushboolean(state, input.wasActionPressed(checkAction(state, input, 1),
                                                checkPlayer(state, 2)) ? 1 : 0);
  return 1;
}

int32
wasActionReleased(lua_State* state)
{
  const InputManager& input = getInput(state);
  lua_pushboolean(state, input.wasActionReleased(checkAction(state, input, 1),
                                                 checkPlayer(state, 2)) ? 1 : 0);
  return 1;
}

int32
getActionValue(lua_State* state)
{
  const InputManager& input = getInput(state);
  lua_pushnumber(state, static_cast<lua_Number>(input.getActionValue(
                            checkAction(state, input, 1), checkPlayer(state, 2))));
  return 1;
}

int32
getActionValue2D(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputActionId action = checkAction(state, input, 1);
  const Vector2Arguments arguments = readVector2Arguments(state, 2);
  const uint32 player = arguments.numberIndex ? checkPlayer(state, arguments.numberIndex) : 0;
  return pushVector2(state, input.getActionValue2D(action, player), arguments.outIndex);
}

int32
setContextEnabled(lua_State* state)
{
  getInput(state).setContextEnabled(ScriptTraits<StringView>::check(state, 1),
                                    lua_toboolean(state, 2) != 0);
  return 0;
}

int32
isContextEnabled(lua_State* state)
{
  lua_pushboolean(state,
                  getInput(state).isContextEnabled(ScriptTraits<StringView>::check(state, 1))
                      ? 1
                      : 0);
  return 1;
}

/*
 * Keys. Like the actions they read as up while game input is off (the editor outside Play or
 * without the viewport's focus), so a script never reacts to typing in a panel.
 */
int32
isKeyDown(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  const uint32 gamepads = checkGamepadMask(state, 2);
  lua_pushboolean(state,
                  input.isGameInputEnabled() && input.getState().isDown(key, gamepads) ? 1 : 0);
  return 1;
}

int32
wasKeyPressed(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  const uint32 gamepads = checkGamepadMask(state, 2);
  lua_pushboolean(
      state, input.isGameInputEnabled() && input.getState().wasPressed(key, gamepads) ? 1 : 0);
  return 1;
}

int32
wasKeyReleased(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  const uint32 gamepads = checkGamepadMask(state, 2);
  lua_pushboolean(
      state, input.isGameInputEnabled() && input.getState().wasReleased(key, gamepads) ? 1 : 0);
  return 1;
}

int32
getAxis(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  const uint32 gamepads = checkGamepadMask(state, 2);
  const float value = input.isGameInputEnabled() ? input.getState().getAxis(key, gamepads)
                                                 : 0.0f;
  lua_pushnumber(state, static_cast<lua_Number>(value));
  return 1;
}

int32
getAxis2D(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  const Vector2Arguments arguments = readVector2Arguments(state, 2);
  const uint32 gamepads = arguments.numberIndex
                              ? 1u << checkGamepadSlot(state, arguments.numberIndex)
                              : InputState::ALL_GAMEPADS;
  const Vector2 value = input.isGameInputEnabled()
                            ? input.getState().getAxis2D(key, gamepads)
                            : Vector2::ZERO;
  return pushVector2(state, value, arguments.outIndex);
}

/*
 * The position is where the mouse is, not something the player does, so it is not hidden
 * while game input is off.
 */
int32
getMousePosition(lua_State* state)
{
  const Vector2Arguments arguments = readVector2Arguments(state, 1);
  return pushVector2(state, getInput(state).getState().getMousePosition(), arguments.outIndex);
}

int32
getMouseDelta(lua_State* state)
{
  const InputManager& input = getInput(state);
  const Vector2Arguments arguments = readVector2Arguments(state, 1);
  return pushVector2(state,
                     input.isGameInputEnabled() ? input.getState().getMouseDelta()
                                                : Vector2::ZERO,
                     arguments.outIndex);
}

int32
getMouseWheel(lua_State* state)
{
  const InputManager& input = getInput(state);
  const Vector2Arguments arguments = readVector2Arguments(state, 1);
  return pushVector2(state,
                     input.isGameInputEnabled() ? input.getState().getMouseWheel()
                                                : Vector2::ZERO,
                     arguments.outIndex);
}

int32
isGameInputEnabled(lua_State* state)
{
  lua_pushboolean(state, getInput(state).isGameInputEnabled() ? 1 : 0);
  return 1;
}

int32
getKeyName(lua_State* state)
{
  ScriptTraits<StringView>::push(state, InputKeyUtils::getName(checkKey(state, 1)));
  return 1;
}

int32
getKeyDisplayName(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputKey key = checkKey(state, 1);
  ScriptTraits<String>::push(state, input.getKeyDisplayName(key, checkPlayer(state, 2)));
  return 1;
}

/*
 * Gamepads and players.
 */
int32
isGamepadConnected(lua_State* state)
{
  const InputManager& input = getInput(state);
  const uint32 slot = checkGamepadSlot(state, 1);
  lua_pushboolean(state, input.getState().getGamepad(slot).bConnected ? 1 : 0);
  return 1;
}

int32
getGamepadType(lua_State* state)
{
  const InputManager& input = getInput(state);
  const GamepadType type = input.getState().getGamepad(checkGamepadSlot(state, 1)).type;
  lua_pushstring(state, kGamepadTypeNames[static_cast<SIZE_T>(type)]);
  return 1;
}

int32
getLastGamepad(lua_State* state)
{
  const int32 slot = getInput(state).getState().getLastGamepad();
  if (slot == InputState::NO_GAMEPAD) {
    lua_pushnil(state);
  }
  else {
    lua_pushinteger(state, static_cast<lua_Integer>(slot + 1));
  }
  return 1;
}

int32
getPlayerCount(lua_State* state)
{
  lua_pushinteger(state, static_cast<lua_Integer>(getInput(state).getPlayerCount()));
  return 1;
}

int32
setPlayerCount(lua_State* state)
{
  InputManager& input = getInput(state);
  const lua_Integer count = luaL_checkinteger(state, 1);
  luaL_argcheck(state, count >= 1 && count <= InputManager::MAX_PLAYERS, 1,
                "players go from 1 to 4");
  input.setPlayerCount(static_cast<uint32>(count));
  return 0;
}

/*
 * setPlayerDevices(player, keyboardAndMouse, {gamepad slots}); the list may be missing.
 */
int32
setPlayerDevices(lua_State* state)
{
  InputManager& input = getInput(state);
  const uint32 player = checkPlayer(state, 1);
  const bool bKeyboardMouse = lua_toboolean(state, 2) != 0;
  uint32 gamepadMask = 0;
  if (!lua_isnoneornil(state, 3)) {
    luaL_checktype(state, 3, LUA_TTABLE);
    const lua_Integer count = luaL_len(state, 3);
    for (lua_Integer i = 1; i <= count; ++i) {
      lua_geti(state, 3, i);
      gamepadMask |= 1u << checkGamepadSlot(state, -1);
      lua_pop(state, 1);
    }
  }
  input.setPlayerDevices(player, bKeyboardMouse, gamepadMask);
  return 0;
}

/*
 * Rebinding. Bindings are written as in Input.ini ("W:+Y", "GamepadRightStick*4") and their
 * index counts from 1. Menus call these, not every frame, so they may make tables.
 */
int32
pushBindings(lua_State* state, const Span<const InputBinding> bindings,
             const InputActionType type)
{
  lua_createtable(state, static_cast<int32>(bindings.size()), 0);
  for (SIZE_T i = 0; i < bindings.size(); ++i) {
    ScriptTraits<String>::push(state, InputManager::bindingToString(bindings[i], type));
    lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
  }
  return 1;
}

int32
getBindings(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputActionId action = checkAction(state, input, 1);
  return pushBindings(state, input.getBindings(action), input.getActionType(action));
}

int32
getDefaultBindings(lua_State* state)
{
  const InputManager& input = getInput(state);
  const InputActionId action = checkAction(state, input, 1);
  return pushBindings(state, input.getDefaultBindings(action), input.getActionType(action));
}

/*
 * setBinding(action, index, text): index one past the last adds a binding. False when the
 * binding does not fit the action's type.
 */
int32
setBinding(lua_State* state)
{
  InputManager& input = getInput(state);
  const InputActionId action = checkAction(state, input, 1);
  const lua_Integer index = luaL_checkinteger(state, 2);
  const lua_Integer count = static_cast<lua_Integer>(input.getBindings(action).size());
  luaL_argcheck(state, index >= 1 && index <= count + 1, 2, "binding index out of range");
  InputBinding binding;
  if (!InputManager::parseBinding(ScriptTraits<StringView>::check(state, 3), binding)) {
    return luaL_argerror(state, 3, "not a binding (Key[:Target][*Scale])");
  }
  lua_pushboolean(state, input.setBinding(action, static_cast<uint32>(index - 1), binding));
  return 1;
}

int32
removeBinding(lua_State* state)
{
  InputManager& input = getInput(state);
  const InputActionId action = checkAction(state, input, 1);
  const lua_Integer index = luaL_checkinteger(state, 2);
  const bool bRemoved =
      index >= 1 && input.removeBinding(action, static_cast<uint32>(index - 1));
  lua_pushboolean(state, bRemoved ? 1 : 0);
  return 1;
}

int32
resetBindings(lua_State* state)
{
  InputManager& input = getInput(state);
  input.resetBindings(checkAction(state, input, 1));
  return 0;
}

int32
resetAllBindings(lua_State* state)
{
  getInput(state).resetAllBindings();
  return 0;
}

int32
findActionsUsingKey(lua_State* state)
{
  const InputManager& input = getInput(state);
  Vector<InputActionId> actions;
  input.findActionsUsingKey(checkKey(state, 1), actions);
  lua_createtable(state, static_cast<int32>(actions.size()), 0);
  for (SIZE_T i = 0; i < actions.size(); ++i) {
    lua_pushinteger(state, static_cast<lua_Integer>(actions[i]));
    lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
  }
  return 1;
}

int32
saveUserBindings(lua_State* state)
{
  lua_pushboolean(state, getInput(state).saveUserBindings() ? 1 : 0);
  return 1;
}

/*
 * captureNextKey(function(key) ... end): the key is a number of the Key table. A new capture
 * replaces the one waiting, whose function is never called.
 */
int32
captureNextKey(lua_State* state)
{
  getInput(state);
  luaL_checktype(state, 1, LUA_TFUNCTION);
  lua_settop(state, 1);
  ScriptManager::instance().captureNextKey(luaL_ref(state, LUA_REGISTRYINDEX));
  return 0;
}

int32
cancelKeyCapture(lua_State* state)
{
  getInput(state).cancelKeyCapture();
  return 0;
}

int32
isCapturingKey(lua_State* state)
{
  lua_pushboolean(state, getInput(state).isCapturingKey() ? 1 : 0);
  return 1;
}

/*
 * Feedback.
 */
int32
rumble(lua_State* state)
{
  InputManager& input = getInput(state);
  const uint32 player = checkPlayer(state, 1);
  const float low = ScriptTraits<float>::check(state, 2);
  const float high = ScriptTraits<float>::check(state, 3);
  const uint32 milliseconds = ScriptTraits<uint32>::check(state, 4);
  lua_pushboolean(state, input.rumble(player, low, high, milliseconds) ? 1 : 0);
  return 1;
}

int32
setCursorMode(lua_State* state)
{
  InputManager& input = getInput(state);
  input.setCursorMode(static_cast<CursorMode>(luaL_checkoption(state, 1, nullptr,
                                                               kCursorModeNames)));
  return 0;
}

int32
getCursorMode(lua_State* state)
{
  lua_pushstring(state,
                 kCursorModeNames[static_cast<SIZE_T>(getInput(state).getCursorMode())]);
  return 1;
}
} // namespace

/*
 */
void
ScriptBindings::registerInput(lua_State* state)
{
  ScriptApiDescription* const api = ScriptApiDescription::find(state);
  if (api) {
    api->addAlias("InputAction", "integer|string");
    api->addAlias("InputKey", "Key|string");
    api->addAlias("InputActionType", "\"Button\"|\"Axis1D\"|\"Axis2D\"");
    api->addAlias("GamepadType",
                  "\"Unknown\"|\"Standard\"|\"Xbox\"|\"PlayStation\"|\"Nintendo\"");
    api->addAlias("CursorMode", "\"Normal\"|\"Hidden\"|\"Locked\"");
  }

  const ANSICHAR* const kActionButton = "fun(action: InputAction, player?: integer): boolean";
  const ANSICHAR* const kKeyButton = "fun(key: InputKey, gamepad?: integer): boolean";
  const ANSICHAR* const kMouse = "fun(out?: Vector2): Vector2";
  ScriptTable(state, "Input",
              "Game input. Actions come from Input.ini; look their ids up once with "
              "findAction. Players and gamepads count from 1.")
      .rawFunction("findAction", &findAction, "fun(name: string): integer?")
      .rawFunction("getActionCount", &getActionCount, "fun(): integer")
      .rawFunction("getActionName", &getActionName, "fun(action: InputAction): string")
      .rawFunction("getActionType", &getActionType,
                   "fun(action: InputAction): InputActionType")
      .rawFunction("getActionContext", &getActionContext, "fun(action: InputAction): string")
      .rawFunction("isActionDown", &isActionDown, kActionButton)
      .rawFunction("wasActionPressed", &wasActionPressed, kActionButton)
      .rawFunction("wasActionReleased", &wasActionReleased, kActionButton)
      .rawFunction("getActionValue", &getActionValue,
                   "fun(action: InputAction, player?: integer): number")
      .rawFunction("getActionValue2D", &getActionValue2D,
                   "fun(action: InputAction, out?: Vector2, player?: integer): Vector2",
                   "fun(action: InputAction, player: integer, out?: Vector2): Vector2")
      .rawFunction("setContextEnabled", &setContextEnabled,
                   "fun(context: string, enabled: boolean)")
      .rawFunction("isContextEnabled", &isContextEnabled, "fun(context: string): boolean")
      .rawFunction("isKeyDown", &isKeyDown, kKeyButton)
      .rawFunction("wasKeyPressed", &wasKeyPressed, kKeyButton)
      .rawFunction("wasKeyReleased", &wasKeyReleased, kKeyButton)
      .rawFunction("getAxis", &getAxis, "fun(key: InputKey, gamepad?: integer): number")
      .rawFunction("getAxis2D", &getAxis2D,
                   "fun(key: InputKey, out?: Vector2, gamepad?: integer): Vector2",
                   "fun(key: InputKey, gamepad: integer, out?: Vector2): Vector2")
      .rawFunction("getMousePosition", &getMousePosition, kMouse)
      .rawFunction("getMouseDelta", &getMouseDelta, kMouse)
      .rawFunction("getMouseWheel", &getMouseWheel, kMouse)
      .rawFunction("isGameInputEnabled", &isGameInputEnabled, "fun(): boolean")
      .rawFunction("getKeyName", &getKeyName, "fun(key: InputKey): string")
      .rawFunction("getKeyDisplayName", &getKeyDisplayName,
                   "fun(key: InputKey, player?: integer): string")
      .rawFunction("isGamepadConnected", &isGamepadConnected, "fun(gamepad: integer): boolean")
      .rawFunction("getGamepadType", &getGamepadType, "fun(gamepad: integer): GamepadType")
      .rawFunction("getLastGamepad", &getLastGamepad, "fun(): integer?")
      .rawFunction("getPlayerCount", &getPlayerCount, "fun(): integer")
      .rawFunction("setPlayerCount", &setPlayerCount, "fun(count: integer)")
      .rawFunction("setPlayerDevices", &setPlayerDevices,
                   "fun(player: integer, keyboardAndMouse: boolean, gamepads?: integer[])")
      .rawFunction("getBindings", &getBindings, "fun(action: InputAction): string[]")
      .rawFunction("getDefaultBindings", &getDefaultBindings,
                   "fun(action: InputAction): string[]")
      .rawFunction("setBinding", &setBinding,
                   "fun(action: InputAction, index: integer, binding: string): boolean")
      .rawFunction("removeBinding", &removeBinding,
                   "fun(action: InputAction, index: integer): boolean")
      .rawFunction("resetBindings", &resetBindings, "fun(action: InputAction)")
      .rawFunction("resetAllBindings", &resetAllBindings, "fun()")
      .rawFunction("findActionsUsingKey", &findActionsUsingKey,
                   "fun(key: InputKey): integer[]")
      .rawFunction("saveUserBindings", &saveUserBindings, "fun(): boolean")
      .rawFunction("captureNextKey", &captureNextKey, "fun(callback: fun(key: Key))")
      .rawFunction("cancelKeyCapture", &cancelKeyCapture, "fun()")
      .rawFunction("isCapturingKey", &isCapturingKey, "fun(): boolean")
      .rawFunction("rumble", &rumble,
                   "fun(player: integer, low: number, high: number, milliseconds: integer): "
                   "boolean")
      .rawFunction("setCursorMode", &setCursorMode, "fun(mode: CursorMode)")
      .rawFunction("getCursorMode", &getCursorMode, "fun(): CursorMode");

  // Key.W, Key.GamepadSouth...: the names Input.ini uses, as numbers so a check costs nothing.
  ScriptApiEnum* const keyEnum = api ? &api->addEnum("Key") : nullptr;
  lua_createtable(state, 0, static_cast<int32>(InputKeyUtils::KEY_COUNT));
  for (uint32 i = 1; i < InputKeyUtils::KEY_COUNT; ++i) {
    const StringView name = InputKeyUtils::getName(static_cast<InputKey>(i));
    ScriptTraits<StringView>::push(state, name);
    lua_pushinteger(state, static_cast<lua_Integer>(i));
    lua_rawset(state, -3);
    if (keyEnum) {
      keyEnum->values.push_back({String(name), static_cast<int64>(i)});
    }
  }
  lua_setglobal(state, "Key");
}

} // namespace chEngineSDK
