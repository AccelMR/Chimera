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
#include "chScriptAllocator.h"
#include "chScriptAsset.h"
#include "chScriptBinder.h"
#include "chScriptBindings.h"

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
  return 0;
}
} // namespace

/*
 */
ScriptManager::ScriptManager()
  : ScriptManager(g_cvarPoolAllocator.get())
{}

/*
 */
ScriptManager::ScriptManager(const bool bPoolAllocator)
  : m_allocator(chMakeUnique<ScriptAllocator>(bPoolAllocator))
{
  m_state = lua_newstate(&ScriptAllocator::allocate, m_allocator.get());
  CH_ASSERT(m_state && "Not enough memory to start the script runtime.");
  lua_atpanic(m_state, &panic);
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

  const luaL_Reg logFunctions[] = {{"info", &logInfo},
                                   {"warning", &logWarning},
                                   {"error", &logError},
                                   {"debug", &logDebug},
                                   {nullptr, nullptr}};
  luaL_newlib(m_state, logFunctions);
  lua_setglobal(m_state, "Log");

  lua_createtable(m_state, 0, 4);
  lua_pushvalue(m_state, -1);
  m_timeTable = luaL_ref(m_state, LUA_REGISTRYINDEX);
  lua_setglobal(m_state, "Time");
  writeTime();

  ScriptBindings::registerMath(m_state);
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
  const String chunkName = script.getChunkName();
  const String& source = script.getSource();

  const int32 base = lua_gettop(m_state);
  lua_pushcfunction(m_state, &traceback);
  if (luaL_loadbufferx(m_state, source.data(), source.size(), chunkName.c_str(), "t") !=
      LUA_OK) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
    lua_settop(m_state, base);
    return &info;
  }
  if (lua_pcall(m_state, 0, 1, base + 1) != LUA_OK) {
    CH_LOG_ERROR(ScriptLog, "{0}", getErrorText(m_state));
    lua_settop(m_state, base);
    return &info;
  }
  if (!lua_istable(m_state, -1)) {
    CH_LOG_ERROR(ScriptLog,
                 "Script {0} must return a table, its class (return MyScript); it returned "
                 "a {1}.",
                 script.getFilePath(), luaL_typename(m_state, -1));
    lua_settop(m_state, base);
    return &info;
  }

  lua_pushcfunction(m_state, &readClass);
  lua_insert(m_state, -2);
  lua_pushlightuserdata(m_state, &info);
  info.bValid = lua_pcall(m_state, 2, 0, base + 1) == LUA_OK;
  if (!info.bValid) {
    CH_LOG_ERROR(ScriptLog, "Script {0}: {1}", script.getFilePath(),
                 getErrorText(m_state));
  }
  lua_settop(m_state, base);
  return &info;
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
  const ScriptClassInfo& info = it->second;
  for (const int32 reference : {info.classTable, info.instanceMetatable, info.onStart,
                                info.update, info.fixedUpdate, info.onDestroy}) {
    releaseReference(reference);
  }
  m_classes.erase(it);
}

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
ScriptManager::createInstance(const ScriptClassInfo& scriptClass)
{
  lua_createtable(m_state, 0, 4);
  lua_rawgeti(m_state, LUA_REGISTRYINDEX, scriptClass.instanceMetatable);
  lua_setmetatable(m_state, -2);
  return luaL_ref(m_state, LUA_REGISTRYINDEX);
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
  writeTime();
}

/*
 */
void
ScriptManager::beginFixedUpdate(const float fixedDeltaTime)
{
  m_deltaTime = fixedDeltaTime;
  m_fixedDeltaTime = fixedDeltaTime;
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
