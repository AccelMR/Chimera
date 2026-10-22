/************************************************************************/
/**
 * @file chScriptBinder.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Exposes C++ types and functions to scripts.
 */
/************************************************************************/
#include "chScriptBinder.h"

namespace chEngineSDK {
namespace ScriptDetail {
namespace {
/*
 * Reads the function pointer pushAccessor stored.
 */
FORCEINLINE lua_CFunction
toAccessor(lua_State* state, const int32 index)
{
  lua_CFunction accessor = nullptr;
  std::memcpy(&accessor, lua_touserdata(state, index), sizeof(accessor));
  return accessor;
}

/*
 * Raises "<type> has no member '<key>'" with the type name kept in upvalue nameUpvalue.
 */
int32
raiseMissingMember(lua_State* state, const int32 nameUpvalue, const ANSICHAR* what)
{
  const ANSICHAR* const typeName = lua_tostring(state, lua_upvalueindex(nameUpvalue));
  const ANSICHAR* const key = luaL_tolstring(state, 2, nullptr);
  return luaL_error(state, "%s has no %s '%s'", typeName, what, key);
}
} // namespace

/*
 */
const ANSICHAR*
getMetamethodName(const ScriptOperator op) noexcept
{
  switch (op) {
  case ScriptOperator::Add:
    return "__add";
  case ScriptOperator::Subtract:
    return "__sub";
  case ScriptOperator::Multiply:
    return "__mul";
  case ScriptOperator::Divide:
    return "__div";
  case ScriptOperator::Negate:
    return "__unm";
  case ScriptOperator::Equal:
    return "__eq";
  case ScriptOperator::ToString:
    return "__tostring";
  }
  return "__tostring";
}

/*
 */
void
pushAccessor(lua_State* state, const lua_CFunction accessor)
{
  void* const data = lua_newuserdatauv(state, sizeof(accessor), 0);
  std::memcpy(data, &accessor, sizeof(accessor));
}

/*
 * Upvalues: the members table, the type name. Stack: value, key.
 */
int32
indexValue(lua_State* state)
{
  lua_pushvalue(state, 2);
  const int32 type = lua_rawget(state, lua_upvalueindex(1));
  if (type == LUA_TUSERDATA) {
    const lua_CFunction getter = toAccessor(state, -1);
    lua_settop(state, 2);
    return getter(state);
  }
  if (type != LUA_TNIL) {
    return 1;
  }
  return raiseMissingMember(state, 2, "member");
}

/*
 * Upvalues: the setters table, the type name. Stack: value, key, new value.
 */
int32
newIndexValue(lua_State* state)
{
  lua_pushvalue(state, 2);
  if (lua_rawget(state, lua_upvalueindex(1)) == LUA_TUSERDATA) {
    const lua_CFunction setter = toAccessor(state, -1);
    lua_settop(state, 3);
    return setter(state);
  }
  return raiseMissingMember(state, 2, "writable member");
}

/*
 * Upvalues: the constructors table (by argument count), the type name. Stack: class table,
 * arguments.
 */
int32
callConstructor(lua_State* state)
{
  const int32 argumentCount = lua_gettop(state) - 1;
  if (lua_rawgeti(state, lua_upvalueindex(1), argumentCount) == LUA_TFUNCTION) {
    const lua_CFunction constructor = lua_tocfunction(state, -1);
    lua_pop(state, 1);
    return constructor(state);
  }
  return luaL_error(state, "%s has no constructor that takes %d arguments",
                    lua_tostring(state, lua_upvalueindex(2)), argumentCount);
}

/*
 * Upvalues: the constants table, the type name. Stack: class table, key. Value constants are
 * copied, so changing one in place never changes the constant.
 */
int32
indexConstant(lua_State* state)
{
  lua_pushvalue(state, 2);
  if (lua_rawget(state, lua_upvalueindex(1)) != LUA_TUSERDATA) {
    return raiseMissingMember(state, 2, "static member");
  }
  const SIZE_T size = lua_rawlen(state, -1);
  void* const copy = lua_newuserdatauv(state, size, 0);
  std::memcpy(copy, lua_touserdata(state, -2), size);
  lua_getmetatable(state, -2);
  lua_setmetatable(state, -2);
  return 1;
}

/*
 */
void
beginValueClass(lua_State* state, const ANSICHAR* name)
{
  luaL_newmetatable(state, name);
  lua_newtable(state); // Members: methods (functions) and getters (accessor userdata)
  lua_newtable(state); // Setters
  lua_newtable(state); // Class
  lua_newtable(state); // Constructors
  lua_newtable(state); // Constants
  const int32 metatable = lua_gettop(state) - 5;
  const int32 members = metatable + 1;
  const int32 setters = metatable + 2;
  const int32 classTable = metatable + 3;
  const int32 constructors = metatable + 4;
  const int32 constants = metatable + 5;

  lua_pushvalue(state, members);
  lua_pushstring(state, name);
  lua_pushcclosure(state, &indexValue, 2);
  lua_setfield(state, metatable, "__index");
  lua_pushvalue(state, setters);
  lua_pushstring(state, name);
  lua_pushcclosure(state, &newIndexValue, 2);
  lua_setfield(state, metatable, "__newindex");
  // getmetatable returns this instead of the table, so scripts cannot reach __index and call
  // the unchecked getters with another value.
  lua_pushstring(state, name);
  lua_setfield(state, metatable, "__metatable");

  lua_createtable(state, 0, 3);
  lua_pushvalue(state, constructors);
  lua_pushstring(state, name);
  lua_pushcclosure(state, &callConstructor, 2);
  lua_setfield(state, -2, "__call");
  lua_pushvalue(state, constants);
  lua_pushstring(state, name);
  lua_pushcclosure(state, &indexConstant, 2);
  lua_setfield(state, -2, "__index");
  lua_pushstring(state, name);
  lua_setfield(state, -2, "__metatable");
  lua_setmetatable(state, classTable);

  lua_pushvalue(state, classTable);
  lua_setglobal(state, name);
}

} // namespace ScriptDetail

/*
 */
ScriptTable::ScriptTable(lua_State* state, const ANSICHAR* name, const ANSICHAR* description)
  : m_state(state),
    m_name(name)
{
  lua_newtable(state);
  m_index = lua_gettop(state);
  if constexpr (ScriptDetail::RECORD_API) {
    if (ScriptApiDescription* api = ScriptApiDescription::find(state)) {
      m_api = &api->addType(name, ScriptApiTypeKind::Library);
      m_api->description = description;
    }
  }
  else {
    ScriptDetail::ignore(description);
  }
}

/*
 */
ScriptTable::~ScriptTable()
{
  lua_settop(m_state, m_index);
  lua_setglobal(m_state, m_name);
}

/*
 */
ScriptTable&
ScriptTable::describeField(const ANSICHAR* name, const ANSICHAR* type)
{
  if (m_api) {
    m_api->constants.push_back({name, type});
  }
  return *this;
}

/*
 */
void
ScriptTable::setFunction(const ANSICHAR* name, const lua_CFunction function,
                         const Span<const ANSICHAR* const> signatures)
{
  lua_pushcfunction(m_state, function);
  lua_setfield(m_state, m_index, name);
  if (m_api) {
    ScriptApiFunction& apiFunction = m_api->functions.emplace_back();
    apiFunction.name = name;
    ScriptApiDescription::addSignatures(apiFunction, signatures);
  }
}
} // namespace chEngineSDK
