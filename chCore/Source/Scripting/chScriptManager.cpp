/************************************************************************/
/**
 * @file chScriptManager.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The Lua runtime that runs the game scripts.
 */
/************************************************************************/
#include "chScriptManager.h"

#include "chAssetManager.h"
#include "chConsoleVariable.h"
#include "chInputManager.h"
#include "chScriptAllocator.h"
#include "chScriptApi.h"
#include "chScriptAsset.h"
#include "chScriptBinder.h"
#include "chScriptBindings.h"
#include "chScriptMathTypes.h"
#include "chScriptObjectTypes.h"
#include "chGameObject.h"
#include "chScene.h"

#include "lualib.h"

namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(ScriptLog, All);

namespace {
ConsoleVariable<bool> g_cvarPoolAllocator("Script.PoolAllocator",
                                          true,
                                          "Small script objects (temporary vectors, strings, "
                                          "tables) come from pools instead of malloc. Read "
                                          "when the script runtime starts.");

ConsoleVariable<bool> g_cvarGenerationalGC("Script.GenerationalGC",
                                           true,
                                           "Generational garbage collection: objects that die "
                                           "young are freed in quick passes. Off uses Lua's "
                                           "incremental collector.");

/*
 * Message handler of every protected call: adds the traceback while the failing function is
 * still on the stack.
 */
int32
traceback(lua_State* state)
{
  const ANSICHAR* message = lua_tostring(state, 1);
  if (!message) {
    if (luaL_callmeta(state, 1, "__tostring") && lua_type(state, -1) == LUA_TSTRING) {
      return 1;
    }
    message = lua_pushfstring(state, "(error object is a %s value)", luaL_typename(state, 1));
  }
  luaL_traceback(state, state, message, 1);
  return 1;
}

/*
 * The error on top of the stack as text (the message handler always leaves a string, but a
 * load error comes without it).
 */
const ANSICHAR*
getErrorText(lua_State* state)
{
  const ANSICHAR* const text = lua_tostring(state, -1);
  return text ? text : "(error without a message)";
}

/*
 * A Lua error outside any protected call would end the process, so at least it is logged.
 */
int32
panic(lua_State* state)
{
  CH_LOG_FATAL(ScriptLog, "Unprotected script error: {0}", getErrorText(state));
  return 0;
}

/*
 * Writes the arguments as one line, after the script and line that called.
 */
int32
writeLog(lua_State* state, const LogVerbosity verbosity, const ANSICHAR separator)
{
  const int32 count = lua_gettop(state);
  luaL_Buffer buffer;
  luaL_buffinit(state, &buffer);
  luaL_where(state, 1);
  luaL_addvalue(&buffer);
  for (int32 i = 1; i <= count; ++i) {
    if (i > 1) {
      luaL_addchar(&buffer, separator);
    }
    luaL_tolstring(state, i, nullptr);
    luaL_addvalue(&buffer);
  }
  luaL_pushresult(&buffer);

  SIZE_T size = 0;
  const ANSICHAR* const text = lua_tolstring(state, -1, &size);
  const StringView message(text, size);
  switch (verbosity) {
  case LogVerbosity::Error:
    CH_LOG(ScriptLog, Error, message);
    break;
  case LogVerbosity::Warning:
    CH_LOG(ScriptLog, Warning, message);
    break;
  case LogVerbosity::Debug:
    CH_LOG(ScriptLog, Debug, message);
    break;
  default:
    CH_LOG(ScriptLog, Info, message);
    break;
  }
  return 0;
}

int32
print(lua_State* state)
{
  return writeLog(state, LogVerbosity::Info, '\t');
}

int32
logInfo(lua_State* state)
{
  return writeLog(state, LogVerbosity::Info, ' ');
}

int32
logWarning(lua_State* state)
{
  return writeLog(state, LogVerbosity::Warning, ' ');
}

int32
logError(lua_State* state)
{
  return writeLog(state, LogVerbosity::Error, ' ');
}

int32
logDebug(lua_State* state)
{
  return writeLog(state, LogVerbosity::Debug, ' ');
}

/*
 * The value at index as type, without raising. False when it holds another type.
 */
bool
toPropertyValue(lua_State* state,
                const int32 index,
                const ScriptPropertyType type,
                ScriptPropertyValue& outValue)
{
  outValue = ScriptPropertyValue::makeDefault(type);
  const auto copyComponents = [&outValue](const float* components, const uint32 count) {
    for (uint32 i = 0; i < count; ++i) {
      outValue.components[i] = components[i];
    }
  };
  switch (type) {
    case ScriptPropertyType::Number:
      if (lua_type(state, index) != LUA_TNUMBER) {
        return false;
      }
      outValue.number = static_cast<double>(lua_tonumber(state, index));
      return true;
    case ScriptPropertyType::Integer: {
      if (lua_type(state, index) != LUA_TNUMBER) {
        return false;
      }
      if (lua_isinteger(state, index)) {
        outValue.integer = static_cast<int64>(lua_tointeger(state, index));
        return true;
      }
      // 3.0 is fine for an integer; 3.5 is not.
      const double number = static_cast<double>(lua_tonumber(state, index));
      outValue.integer = static_cast<int64>(number);
      return static_cast<double>(outValue.integer) == number;
    }
    case ScriptPropertyType::Boolean:
      if (lua_type(state, index) != LUA_TBOOLEAN) {
        return false;
      }
      outValue.boolean = lua_toboolean(state, index) != 0;
      return true;
    case ScriptPropertyType::String: {
      if (lua_type(state, index) != LUA_TSTRING) {
        return false;
      }
      SIZE_T length = 0;
      const ANSICHAR* text = lua_tolstring(state, index, &length);
      outValue.text.assign(text, length);
      return true;
    }
    case ScriptPropertyType::Vector2: {
      const auto* vector = static_cast<const Vector2*>(
          luaL_testudata(state, index, ScriptValueType<Vector2>::NAME));
      if (vector) {
        const float components[2] = {vector->x, vector->y};
        copyComponents(components, 2);
      }
      return vector != nullptr;
    }
    case ScriptPropertyType::Vector3: {
      const auto* vector = static_cast<const Vector3*>(
          luaL_testudata(state, index, ScriptValueType<Vector3>::NAME));
      if (vector) {
        const float components[3] = {vector->x, vector->y, vector->z};
        copyComponents(components, 3);
      }
      return vector != nullptr;
    }
    case ScriptPropertyType::Vector4: {
      const auto* vector = static_cast<const Vector4*>(
          luaL_testudata(state, index, ScriptValueType<Vector4>::NAME));
      if (vector) {
        const float components[4] = {vector->x, vector->y, vector->z, vector->w};
        copyComponents(components, 4);
      }
      return vector != nullptr;
    }
    case ScriptPropertyType::Color: {
      const auto* color = static_cast<const LinearColor*>(
          luaL_testudata(state, index, ScriptValueType<LinearColor>::NAME));
      if (color) {
        const float components[4] = {color->r, color->g, color->b, color->a};
        copyComponents(components, 4);
      }
      return color != nullptr;
    }
    case ScriptPropertyType::GameObject: {
      if (lua_isnil(state, index)) {
        return true;
      }
      const GameObject* gameObject = ScriptTraits<GameObject>::test(state, index);
      if (gameObject) {
        outValue.object = gameObject->getUUID();
      }
      return gameObject != nullptr;
    }
    default:
      return false;
  }
}

/*
 * The type a default given without type = "..." stands for. False for nil and for values
 * no property can hold.
 */
bool
inferPropertyType(lua_State* state, const int32 index, ScriptPropertyType& outType)
{
  switch (lua_type(state, index)) {
    case LUA_TNUMBER:
      // 5 is a number like 5.0, so the editor never forbids 2.5; integers ask for it.
      outType = ScriptPropertyType::Number;
      return true;
    case LUA_TBOOLEAN:
      outType = ScriptPropertyType::Boolean;
      return true;
    case LUA_TSTRING:
      outType = ScriptPropertyType::String;
      return true;
    case LUA_TUSERDATA: {
      constexpr ScriptPropertyType kValueTypes[] = {
          ScriptPropertyType::Vector2, ScriptPropertyType::Vector3,
          ScriptPropertyType::Vector4, ScriptPropertyType::Color};
      ScriptPropertyValue scratch;
      for (const ScriptPropertyType type : kValueTypes) {
        if (toPropertyValue(state, index, type, scratch)) {
          outType = type;
          return true;
        }
      }
      return false;
    }
    default:
      return false;
  }
}

void
pushPropertyValue(lua_State* state, const ScriptPropertyValue& value, const Scene* scene)
{
  const float* components = value.components;
  switch (value.type) {
    case ScriptPropertyType::Number:
      lua_pushnumber(state, static_cast<lua_Number>(value.number));
      break;
    case ScriptPropertyType::Integer:
      lua_pushinteger(state, static_cast<lua_Integer>(value.integer));
      break;
    case ScriptPropertyType::Boolean:
      lua_pushboolean(state, value.boolean ? 1 : 0);
      break;
    case ScriptPropertyType::String:
      lua_pushlstring(state, value.text.data(), value.text.size());
      break;
    case ScriptPropertyType::Vector2:
      ScriptTraits<Vector2>::push(state, Vector2(components[0], components[1]));
      break;
    case ScriptPropertyType::Vector3:
      ScriptTraits<Vector3>::push(state,
                                  Vector3(components[0], components[1], components[2]));
      break;
    case ScriptPropertyType::Vector4:
      ScriptTraits<Vector4>::push(
          state, Vector4(components[0], components[1], components[2], components[3]));
      break;
    case ScriptPropertyType::Color:
      ScriptTraits<LinearColor>::push(
          state, LinearColor(components[0], components[1], components[2], components[3]));
      break;
    case ScriptPropertyType::GameObject: {
      const SPtr<GameObject> gameObject =
          scene && !value.object.isNull() ? scene->findGameObject(value.object) : nullptr;
      if (gameObject) {
        ScriptTraits<GameObject>::push(state, *gameObject);
      }
      else {
        lua_pushnil(state);
      }
      break;
    }
    default:
      lua_pushnil(state);
      break;
  }
}

/*
 * The override with the property's name and type, else its default.
 */
const ScriptPropertyValue&
getPropertyValue(const ScriptProperty& property, Span<const ScriptPropertyOverride> overrides)
{
  for (const ScriptPropertyOverride& entry : overrides) {
    if (entry.name == property.name && entry.value.type == property.defaultValue.type) {
      return entry.value;
    }
  }
  return property.defaultValue;
}

/*
 * One entry of the properties list, at the top of the stack:
 * { "name", default, type = "...", min = n, max = n, tooltip = "..." }.
 */
void
readProperty(lua_State* state, const lua_Integer position, ScriptClassInfo& info)
{
  const int32 entry = lua_gettop(state);
  if (!lua_istable(state, entry)) {
    luaL_error(state, "property %d must be a table such as { \"speed\", 5 }, not a %s",
               static_cast<int32>(position), luaL_typename(state, entry));
  }

  // A misspelled field (mni = 0) would otherwise be ignored without a word.
  lua_pushnil(state);
  while (lua_next(state, entry) != 0) {
    lua_pop(state, 1);
    bool bKnown = false;
    if (lua_type(state, -1) == LUA_TNUMBER) {
      const lua_Integer key = lua_isinteger(state, -1) ? lua_tointeger(state, -1) : 0;
      bKnown = key == 1 || key == 2;
    }
    else if (lua_type(state, -1) == LUA_TSTRING) {
      const StringView key(lua_tostring(state, -1));
      bKnown = key == "type" || key == "min" || key == "max" || key == "tooltip";
    }
    if (!bKnown) {
      lua_pushvalue(state, -1);
      const ANSICHAR* keyText = luaL_tolstring(state, -1, nullptr);
      luaL_error(state,
                 "property %d has an unknown field '%s' (allowed: name, default, type, min, "
                 "max, tooltip)",
                 static_cast<int32>(position), keyText);
    }
  }

  if (lua_rawgeti(state, entry, 1) != LUA_TSTRING) {
    luaL_error(state, "property %d must start with its name, a string",
               static_cast<int32>(position));
  }
  SIZE_T nameLength = 0;
  const ANSICHAR* nameText = lua_tolstring(state, -1, &nameLength);
  ScriptProperty property;
  property.name.assign(nameText, nameLength);
  lua_pop(state, 1);
  const ANSICHAR* name = property.name.c_str();
  if (property.name.empty() || nameLength > ScriptPropertyUtils::MAX_NAME_LENGTH) {
    luaL_error(state, "property %d needs a name of 1 to %d characters",
               static_cast<int32>(position),
               static_cast<int32>(ScriptPropertyUtils::MAX_NAME_LENGTH));
  }
  if (ScriptPropertyUtils::isReservedName(property.name)) {
    luaL_error(state, "property '%s' uses a name the engine reserves", name);
  }
  for (const ScriptProperty& other : info.properties) {
    if (other.name == property.name) {
      luaL_error(state, "property '%s' is listed twice", name);
    }
  }

  const int32 defaultIndex = entry + 1;
  lua_rawgeti(state, entry, 2);
  ScriptPropertyType type = ScriptPropertyType::Number;
  if (lua_getfield(state, entry, "type") != LUA_TNIL) {
    if (lua_type(state, -1) != LUA_TSTRING ||
        !ScriptPropertyUtils::fromTypeName(lua_tostring(state, -1), type)) {
      luaL_error(state,
                 "property '%s' has an unknown type '%s' (number, integer, boolean, string, "
                 "Vector2, Vector3, Vector4, Color, GameObject)",
                 name, luaL_tolstring(state, -1, nullptr));
    }
  }
  else if (!inferPropertyType(state, defaultIndex, type)) {
    if (lua_isnil(state, defaultIndex)) {
      luaL_error(state, "property '%s' needs a default value or a type", name);
    }
    luaL_error(state, "property '%s' cannot hold a %s", name,
               luaL_typename(state, defaultIndex));
  }
  lua_pop(state, 1);

  if (type == ScriptPropertyType::GameObject && !lua_isnil(state, defaultIndex)) {
    luaL_error(state, "property '%s' is a GameObject: it is picked in the editor and has no "
                      "default",
               name);
  }
  if (lua_isnil(state, defaultIndex)) {
    property.defaultValue = ScriptPropertyValue::makeDefault(type);
  }
  else if (!toPropertyValue(state, defaultIndex, type, property.defaultValue)) {
    luaL_error(state, "property '%s' is a %s but its default is a %s", name,
               ScriptPropertyUtils::getTypeName(type), luaL_typename(state, defaultIndex));
  }
  lua_pop(state, 1);

  const auto readNumber = [state, entry, name](const ANSICHAR* field, double& outNumber) {
    const int32 fieldType = lua_getfield(state, entry, field);
    if (fieldType == LUA_TNUMBER) {
      outNumber = static_cast<double>(lua_tonumber(state, -1));
    }
    else if (fieldType != LUA_TNIL) {
      luaL_error(state, "'%s' of property '%s' must be a number", field, name);
    }
    lua_pop(state, 1);
  };
  double minimum = 0.0;
  double maximum = 0.0;
  readNumber("min", minimum);
  readNumber("max", maximum);
  const int32 tooltipType = lua_getfield(state, entry, "tooltip");
  if (tooltipType != LUA_TNIL && tooltipType != LUA_TSTRING) {
    luaL_error(state, "'tooltip' of property '%s' must be a string", name);
  }
#if USING(CH_EDITOR)
  property.minimum = minimum;
  property.maximum = maximum;
  if (tooltipType == LUA_TSTRING) {
    property.tooltip = lua_tostring(state, -1);
  }
#endif
  lua_pop(state, 1);
  info.properties.push_back(std::move(property));
}

/*
 * Reads the optional properties list of the class at index 1.
 */
void
readProperties(lua_State* state, ScriptClassInfo& info)
{
  const int32 type = lua_getfield(state, 1, "properties");
  if (type == LUA_TNIL) {
    lua_pop(state, 1);
    return;
  }
  if (type != LUA_TTABLE) {
    luaL_error(state, "'properties' of a script class must be a list, not a %s",
               lua_typename(state, type));
  }
  const int32 list = lua_gettop(state);
  const lua_Integer count = static_cast<lua_Integer>(lua_rawlen(state, list));
  if (count > static_cast<lua_Integer>(ScriptPropertyUtils::MAX_PROPERTIES)) {
    luaL_error(state, "a script can list at most %d properties",
               static_cast<int32>(ScriptPropertyUtils::MAX_PROPERTIES));
  }
  info.properties.reserve(static_cast<SIZE_T>(count));
  for (lua_Integer i = 1; i <= count; ++i) {
    lua_rawgeti(state, list, i);
    readProperty(state, i, info);
    lua_pop(state, 1);
  }
  lua_pop(state, 1);
}

/*
 * Runs protected, since reading the class may run its metamethods. Stack: class table,
 * ScriptClassInfo as light userdata.
 */
int32
readClass(lua_State* state)
{
  ScriptClassInfo& info = *static_cast<ScriptClassInfo*>(lua_touserdata(state, 2));
  lua_settop(state, 1);

  lua_pushvalue(state, 1);
  info.classTable = luaL_ref(state, LUA_REGISTRYINDEX);

  lua_createtable(state, 0, 1);
  lua_pushvalue(state, 1);
  lua_setfield(state, -2, "__index");
  info.instanceMetatable = luaL_ref(state, LUA_REGISTRYINDEX);

  const auto takeFunction = [state](const ANSICHAR* name, int32& outReference) {
    const int32 type = lua_getfield(state, 1, name);
    if (type == LUA_TFUNCTION) {
      outReference = luaL_ref(state, LUA_REGISTRYINDEX);
      return;
    }
    if (type != LUA_TNIL) {
      luaL_error(state, "'%s' of a script class must be a function, not a %s", name,
                 lua_typename(state, type));
    }
    lua_pop(state, 1);
  };
  takeFunction("onStart", info.onStart);
  takeFunction("update", info.update);
  takeFunction("fixedUpdate", info.fixedUpdate);
  takeFunction("onDestroy", info.onDestroy);
  readProperties(state, info);
  return 0;
}

/*
 * Shared by the copies of the capture callback, so the Lua function is released once, when
 * the InputManager drops the callback: after calling it, on a new capture, on a cancel, or
 * when the game state is reset at Stop.
 */
struct ScriptKeyCapture
{
  ScriptKeyCapture(lua_State* inState, const int32 inCallback) noexcept
    : state(inState),
      callback(inCallback)
  {}

  ~ScriptKeyCapture()
  {
    luaL_unref(state, LUA_REGISTRYINDEX, callback);
  }

  ScriptKeyCapture(const ScriptKeyCapture&) = delete;

  ScriptKeyCapture&
  operator=(const ScriptKeyCapture&) = delete;

  lua_State* state;
  int32 callback;
};
} // namespace

/*
 */
ScriptManager::ScriptManager()
  : ScriptManager(g_cvarPoolAllocator.get())
{}

/*
 */
ScriptManager::ScriptManager(const bool bPoolAllocator)
  : m_allocator(chMakeUnique<ScriptAllocator>(bPoolAllocator)),
    m_input(InputManager::isStarted() ? &InputManager::instance() : nullptr)
{
  m_state = lua_newstate(&ScriptAllocator::allocate, m_allocator.get());
  CH_ASSERT(m_state && "Not enough memory to start the script runtime.");
  lua_atpanic(m_state, &panic);
#if USING(CH_EDITOR)
  m_apiDescription = chMakeUnique<ScriptApiDescription>();
#endif // USING(CH_EDITOR)
  // Always written: Lua leaves the state's extra space uninitialized.
  ScriptApiDescription::attach(m_state, m_apiDescription.get());
  if (g_cvarGenerationalGC.get()) {
    lua_gc(m_state, LUA_GCGEN, 0, 0);
  }
  else {
    lua_gc(m_state, LUA_GCINC, 0, 0, 0);
  }
  openLibraries();
}

/*
 */
ScriptManager::~ScriptManager()
{
  // The capture's callback lives in this state, so it must go before the state does.
  if (m_input && !m_keyCapture.expired()) {
    m_input->cancelKeyCapture();
  }
  lua_close(m_state);
}

/*
 */
void
ScriptManager::openLibraries()
{
  // No io, os, package or debug: scripts never touch files, the OS or the VM's internals.
  const luaL_Reg libraries[] = {{LUA_GNAME, &luaopen_base},
                                {LUA_COLIBNAME, &luaopen_coroutine},
                                {LUA_TABLIBNAME, &luaopen_table},
                                {LUA_STRLIBNAME, &luaopen_string},
                                {LUA_MATHLIBNAME, &luaopen_math},
                                {LUA_UTF8LIBNAME, &luaopen_utf8}};
  for (const luaL_Reg& library : libraries) {
    luaL_requiref(m_state, library.name, library.func, 1);
    lua_pop(m_state, 1);
  }
  // load also takes precompiled chunks, which can break the VM; the others read files.
  for (const ANSICHAR* name : {"dofile", "loadfile", "load"}) {
    lua_pushnil(m_state);
    lua_setglobal(m_state, name);
  }
  lua_pushcfunction(m_state, &print);
  lua_setglobal(m_state, "print");

  const ANSICHAR* const kLogSignature = "fun(...: any)";
  ScriptTable(m_state, "Log", "Writes to the engine log, after the script and line that called.")
      .rawFunction("info", &logInfo, kLogSignature)
      .rawFunction("warning", &logWarning, kLogSignature)
      .rawFunction("error", &logError, kLogSignature)
      .rawFunction("debug", &logDebug, kLogSignature);

  {
    ScriptTable time(m_state, "Time", "Written by the engine before scripts run.");
    time.describeField("deltaTime", "number")
        .describeField("fixedDeltaTime", "number")
        .describeField("time", "number")
        .describeField("frameCount", "integer");
    lua_pushvalue(m_state, time.getIndex());
    m_timeTable = luaL_ref(m_state, LUA_REGISTRYINDEX);
  }
  writeTime();

  if (m_apiDescription) {
    ScriptApiType& script = m_apiDescription->addType("Script", ScriptApiTypeKind::Shape);
    script.description = "The self of every script. Write `---@class MyScript : Script` "
                         "above a script's table to have its fields completed.";
    script.fields = {{"gameObject", "GameObject"},
                     {"transform", "Transform"},
                     {"onStart?", "fun(self: Script)"},
                     {"update?", "fun(self: Script, deltaTime: number)"},
                     {"fixedUpdate?", "fun(self: Script, deltaTime: number)"},
                     {"onDestroy?", "fun(self: Script)"},
                     {"properties?", "ScriptPropertyEntry[]"}};

    m_apiDescription->addAlias("ScriptPropertyType",
                               "\"number\"|\"integer\"|\"boolean\"|\"string\"|\"Vector2\"|"
                               "\"Vector3\"|\"Vector4\"|\"Color\"|\"GameObject\"");
    ScriptApiType& entry =
        m_apiDescription->addType("ScriptPropertyEntry", ScriptApiTypeKind::Shape);
    entry.description = "One value the editor shows for each object using the script: "
                        "`{ \"speed\", 5, min = 0, max = 10, tooltip = \"...\" }`. "
                        "Instances find it in self before onStart.";
    entry.fields = {{"[1]", "string"},
                    {"[2]", "any"},
                    {"type?", "ScriptPropertyType"},
                    {"min?", "number"},
                    {"max?", "number"},
                    {"tooltip?", "string"}};
  }

  ScriptBindings::registerMath(m_state);
  ScriptBindings::registerObjects(m_state);
  ScriptBindings::registerInput(m_state);
  ScriptBindings::registerDebugDraw(m_state);
}

/*
 */
bool
ScriptManager::runString(const StringView code, const ANSICHAR* chunkName)
{
  lua_pushcfunction(m_state, &traceback);
  if (luaL_loadbufferx(m_state, code.data(), code.size(), chunkName, "t") != LUA_OK) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
    lua_pop(m_state, 2);
    return false;
  }
  return protectedCall(0);
}

/*
 */
const ScriptClassInfo*
ScriptManager::getClass(const ScriptAsset& script)
{
  const auto it = m_classes.find(script.getUUID());
  if (it != m_classes.end()) {
    return &it->second;
  }
  if (!script.isLoaded()) {
    return nullptr;
  }

  // Kept even when it fails, so a broken script is reported once and not compiled again
  // every frame.
  ScriptClassInfo& info = m_classes[script.getUUID()];
  if (!compileClass(script, info)) {
    releaseClass(info);
    return &info;
  }
  info.bValid = true;
  return &info;
}

/*
 */
bool
ScriptManager::compileClass(const ScriptAsset& script, ScriptClassInfo& outInfo)
{
  const String chunkName = script.getChunkName();
  const String& source = script.getSource();

  const int32 base = lua_gettop(m_state);
  lua_pushcfunction(m_state, &traceback);
  if (luaL_loadbufferx(m_state, source.data(), source.size(), chunkName.c_str(), "t") !=
      LUA_OK) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
    lua_settop(m_state, base);
    return false;
  }
  if (lua_pcall(m_state, 0, 1, base + 1) != LUA_OK) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
    lua_settop(m_state, base);
    return false;
  }
  if (!lua_istable(m_state, -1)) {
    CH_LOG_ERROR(ScriptLog,
                 "Script {0} must return a table, its class (return MyScript); it returned "
                 "a {1}.",
                 script.getFilePath(), luaL_typename(m_state, -1));
    lua_settop(m_state, base);
    return false;
  }

  lua_pushcfunction(m_state, &readClass);
  lua_insert(m_state, -2);
  lua_pushlightuserdata(m_state, &outInfo);
  const bool bValid = lua_pcall(m_state, 2, 0, base + 1) == LUA_OK;
  if (!bValid) {
    CH_LOG_ERROR(ScriptLog, "Script {0}: {1}", script.getFilePath(),
                 getErrorText(m_state));
  }
  lua_settop(m_state, base);
  return bValid;
}

/*
 */
void
ScriptManager::releaseClass(ScriptClassInfo& info) noexcept
{
  for (int32* reference : {&info.classTable, &info.instanceMetatable, &info.onStart,
                           &info.update, &info.fixedUpdate, &info.onDestroy}) {
    releaseReference(*reference);
    *reference = ScriptClassInfo::NO_REFERENCE;
  }
  info.properties.clear();
  info.bValid = false;
}

/*
 */
void
ScriptManager::forgetClass(const UUID& scriptId)
{
  const auto it = m_classes.find(scriptId);
  if (it == m_classes.end()) {
    return;
  }
  releaseClass(it->second);
  m_classes.erase(it);
}

#if USING(CH_EDITOR)
/*
 */
bool
ScriptManager::reloadScript(const SPtr<ScriptAsset>& script)
{
  if (!script || !script->isLoaded() || !AssetManager::isStarted()) {
    return false;
  }
  AssetManager& assetManager = AssetManager::instance();
  if (!assetManager.unloadAsset(script) || !assetManager.syncLoadAsset(script)) {
    CH_LOG_ERROR(ScriptLog, "Could not read script {0} again.", script->getFilePath());
    return false;
  }
  if (!updateClass(*script)) {
    return false;
  }
  CH_LOG_INFO(ScriptLog, "Script {0} reloaded.", script->getFilePath());
  return true;
}

/*
 */
bool
ScriptManager::updateClass(const ScriptAsset& script)
{
  // Nothing was compiled from it yet, so its first use reads the new text.
  const auto it = m_classes.find(script.getUUID());
  if (it == m_classes.end()) {
    return true;
  }

  ScriptClassInfo fresh;
  if (!compileClass(script, fresh)) {
    releaseClass(fresh);
    if (it->second.bValid) {
      CH_LOG_WARNING(ScriptLog, "Script {0} keeps running its previous version.",
                     script.getFilePath());
    }
    return false;
  }

  // Running instances hold the old metatable, so it stays and reads from the new class.
  ScriptClassInfo& info = it->second;
  if (info.instanceMetatable != ScriptClassInfo::NO_REFERENCE) {
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, info.instanceMetatable);
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, fresh.classTable);
    lua_setfield(m_state, -2, "__index");
    lua_pop(m_state, 1);
    releaseReference(fresh.instanceMetatable);
    fresh.instanceMetatable = info.instanceMetatable;
    info.instanceMetatable = ScriptClassInfo::NO_REFERENCE;
  }
  const uint32 revision = info.revision + 1;
  releaseClass(info);
  info = std::move(fresh);
  info.bValid = true;
  info.revision = revision;
  return true;
}
#endif

/*
 */
void
ScriptManager::reloadScripts()
{
  while (!m_classes.empty()) {
    forgetClass(m_classes.begin()->first);
  }
  if (!AssetManager::isStarted()) {
    return;
  }
  AssetManager& assetManager = AssetManager::instance();
  for (const SPtr<IAsset>& asset : assetManager.getAllAssets()) {
    if (asset->isTypeOf<ScriptAsset>() && asset->isLoaded() &&
        (!assetManager.unloadAsset(asset) || !assetManager.syncLoadAsset(asset))) {
      CH_LOG_ERROR(ScriptLog, "Could not read script {0} again.", asset->getFilePath());
    }
  }
}

/*
 */
int32
ScriptManager::createInstance(const ScriptClassInfo& scriptClass,
                              GameObject& owner,
                              Span<const ScriptPropertyOverride> overrides)
{
  lua_createtable(m_state, 0, 4 + static_cast<int32>(scriptClass.properties.size()));
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, scriptClass.instanceMetatable);
  lua_setmetatable(m_state, -2);
  ScriptTraits<GameObject>::push(m_state, owner);
  lua_setfield(m_state, -2, "gameObject");
  ScriptTraits<Transform>::pushOf(m_state, owner);
  lua_setfield(m_state, -2, "transform");
  // Every instance gets its own copy, so a vector changed in place never reaches the class.
  for (const ScriptProperty& property : scriptClass.properties) {
    lua_pushlstring(m_state, property.name.data(), property.name.size());
    pushPropertyValue(m_state, getPropertyValue(property, overrides), owner.getScene());
    lua_rawset(m_state, -3);
  }
  return luaL_ref(m_state, LUA_REGISTRYINDEX);
}

/*
 */
void
ScriptManager::setInstanceProperty(const int32 instance,
                                   const StringView name,
                                   const ScriptPropertyValue& value,
                                   const Scene* scene)
{
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, instance);
  lua_pushlstring(m_state, name.data(), name.size());
  pushPropertyValue(m_state, value, scene);
  lua_rawset(m_state, -3);
  lua_pop(m_state, 1);
}

#if USING(CH_EDITOR)
/*
 */
bool
ScriptManager::getInstanceProperty(const int32 instance,
                                   const StringView name,
                                   const ScriptPropertyType type,
                                   ScriptPropertyValue& outValue)
{
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, instance);
  lua_pushlstring(m_state, name.data(), name.size());
  lua_rawget(m_state, -2);
  const bool bRead = toPropertyValue(m_state, -1, type, outValue);
  lua_pop(m_state, 2);
  return bRead;
}

/*
 */
void
ScriptManager::addMissingProperties(const int32 instance,
                                    const ScriptClassInfo& scriptClass,
                                    Span<const ScriptPropertyOverride> overrides,
                                    const Scene* scene)
{
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, instance);
  for (const ScriptProperty& property : scriptClass.properties) {
    lua_pushlstring(m_state, property.name.data(), property.name.size());
    if (lua_rawget(m_state, -2) == LUA_TNIL) {
      lua_pushlstring(m_state, property.name.data(), property.name.size());
      pushPropertyValue(m_state, getPropertyValue(property, overrides), scene);
      lua_rawset(m_state, -4);
    }
    lua_pop(m_state, 1);
  }
  lua_pop(m_state, 1);
}
#endif

/*
 */
void
ScriptManager::captureNextKey(const int32 callback)
{
  CH_ASSERT(m_input);
  const SPtr<ScriptKeyCapture> capture = chMakeShared<ScriptKeyCapture>(m_state, callback);
  m_keyCapture = capture;
  m_input->captureNextKey([this, capture](const InputKey key) {
    // An error is already logged; there is nothing to stop.
    static_cast<void>(callFunction(capture->callback, static_cast<int64>(key)));
  });
}

/*
 */
void
ScriptManager::releaseReference(const int32 reference) noexcept
{
  luaL_unref(m_state, LUA_REGISTRYINDEX, reference);
}

/*
 */
bool
ScriptManager::callMethod(const int32 function, const int32 instance)
{
  lua_pushcfunction(m_state, &traceback);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, function);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, instance);
  return protectedCall(1);
}

/*
 */
bool
ScriptManager::callMethod(const int32 function, const int32 instance, const float deltaTime)
{
  lua_pushcfunction(m_state, &traceback);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, function);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, instance);
  lua_pushnumber(m_state, static_cast<lua_Number>(deltaTime));
  return protectedCall(2);
}

/*
 */
bool
ScriptManager::callFunction(const int32 function, const int64 argument)
{
  lua_pushcfunction(m_state, &traceback);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, function);
  lua_pushinteger(m_state, static_cast<lua_Integer>(argument));
  return protectedCall(1);
}

/*
 */
bool
ScriptManager::protectedCall(const int32 argumentCount)
{
  const int32 handler = lua_gettop(m_state) - argumentCount - 1;
  const bool bSucceeded = lua_pcall(m_state, argumentCount, 0, handler) == LUA_OK;
  if (!bSucceeded) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
  }
  lua_settop(m_state, handler - 1);
  return bSucceeded;
}

/*
 */
void
ScriptManager::beginUpdate(const float deltaTime)
{
  m_deltaTime = deltaTime;
  m_time += static_cast<double>(deltaTime);
  ++m_frameCount;
#if USING(CH_DEBUG_DRAW)
  m_bInFixedStep = false;
#endif
  writeTime();
}

/*
 */
void
ScriptManager::beginFixedUpdate(const float fixedDeltaTime)
{
  m_deltaTime = fixedDeltaTime;
  m_fixedDeltaTime = fixedDeltaTime;
#if USING(CH_DEBUG_DRAW)
  m_bInFixedStep = true;
#endif
  writeTime();
}

/*
 */
void
ScriptManager::resetTime()
{
  m_time = 0.0;
  m_frameCount = 0;
  writeTime();
}

/*
 */
void
ScriptManager::writeTime()
{
  // Raw sets, so a metatable a script put on Time cannot raise an error here.
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_timeTable);
  lua_pushliteral(m_state, "deltaTime");
  lua_pushnumber(m_state, static_cast<lua_Number>(m_deltaTime));
  lua_rawset(m_state, -3);
  lua_pushliteral(m_state, "fixedDeltaTime");
  lua_pushnumber(m_state, static_cast<lua_Number>(m_fixedDeltaTime));
  lua_rawset(m_state, -3);
  lua_pushliteral(m_state, "time");
  lua_pushnumber(m_state, static_cast<lua_Number>(m_time));
  lua_rawset(m_state, -3);
  lua_pushliteral(m_state, "frameCount");
  lua_pushinteger(m_state, static_cast<lua_Integer>(m_frameCount));
  lua_rawset(m_state, -3);
  lua_pop(m_state, 1);
}

/*
 */
uint64
ScriptManager::getAllocationCount() const noexcept
{
  return m_allocator->getAllocationCount();
}

/*
 */
SIZE_T
ScriptManager::getBytesInUse() const noexcept
{
  return m_allocator->getBytesInUse();
}

/*
 */
void
ScriptManager::collectGarbage()
{
  lua_gc(m_state, LUA_GCCOLLECT);
}

} // namespace chEngineSDK
