/************************************************************************/
/**
 * @file chScriptBinder.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Exposes C++ types and functions to scripts.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include <array>
#include <cstring>
#include <tuple>
#include <utility>

// Lua is built as C++ (see the root CMakeLists), so its headers are included as they are;
// lua.hpp would wrap them in extern "C" and name symbols the library does not have.
#include "lauxlib.h"
#include "lua.h"

#include "chAngle.h"
#include "chScriptApi.h"

namespace chEngineSDK {

/**
 * Exists so the binder knows how a C++ type goes to Lua (push) and comes back (check). check
 * raises a Lua error naming the argument when the value has the wrong type, and TYPE_NAME is
 * the type code editors are told (value and handle types use their script name). A type
 * without a specialization stops the build.
 */
template<typename T>
struct ScriptTraits
{
  static_assert(sizeof(T) == 0,
                "This type cannot reach scripts: give it a ScriptTraits specialization or "
                "declare it with CH_SCRIPT_VALUE_TYPE.");
};

/**
 * Exists to mark the small trivially copyable types (vectors, colors) that scripts get as
 * their own copy, a Lua userdata with the members and methods ScriptClass registers. Declare
 * one with CH_SCRIPT_VALUE_TYPE(Type, "Name") inside chEngineSDK.
 */
template<typename T>
struct ScriptValueType
{
  static constexpr bool DECLARED = false;
};

#define CH_SCRIPT_VALUE_TYPE(Type, ScriptName)                                                \
  template<>                                                                                  \
  struct ScriptValueType<Type>                                                                \
  {                                                                                           \
    static constexpr bool DECLARED = true;                                                    \
    static constexpr const ANSICHAR* NAME = ScriptName;                                       \
  }

template<typename T>
concept ScriptValue = ScriptValueType<T>::DECLARED;

/**
 * Exists to mark engine objects that scripts reach through a handle instead of a copy (a
 * GameObject): the userdata only points at the object, which may die first, so its
 * ScriptTraits checks the object is still alive. Declare one with
 * CH_SCRIPT_HANDLE_TYPE(Type, "Name") and write its ScriptTraits by hand.
 */
template<typename T>
struct ScriptHandleType
{
  static constexpr bool DECLARED = false;
};

#define CH_SCRIPT_HANDLE_TYPE(Type, ScriptName)                                               \
  template<>                                                                                  \
  struct ScriptHandleType<Type>                                                               \
  {                                                                                           \
    static constexpr bool DECLARED = true;                                                    \
    static constexpr const ANSICHAR* NAME = ScriptName;                                       \
  }

template<typename T>
concept ScriptHandle = ScriptHandleType<T>::DECLARED;

/*
 * The name scripts know a value or handle type by.
 */
template<typename T>
  requires(ScriptValue<T> || ScriptHandle<T>)
consteval const ANSICHAR*
getScriptTypeName()
{
  if constexpr (ScriptValue<T>) {
    return ScriptValueType<T>::NAME;
  }
  else {
    return ScriptHandleType<T>::NAME;
  }
}

template<ScriptValue T>
struct ScriptTraits<T>
{
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                "A script value type is copied as bytes and never destroyed.");
  // Lua aligns userdata to 8 bytes.
  static_assert(alignof(T) <= 8, "A script value type cannot need more than 8-byte alignment.");

  static FORCEINLINE void
  push(lua_State* state, const T& value)
  {
    void* const data = lua_newuserdatauv(state, sizeof(T), 0);
    std::memcpy(data, &value, sizeof(T));
    luaL_setmetatable(state, ScriptValueType<T>::NAME);
  }

  NODISCARD static FORCEINLINE T&
  check(lua_State* state, const int32 index)
  {
    return *static_cast<T*>(luaL_checkudata(state, index, ScriptValueType<T>::NAME));
  }
};

template<>
struct ScriptTraits<float>
{
  static constexpr const ANSICHAR* TYPE_NAME = "number";

  static FORCEINLINE void
  push(lua_State* state, const float value)
  {
    lua_pushnumber(state, static_cast<lua_Number>(value));
  }

  NODISCARD static FORCEINLINE float
  check(lua_State* state, const int32 index)
  {
    return static_cast<float>(luaL_checknumber(state, index));
  }
};

template<>
struct ScriptTraits<double>
{
  static constexpr const ANSICHAR* TYPE_NAME = "number";

  static FORCEINLINE void
  push(lua_State* state, const double value)
  {
    lua_pushnumber(state, static_cast<lua_Number>(value));
  }

  NODISCARD static FORCEINLINE double
  check(lua_State* state, const int32 index)
  {
    return static_cast<double>(luaL_checknumber(state, index));
  }
};

template<>
struct ScriptTraits<int32>
{
  static constexpr const ANSICHAR* TYPE_NAME = "integer";

  static FORCEINLINE void
  push(lua_State* state, const int32 value)
  {
    lua_pushinteger(state, static_cast<lua_Integer>(value));
  }

  NODISCARD static FORCEINLINE int32
  check(lua_State* state, const int32 index)
  {
    const lua_Integer value = luaL_checkinteger(state, index);
    luaL_argcheck(state, static_cast<lua_Integer>(static_cast<int32>(value)) == value, index,
                  "out of range");
    return static_cast<int32>(value);
  }
};

template<>
struct ScriptTraits<uint32>
{
  static constexpr const ANSICHAR* TYPE_NAME = "integer";

  static FORCEINLINE void
  push(lua_State* state, const uint32 value)
  {
    lua_pushinteger(state, static_cast<lua_Integer>(value));
  }

  NODISCARD static FORCEINLINE uint32
  check(lua_State* state, const int32 index)
  {
    const lua_Integer value = luaL_checkinteger(state, index);
    luaL_argcheck(state, static_cast<lua_Integer>(static_cast<uint32>(value)) == value, index,
                  "out of range");
    return static_cast<uint32>(value);
  }
};

/**
 * Any value counts, as in a Lua condition: only nil and false are false.
 */
template<>
struct ScriptTraits<bool>
{
  static constexpr const ANSICHAR* TYPE_NAME = "boolean";

  static FORCEINLINE void
  push(lua_State* state, const bool value)
  {
    lua_pushboolean(state, value ? 1 : 0);
  }

  NODISCARD static FORCEINLINE bool
  check(lua_State* state, const int32 index)
  {
    return lua_toboolean(state, index) != 0;
  }
};

/**
 * The view points into the Lua string, which lives while it is on the stack, that is, for the
 * whole call.
 */
template<>
struct ScriptTraits<StringView>
{
  static constexpr const ANSICHAR* TYPE_NAME = "string";

  static FORCEINLINE void
  push(lua_State* state, const StringView value)
  {
    lua_pushlstring(state, value.data(), value.size());
  }

  NODISCARD static FORCEINLINE StringView
  check(lua_State* state, const int32 index)
  {
    SIZE_T size = 0;
    const ANSICHAR* const text = luaL_checklstring(state, index, &size);
    return StringView(text, size);
  }
};

template<>
struct ScriptTraits<String>
{
  static constexpr const ANSICHAR* TYPE_NAME = "string";

  static FORCEINLINE void
  push(lua_State* state, const String& value)
  {
    lua_pushlstring(state, value.data(), value.size());
  }

  NODISCARD static FORCEINLINE String
  check(lua_State* state, const int32 index)
  {
    return String(ScriptTraits<StringView>::check(state, index));
  }
};

template<>
struct ScriptTraits<const ANSICHAR*>
{
  static constexpr const ANSICHAR* TYPE_NAME = "string";

  static FORCEINLINE void
  push(lua_State* state, const ANSICHAR* value)
  {
    lua_pushstring(state, value);
  }

  NODISCARD static FORCEINLINE const ANSICHAR*
  check(lua_State* state, const int32 index)
  {
    return luaL_checkstring(state, index);
  }
};

/**
 * Scripts write angles as plain numbers: degrees here, radians for Radian.
 */
template<>
struct ScriptTraits<Degree>
{
  static constexpr const ANSICHAR* TYPE_NAME = "number";

  static FORCEINLINE void
  push(lua_State* state, const Degree& value)
  {
    lua_pushnumber(state, static_cast<lua_Number>(value.valueDegree()));
  }

  NODISCARD static FORCEINLINE Degree
  check(lua_State* state, const int32 index)
  {
    return Degree(static_cast<float>(luaL_checknumber(state, index)));
  }
};

template<>
struct ScriptTraits<Radian>
{
  static constexpr const ANSICHAR* TYPE_NAME = "number";

  static FORCEINLINE void
  push(lua_State* state, const Radian& value)
  {
    lua_pushnumber(state, static_cast<lua_Number>(value.valueRadian()));
  }

  NODISCARD static FORCEINLINE Radian
  check(lua_State* state, const int32 index)
  {
    return Radian(static_cast<float>(luaL_checknumber(state, index)));
  }
};

/**
 * Lua metamethods ScriptClass can give a value type.
 */
enum class ScriptOperator : uint8 {
  Add,
  Subtract,
  Multiply,
  Divide,
  Negate,
  Equal,
  ToString
};

namespace ScriptDetail {
template<typename Function>
struct FunctionTraits;

template<typename R, typename... A>
struct FunctionTraits<R (*)(A...)>
{
  using Return = R;
  using Class = void;
  using Arguments = std::tuple<A...>;
};

template<typename R, typename... A>
struct FunctionTraits<R (*)(A...) noexcept> : FunctionTraits<R (*)(A...)>
{};

template<typename R, typename C, typename... A>
struct FunctionTraits<R (C::*)(A...)>
{
  using Return = R;
  using Class = C;
  using Arguments = std::tuple<A...>;
};

template<typename R, typename C, typename... A>
struct FunctionTraits<R (C::*)(A...) noexcept> : FunctionTraits<R (C::*)(A...)>
{};

template<typename R, typename C, typename... A>
struct FunctionTraits<R (C::*)(A...) const> : FunctionTraits<R (C::*)(A...)>
{};

template<typename R, typename C, typename... A>
struct FunctionTraits<R (C::*)(A...) const noexcept> : FunctionTraits<R (C::*)(A...)>
{};

template<typename Member>
struct MemberTraits;

template<typename V, typename C>
struct MemberTraits<V C::*>
{
  using Value = V;
  using Class = C;
};

template<typename T>
struct IsTuple : std::false_type
{};

template<typename... A>
struct IsTuple<std::tuple<A...>> : std::true_type
{};

template<typename T>
using Traits = ScriptTraits<std::remove_cvref_t<T>>;

template<typename Arguments, SIZE_T I>
using Argument = std::tuple_element_t<I, Arguments>;

/*
 * A function that returns a reference to the value it was called on gives back that same
 * Lua value, so in-place methods chain (v:sub(a, b):normalize()) without making copies.
 */
template<typename R, typename Call>
FORCEINLINE int32
callAndPush(lua_State* state, Call&& call)
{
  if constexpr (std::is_void_v<R>) {
    call();
    return 0;
  }
  else if constexpr (std::is_lvalue_reference_v<R> && ScriptValue<std::remove_cvref_t<R>>) {
    R result = call();
    if (lua_touserdata(state, 1) == static_cast<const void*>(&result)) {
      lua_pushvalue(state, 1);
    }
    else {
      Traits<R>::push(state, result);
    }
    return 1;
  }
  else if constexpr (IsTuple<std::remove_cvref_t<R>>::value) {
    const R results = call();
    std::apply(
        [state](const auto&... values) {
          (Traits<decltype(values)>::push(state, values), ...);
        },
        results);
    return static_cast<int32>(std::tuple_size_v<std::remove_cvref_t<R>>);
  }
  else {
    Traits<R>::push(state, call());
    return 1;
  }
}

/*
 * Free and static functions read their arguments from index 1; a free function whose first
 * parameter is the value type works as a method, since Lua passes the value first.
 */
template<auto Function, int32 FirstIndex, SIZE_T... I>
FORCEINLINE int32
callFree(lua_State* state, std::index_sequence<I...>)
{
  using FunctionInfo = FunctionTraits<decltype(Function)>;
  using Arguments = typename FunctionInfo::Arguments;
  // [&]: a function without arguments never reads state, and an unused explicit capture warns.
  return callAndPush<typename FunctionInfo::Return>(state, [&]() -> decltype(auto) {
    return Function(
        Traits<Argument<Arguments, I>>::check(state, static_cast<int32>(I) + FirstIndex)...);
  });
}

template<auto Function, SIZE_T... I>
FORCEINLINE int32
callMember(lua_State* state, std::index_sequence<I...>)
{
  using FunctionInfo = FunctionTraits<decltype(Function)>;
  using Arguments = typename FunctionInfo::Arguments;
  using Class = typename FunctionInfo::Class;
  return callAndPush<typename FunctionInfo::Return>(state, [state]() -> decltype(auto) {
    Class& self = ScriptTraits<Class>::check(state, 1);
    return (self.*Function)(
        Traits<Argument<Arguments, I>>::check(state, static_cast<int32>(I) + 2)...);
  });
}

template<auto Function>
int32
callFunction(lua_State* state)
{
  using FunctionInfo = FunctionTraits<decltype(Function)>;
  constexpr SIZE_T kArgumentCount = std::tuple_size_v<typename FunctionInfo::Arguments>;
  if constexpr (std::is_void_v<typename FunctionInfo::Class>) {
    return callFree<Function, 1>(state, std::make_index_sequence<kArgumentCount>());
  }
  else {
    return callMember<Function>(state, std::make_index_sequence<kArgumentCount>());
  }
}

/*
 * Getters and setters only run through the __index and __newindex of their own type, which
 * scripts cannot reach (the metatable is hidden), so the value at index 1 is always theirs
 * and is not checked again. A setter finds the new value at index 3, after the key.
 */
template<auto Member>
int32
getField(lua_State* state)
{
  using Info = MemberTraits<decltype(Member)>;
  const typename Info::Class& self =
      *static_cast<const typename Info::Class*>(lua_touserdata(state, 1));
  Traits<typename Info::Value>::push(state, self.*Member);
  return 1;
}

template<auto Member>
int32
setField(lua_State* state)
{
  using Info = MemberTraits<decltype(Member)>;
  typename Info::Class& self = *static_cast<typename Info::Class*>(lua_touserdata(state, 1));
  self.*Member = Traits<typename Info::Value>::check(state, 3);
  return 0;
}

template<auto Getter>
int32
getProperty(lua_State* state)
{
  return callFunction<Getter>(state);
}

template<auto Setter>
int32
setProperty(lua_State* state)
{
  lua_remove(state, 2);
  return callFunction<Setter>(state);
}

/*
 * Constructors read their arguments from index 2: index 1 is the class table Lua passes to
 * __call. No arguments gives T(), which is zero for the math types.
 */
template<typename T, typename... A, SIZE_T... I>
FORCEINLINE int32
constructWith(lua_State* state, std::index_sequence<I...>)
{
  ScriptTraits<T>::push(state, T(Traits<A>::check(state, static_cast<int32>(I) + 2)...));
  return 1;
}

template<typename T, typename... A>
int32
construct(lua_State* state)
{
  return constructWith<T, A...>(state, std::index_sequence_for<A...>());
}

template<auto Function>
int32
constructWithFunction(lua_State* state)
{
  using Arguments = typename FunctionTraits<decltype(Function)>::Arguments;
  return callFree<Function, 2>(state,
                               std::make_index_sequence<std::tuple_size_v<Arguments>>());
}

template<typename T>
int32
operatorAdd(lua_State* state)
{
  ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) + ScriptTraits<T>::check(state, 2));
  return 1;
}

template<typename T>
int32
operatorSubtract(lua_State* state)
{
  ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) - ScriptTraits<T>::check(state, 2));
  return 1;
}

template<typename T>
int32
operatorNegate(lua_State* state)
{
  ScriptTraits<T>::push(state, -ScriptTraits<T>::check(state, 1));
  return 1;
}

/*
 * Lua calls __mul for value * number, number * value and value * value, in the order they
 * were written.
 */
template<typename T>
int32
operatorMultiply(lua_State* state)
{
  if constexpr (requires(const T& value, float scalar) { value * scalar; }) {
    if (lua_type(state, 2) == LUA_TNUMBER) {
      ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) *
                                       static_cast<float>(lua_tonumber(state, 2)));
      return 1;
    }
  }
  if constexpr (requires(float scalar, const T& value) { scalar * value; }) {
    if (lua_type(state, 1) == LUA_TNUMBER) {
      ScriptTraits<T>::push(state, static_cast<float>(lua_tonumber(state, 1)) *
                                       ScriptTraits<T>::check(state, 2));
      return 1;
    }
  }
  if constexpr (requires(const T& a, const T& b) { a * b; }) {
    ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) * ScriptTraits<T>::check(state, 2));
    return 1;
  }
  else {
    return luaL_error(state, "%s cannot be multiplied by that value",
                      ScriptValueType<T>::NAME);
  }
}

template<typename T>
int32
operatorDivide(lua_State* state)
{
  if constexpr (requires(const T& value, float scalar) { value / scalar; }) {
    if (lua_type(state, 2) == LUA_TNUMBER) {
      ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) /
                                       static_cast<float>(lua_tonumber(state, 2)));
      return 1;
    }
  }
  if constexpr (requires(const T& a, const T& b) { a / b; }) {
    ScriptTraits<T>::push(state, ScriptTraits<T>::check(state, 1) / ScriptTraits<T>::check(state, 2));
    return 1;
  }
  else {
    return luaL_error(state, "%s can only be divided by a number", ScriptValueType<T>::NAME);
  }
}

template<typename T>
int32
operatorEqual(lua_State* state)
{
  const T* const other =
      static_cast<const T*>(luaL_testudata(state, 2, ScriptValueType<T>::NAME));
  lua_pushboolean(state, other && ScriptTraits<T>::check(state, 1) == *other ? 1 : 0);
  return 1;
}

template<typename T, ScriptOperator Op>
FORCEINLINE void
setOperator(lua_State* state, const int32 metatable)
{
  if constexpr (Op == ScriptOperator::Add) {
    lua_pushcfunction(state, &operatorAdd<T>);
    lua_setfield(state, metatable, "__add");
  }
  else if constexpr (Op == ScriptOperator::Subtract) {
    lua_pushcfunction(state, &operatorSubtract<T>);
    lua_setfield(state, metatable, "__sub");
  }
  else if constexpr (Op == ScriptOperator::Multiply) {
    lua_pushcfunction(state, &operatorMultiply<T>);
    lua_setfield(state, metatable, "__mul");
  }
  else if constexpr (Op == ScriptOperator::Divide) {
    lua_pushcfunction(state, &operatorDivide<T>);
    lua_setfield(state, metatable, "__div");
  }
  else if constexpr (Op == ScriptOperator::Negate) {
    lua_pushcfunction(state, &operatorNegate<T>);
    lua_setfield(state, metatable, "__unm");
  }
  else if constexpr (Op == ScriptOperator::Equal) {
    lua_pushcfunction(state, &operatorEqual<T>);
    lua_setfield(state, metatable, "__eq");
  }
  else {
    static_assert(Op != ScriptOperator::ToString,
                  "ToString has no C++ operator: give it a function with metamethod.");
  }
}

NODISCARD CH_CORE_EXPORT const ANSICHAR*
getMetamethodName(ScriptOperator op) noexcept;

/*
 * Puts a C function pointer into a userdata, so a lookup table can tell getters and setters
 * (userdata) apart from methods (functions) with one read.
 */
CH_CORE_EXPORT void
pushAccessor(lua_State* state, lua_CFunction accessor);

/*
 * __index, __newindex and __call of every value type, and __index of its class table.
 */
CH_CORE_EXPORT int32
indexValue(lua_State* state);

CH_CORE_EXPORT int32
newIndexValue(lua_State* state);

CH_CORE_EXPORT int32
callConstructor(lua_State* state);

CH_CORE_EXPORT int32
indexConstant(lua_State* state);

/*
 * Makes the tables of a value type and leaves them on the stack in the order of
 * ScriptClassSlot. Its global is set to the class table.
 */
CH_CORE_EXPORT void
beginValueClass(lua_State* state, const ANSICHAR* name);

// Descriptions for code editors are recorded only in the editor, so games build none of it.
inline constexpr bool RECORD_API = USING(CH_EDITOR);

template<typename... A>
constexpr void
ignore(const A&...) noexcept
{}

template<typename T>
consteval const ANSICHAR*
getLuaTypeName()
{
  if constexpr (ScriptValue<T> || ScriptHandle<T>) {
    return getScriptTypeName<T>();
  }
  else {
    return ScriptTraits<T>::TYPE_NAME;
  }
}

/*
 * A method made from a free function takes the value it is called on as its first argument,
 * which scripts do not write.
 */
template<auto Function>
consteval SIZE_T
getSelfArgumentCount()
{
  return std::is_void_v<typename FunctionTraits<decltype(Function)>::Class> ? 1 : 0;
}

/*
 * "name: type, ..." for the arguments from First on, named in order.
 */
template<typename Arguments, SIZE_T First, SIZE_T... I>
String
describeParameters(const ANSICHAR* const* names, std::index_sequence<I...>)
{
  String text;
  ((text += (I == 0 ? "" : ", "), text += names[I], text += ": ",
    text += getLuaTypeName<std::remove_cvref_t<Argument<Arguments, I + First>>>()),
   ...);
  return text;
}

template<typename Tuple>
struct TupleTypeNames;

template<typename... A>
struct TupleTypeNames<std::tuple<A...>>
{
  static String
  get()
  {
    String text;
    SIZE_T index = 0;
    ((text += (index++ == 0 ? "" : ", "), text += getLuaTypeName<std::remove_cvref_t<A>>()),
     ...);
    return text;
  }
};

template<typename R>
String
describeResults()
{
  if constexpr (std::is_void_v<R>) {
    return String();
  }
  else if constexpr (IsTuple<std::remove_cvref_t<R>>::value) {
    return TupleTypeNames<std::remove_cvref_t<R>>::get();
  }
  else {
    return getLuaTypeName<std::remove_cvref_t<R>>();
  }
}

template<auto Function, SIZE_T First, SIZE_T N>
ScriptApiSignature
describeFunction(const std::array<const ANSICHAR*, N>& names)
{
  using FunctionInfo = FunctionTraits<decltype(Function)>;
  return {describeParameters<typename FunctionInfo::Arguments, First>(
              names.data(), std::make_index_sequence<N>()),
          describeResults<typename FunctionInfo::Return>()};
}

template<typename T, ScriptOperator Op>
void
describeOperator(Vector<String>& outOperators)
{
  const String name = getScriptTypeName<T>();
  if constexpr (Op == ScriptOperator::Add) {
    outOperators.push_back("add(" + name + "): " + name);
  }
  else if constexpr (Op == ScriptOperator::Subtract) {
    outOperators.push_back("sub(" + name + "): " + name);
  }
  else if constexpr (Op == ScriptOperator::Negate) {
    outOperators.push_back("unm: " + name);
  }
  else if constexpr (Op == ScriptOperator::Multiply) {
    if constexpr (requires(const T& value, float scalar) { value * scalar; }) {
      outOperators.push_back("mul(number): " + name);
    }
    if constexpr (requires(const T& a, const T& b) { a * b; }) {
      outOperators.push_back("mul(" + name + "): " + name);
    }
  }
  else if constexpr (Op == ScriptOperator::Divide) {
    if constexpr (requires(const T& value, float scalar) { value / scalar; }) {
      outOperators.push_back("div(number): " + name);
    }
    if constexpr (requires(const T& a, const T& b) { a / b; }) {
      outOperators.push_back("div(" + name + "): " + name);
    }
  }
}
} // namespace ScriptDetail

/**
 * Exists so exposing a type to scripts is one line per member, with the name scripts see
 * chosen here (it may differ from the C++ one) and the names of its parameters, which code
 * editors show:
 *
 *   ScriptClass<Vector3>(state)
 *     .constructor<>()
 *     .constructor<float, float, float>("x", "y", "z")
 *     .field<&Vector3::x>("x")
 *     .method<&Vector3::dot>("dot", "other")
 *     .method<&scaleInPlace>("scale", "scalar")   // Vector3& scaleInPlace(Vector3&, float)
 *     .rawMethod("get", &get, "fun(out?: Vector3): Vector3")
 *     .operators<ScriptOperator::Add, ScriptOperator::Multiply>()
 *     .constant("UP", Vector3::UP);
 *
 * The wrappers are made at compile time from the function types, so a call costs the reading
 * of its arguments and nothing else; the types code editors see come from the same function
 * types, and the build stops when the parameter names do not match their count. A method may
 * be a member function or a free function whose first parameter is the value; a function
 * that returns a reference to that value gives back the same Lua value, so in-place methods
 * chain without copies. Overloads and default arguments are invisible to templates: wrap them
 * in a free function, or write a raw one with each way to call it. The tables stay on the Lua
 * stack while the object lives, so make it, chain the calls and drop it.
 */
template<typename T>
  requires(ScriptValue<T> || ScriptHandle<T>)
class ScriptClass
{
 public:
  explicit ScriptClass(lua_State* state)
    : m_state(state),
      m_base(lua_gettop(state) + 1)
  {
    ScriptDetail::beginValueClass(state, getScriptTypeName<T>());
    if constexpr (ScriptDetail::RECORD_API) {
      if (ScriptApiDescription* description = ScriptApiDescription::find(state)) {
        m_api = &description->addType(getScriptTypeName<T>(), ScriptApiTypeKind::Class);
      }
    }
  }

  ~ScriptClass() { lua_settop(m_state, m_base - 1); }

  ScriptClass(const ScriptClass&) = delete;

  ScriptClass&
  operator=(const ScriptClass&) = delete;

  /**
   * Scripts call Name(args...); constructors are told apart by how many arguments they take.
   */
  template<typename... A, typename... Names>
  ScriptClass&
  constructor(const Names... parameterNames)
  {
    static_assert(ScriptValue<T>, "Only value types are made by scripts.");
    static_assert(sizeof...(Names) == sizeof...(A), "Name every parameter of the constructor.");
    lua_pushcfunction(m_state, (&ScriptDetail::construct<T, A...>));
    lua_rawseti(m_state, slot(Slot::Constructors), static_cast<lua_Integer>(sizeof...(A)));
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        const std::array<const ANSICHAR*, sizeof...(Names)> names{parameterNames...};
        m_api->constructors.push_back(
            {ScriptDetail::describeParameters<std::tuple<A...>, 0>(
                 names.data(), std::index_sequence_for<A...>()),
             getScriptTypeName<T>()});
      }
    }
    else {
      ScriptDetail::ignore(parameterNames...);
    }
    return *this;
  }

  /**
   * A constructor made by a free function that returns T, for when T's own constructors do
   * not fit (Quaternion() must be the identity, not zero).
   */
  template<auto Function, typename... Names>
  ScriptClass&
  constructorFunction(const Names... parameterNames)
  {
    using Arguments = typename ScriptDetail::FunctionTraits<decltype(Function)>::Arguments;
    static_assert(sizeof...(Names) == std::tuple_size_v<Arguments>,
                  "Name every parameter of the constructor.");
    lua_pushcfunction(m_state, &ScriptDetail::constructWithFunction<Function>);
    lua_rawseti(m_state, slot(Slot::Constructors),
                static_cast<lua_Integer>(std::tuple_size_v<Arguments>));
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        const std::array<const ANSICHAR*, sizeof...(Names)> names{parameterNames...};
        m_api->constructors.push_back(ScriptDetail::describeFunction<Function, 0>(names));
      }
    }
    else {
      ScriptDetail::ignore(parameterNames...);
    }
    return *this;
  }

  /**
   * Called as value:name(args...). Name the parameters after the value it is called on.
   */
  template<auto Function, typename... Names>
  ScriptClass&
  method(const ANSICHAR* name, const Names... parameterNames)
  {
    using Arguments = typename ScriptDetail::FunctionTraits<decltype(Function)>::Arguments;
    constexpr SIZE_T kSelf = ScriptDetail::getSelfArgumentCount<Function>();
    static_assert(sizeof...(Names) + kSelf == std::tuple_size_v<Arguments>,
                  "Name every parameter of the method, after the value it is called on.");
    lua_pushcfunction(m_state, &ScriptDetail::callFunction<Function>);
    lua_setfield(m_state, slot(Slot::Members), name);
    recordFunction<Function, kSelf>(name, true, parameterNames...);
    return *this;
  }

  /**
   * A data member scripts read and write as value.name.
   */
  template<auto Member>
  ScriptClass&
  field(const ANSICHAR* name)
  {
    static_assert(ScriptValue<T>, "Handle types have methods only: their object may be gone.");
    ScriptDetail::pushAccessor(m_state, &ScriptDetail::getField<Member>);
    lua_setfield(m_state, slot(Slot::Members), name);
    ScriptDetail::pushAccessor(m_state, &ScriptDetail::setField<Member>);
    lua_setfield(m_state, slot(Slot::Setters), name);
    recordField<typename ScriptDetail::MemberTraits<decltype(Member)>::Value>(name);
    return *this;
  }

  /**
   * Read like a field through a getter that takes only the value. With a setter (value,
   * newValue) it can be written too.
   */
  template<auto Getter, auto Setter = nullptr>
  ScriptClass&
  property(const ANSICHAR* name)
  {
    ScriptDetail::pushAccessor(m_state, &ScriptDetail::getProperty<Getter>);
    lua_setfield(m_state, slot(Slot::Members), name);
    if constexpr (Setter != nullptr) {
      ScriptDetail::pushAccessor(m_state, &ScriptDetail::setProperty<Setter>);
      lua_setfield(m_state, slot(Slot::Setters), name);
    }
    recordField<typename ScriptDetail::FunctionTraits<decltype(Getter)>::Return>(name);
    return *this;
  }

  /**
   * A Lua C function as it is, for what the generated wrappers cannot express (optional or
   * varying arguments). It reads the value at index 1. Each signature is one way to call it,
   * written for code editors as "fun(name: type, name?: type): result", without the value.
   */
  template<typename... Signatures>
  ScriptClass&
  rawMethod(const ANSICHAR* name, const lua_CFunction function,
            const Signatures... signatures)
  {
    static_assert(sizeof...(Signatures) > 0,
                  "Write how the method is called: \"fun(x: number): number\".");
    lua_pushcfunction(m_state, function);
    lua_setfield(m_state, slot(Slot::Members), name);
    recordRawFunction(name, true, signatures...);
    return *this;
  }

  template<typename... Signatures>
  ScriptClass&
  rawStaticFunction(const ANSICHAR* name, const lua_CFunction function,
                    const Signatures... signatures)
  {
    static_assert(sizeof...(Signatures) > 0,
                  "Write how the function is called: \"fun(x: number): number\".");
    lua_pushcfunction(m_state, function);
    lua_setfield(m_state, slot(Slot::Class), name);
    recordRawFunction(name, false, signatures...);
    return *this;
  }

  /**
   * Called on the class: Name.function(args...).
   */
  template<auto Function, typename... Names>
  ScriptClass&
  staticFunction(const ANSICHAR* name, const Names... parameterNames)
  {
    using Arguments = typename ScriptDetail::FunctionTraits<decltype(Function)>::Arguments;
    static_assert(sizeof...(Names) == std::tuple_size_v<Arguments>,
                  "Name every parameter of the function.");
    lua_pushcfunction(m_state, &ScriptDetail::callFunction<Function>);
    lua_setfield(m_state, slot(Slot::Class), name);
    recordFunction<Function, 0>(name, false, parameterNames...);
    return *this;
  }

  /**
   * Name.CONSTANT gives a new copy on each read, so a script that changes it in place does
   * not change the constant for everybody. Keep it in a local when it is read every frame.
   */
  ScriptClass&
  constant(const ANSICHAR* name, const T& value)
  {
    static_assert(ScriptValue<T>, "Only value types have value constants.");
    ScriptTraits<T>::push(m_state, value);
    lua_setfield(m_state, slot(Slot::Constants), name);
    recordConstant(name, getScriptTypeName<T>());
    return *this;
  }

  ScriptClass&
  constant(const ANSICHAR* name, const float value)
  {
    lua_pushnumber(m_state, static_cast<lua_Number>(value));
    lua_setfield(m_state, slot(Slot::Class), name);
    recordConstant(name, "number");
    return *this;
  }

  /**
   * Arithmetic and == through the C++ operators of T. Multiply and Divide accept a number on
   * either side when T has those operators.
   */
  template<ScriptOperator... Ops>
  ScriptClass&
  operators()
  {
    (ScriptDetail::setOperator<T, Ops>(m_state, slot(Slot::Metatable)), ...);
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        (ScriptDetail::describeOperator<T, Ops>(m_api->operators), ...);
      }
    }
    return *this;
  }

  /**
   * A metamethod made from any function, for what has no C++ operator (ToString).
   */
  template<ScriptOperator Op, auto Function>
  ScriptClass&
  metamethod()
  {
    lua_pushcfunction(m_state, &ScriptDetail::callFunction<Function>);
    lua_setfield(m_state, slot(Slot::Metatable), ScriptDetail::getMetamethodName(Op));
    return *this;
  }

 private:
  // Must match the order beginValueClass pushes the tables in.
  enum class Slot : int32 { Metatable, Members, Setters, Class, Constructors, Constants };

  NODISCARD FORCEINLINE int32
  slot(const Slot which) const noexcept
  {
    return m_base + static_cast<int32>(which);
  }

  template<auto Function, SIZE_T First, typename... Names>
  void
  recordFunction(const ANSICHAR* name, const bool bMethod, const Names... parameterNames)
  {
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        const std::array<const ANSICHAR*, sizeof...(Names)> names{parameterNames...};
        ScriptApiFunction& function = m_api->functions.emplace_back();
        function.name = name;
        function.bMethod = bMethod;
        function.signatures.push_back(ScriptDetail::describeFunction<Function, First>(names));
      }
    }
    else {
      ScriptDetail::ignore(name, bMethod, parameterNames...);
    }
  }

  template<typename... Signatures>
  void
  recordRawFunction(const ANSICHAR* name, const bool bMethod, const Signatures... signatures)
  {
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        const ANSICHAR* const texts[] = {signatures...};
        ScriptApiFunction& function = m_api->functions.emplace_back();
        function.name = name;
        function.bMethod = bMethod;
        ScriptApiDescription::addSignatures(function, texts);
      }
    }
    else {
      ScriptDetail::ignore(name, bMethod, signatures...);
    }
  }

  template<typename Value>
  void
  recordField(const ANSICHAR* name)
  {
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        m_api->fields.push_back(
            {name, ScriptDetail::getLuaTypeName<std::remove_cvref_t<Value>>()});
      }
    }
    else {
      ScriptDetail::ignore(name);
    }
  }

  void
  recordConstant(const ANSICHAR* name, const ANSICHAR* type)
  {
    if constexpr (ScriptDetail::RECORD_API) {
      if (m_api) {
        m_api->constants.push_back({name, type});
      }
    }
    else {
      ScriptDetail::ignore(name, type);
    }
  }

  lua_State* m_state;
  int32 m_base;
  // Where registrations are described; null when nothing records them.
  ScriptApiType* m_api = nullptr;
};

/**
 * Exists so a global table of functions (Input, Scene) is registered and described the same
 * way a type is:
 *
 *   ScriptTable(state, "Scene")
 *     .rawFunction("find", &findInScene, "fun(name: string): GameObject?");
 *
 * The global is set when the object is destroyed, so make it, chain the calls and drop it.
 */
class CH_CORE_EXPORT ScriptTable
{
 public:
  /**
   * description is what code editors show for the table.
   */
  ScriptTable(lua_State* state, const ANSICHAR* name, const ANSICHAR* description = "");

  ~ScriptTable();

  ScriptTable(const ScriptTable&) = delete;

  ScriptTable&
  operator=(const ScriptTable&) = delete;

  /**
   * Called as Name.name(args...); each signature is one way to call it, written as in
   * ScriptClass::rawMethod.
   */
  template<typename... Signatures>
  ScriptTable&
  rawFunction(const ANSICHAR* name, const lua_CFunction function,
              const Signatures... signatures)
  {
    static_assert(sizeof...(Signatures) > 0,
                  "Write how the function is called: \"fun(x: number): number\".");
    const ANSICHAR* const texts[] = {signatures...};
    setFunction(name, function, texts);
    return *this;
  }

  /**
   * Tells code editors about a value the engine keeps in the table (Time.deltaTime).
   */
  ScriptTable&
  describeField(const ANSICHAR* name, const ANSICHAR* type);

  /**
   * Where the table is on the Lua stack while this lives.
   */
  NODISCARD FORCEINLINE int32
  getIndex() const noexcept { return m_index; }

 private:
  void
  setFunction(const ANSICHAR* name, lua_CFunction function,
              Span<const ANSICHAR* const> signatures);

  lua_State* m_state;
  const ANSICHAR* m_name;
  int32 m_index;
  ScriptApiType* m_api = nullptr;
};

} // namespace chEngineSDK
