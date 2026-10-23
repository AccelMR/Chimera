/************************************************************************/
/**
 * @file chScriptManager.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The Lua runtime that runs the game scripts.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLogger.h"
#include "chModule.h"
#include "chScriptProperty.h"
#include "chUUID.h"

struct lua_State;

namespace chEngineSDK {
class GameObject;
class InputManager;
class Scene;
class ScriptAllocator;
class ScriptApiDescription;
class ScriptAsset;

CH_LOG_DECLARE_EXTERN(CH_CORE_EXPORT, ScriptLog);

/**
 * Exists so calling a script costs no lookup by name: what a script returned (its class table)
 * and the functions the engine calls on it are kept as references into the Lua registry.
 */
struct ScriptClassInfo
{
  int32 classTable = NO_REFERENCE;
  // Metatable of the instances: missing fields are read from the class.
  int32 instanceMetatable = NO_REFERENCE;
  int32 onStart = NO_REFERENCE;
  int32 update = NO_REFERENCE;
  int32 fixedUpdate = NO_REFERENCE;
  int32 onDestroy = NO_REFERENCE;
  // In the order the script lists them.
  Vector<ScriptProperty> properties;
  // False when the script did not compile, failed while it ran or returned no table.
  bool bValid = false;
#if USING(CH_EDITOR)
  // Goes up every time the script is read again and compiles, so components that stopped
  // after an error try the new code.
  uint32 revision = 0;
#endif

  // LUA_NOREF.
  static constexpr int32 NO_REFERENCE = -2;
};

/**
 * Exists to own the one Lua state of the application and to be the only place that knows its
 * rules: scripts get the safe standard libraries (no files, no OS, no loading of code), print
 * goes to the log, and every call into a script is protected, so an error is logged with its
 * traceback and never stops the engine. Script classes are compiled on first use and kept by
 * the UUID of their asset. Main thread only.
 */
class CH_CORE_EXPORT ScriptManager : public Module<ScriptManager>
{
 public:
  /**
   * Reads Script.PoolAllocator and Script.GenerationalGC.
   */
  ScriptManager();

  /**
   * bPoolAllocator picks the allocator for this state only (tests compare both).
   */
  explicit ScriptManager(bool bPoolAllocator);

  ~ScriptManager() override;

  NODISCARD FORCEINLINE lua_State*
  getState() const noexcept { return m_state; }

  /**
   * Runs text as a script. False when it does not compile or raises an error (logged).
   */
  NODISCARD bool
  runString(StringView code, const ANSICHAR* chunkName = "=script");

  /**
   * The class the script returns, compiled the first time it is asked for. Null when the
   * script is not loaded; a script that fails is logged once and gives a class that is not
   * valid.
   */
  NODISCARD const ScriptClassInfo*
  getClass(const ScriptAsset& script);

  /**
   * Compiles the script again on its next use (after its text changed).
   */
  void
  forgetClass(const UUID& scriptId);

  /**
   * Reads every loaded script again from its file and forgets every compiled class, so the
   * next use runs what is on disk. Call it while no script component runs (before Play):
   * running ones keep pointers to their class.
   */
  void
  reloadScripts();

#if USING(CH_EDITOR)
  /**
   * Reads the script again from its file while its components may be running. A compiled
   * class keeps its place: running instances keep their fields and call the new functions
   * from their next call (onStart does not run again). When the new text fails, the error is
   * logged and the previous functions stay. False when it could not be read or compiled.
   */
  NODISCARD bool
  reloadScript(const SPtr<ScriptAsset>& script);

  /**
   * The part of reloadScript after the file is read: compiles the script's current text
   * into its class, in place. True when nothing was compiled from it yet.
   */
  NODISCARD bool
  updateClass(const ScriptAsset& script);
#endif

  /**
   * A new table whose missing fields are read from the class, with self.gameObject and
   * self.transform set to owner and every property of the class set to its override (same
   * name and type) or its default, as a registry reference. GameObject properties are looked
   * up in the owner's scene; a missing object is nil.
   */
  NODISCARD int32
  createInstance(const ScriptClassInfo& scriptClass,
                 GameObject& owner,
                 Span<const ScriptPropertyOverride> overrides = {});

  /**
   * self[name] = value on a running instance.
   */
  void
  setInstanceProperty(int32 instance,
                      StringView name,
                      const ScriptPropertyValue& value,
                      const Scene* scene);

#if USING(CH_EDITOR)

  /**
   * The value self holds for name, read as type. False when self holds nothing or a value of
   * another type.
   */
  NODISCARD bool
  getInstanceProperty(int32 instance,
                      StringView name,
                      ScriptPropertyType type,
                      ScriptPropertyValue& outValue);

  /**
   * Sets the properties the class declares that self does not hold yet, so a property added
   * to a running script reaches its instances. The others keep what the script made of them.
   */
  void
  addMissingProperties(int32 instance,
                       const ScriptClassInfo& scriptClass,
                       Span<const ScriptPropertyOverride> overrides,
                       const Scene* scene);
#endif

  /**
   * The scene Scene.find looks in: the scene of the script that runs. Script components set
   * it before each call.
   */
  FORCEINLINE void
  setCurrentScene(Scene* scene) noexcept { m_currentScene = scene; }

  NODISCARD FORCEINLINE Scene*
  getCurrentScene() const noexcept { return m_currentScene; }

#if USING(CH_DEBUG_DRAW)
  /**
   * Between beginFixedUpdate and the next beginUpdate, so lines drawn by scripts go to the
   * scene's fixed step list and stay on screen until the next step.
   */
  NODISCARD FORCEINLINE bool
  isInFixedStep() const noexcept { return m_bInFixedStep; }
#endif

  /**
   * The input the Input table reads: the InputManager module when it runs while this starts;
   * tests give a local one. Without one, Input functions raise an error.
   */
  FORCEINLINE void
  setInputManager(InputManager* input) noexcept { m_input = input; }

  NODISCARD FORCEINLINE InputManager*
  getInputManager() const noexcept { return m_input; }

  /**
   * Gives the next key that goes down to the Lua function behind callback (a registry
   * reference this takes over), called with the key's number. Needs an InputManager.
   */
  void
  captureNextKey(int32 callback);

  void
  releaseReference(int32 reference) noexcept;

  /**
   * Calls function(instance) or function(instance, deltaTime), protected. False when it
   * raised an error, which is logged with its traceback.
   */
  NODISCARD bool
  callMethod(int32 function, int32 instance);

  NODISCARD bool
  callMethod(int32 function, int32 instance, float deltaTime);

  /**
   * Calls function(argument), protected. False when it raised an error (logged).
   */
  NODISCARD bool
  callFunction(int32 function, int64 argument);

  /**
   * Sets Time.deltaTime and advances Time.time and Time.frameCount, once per frame before
   * the scripts update.
   */
  void
  beginUpdate(float deltaTime);

  /**
   * Sets Time.deltaTime and Time.fixedDeltaTime for the fixed step that follows.
   */
  void
  beginFixedUpdate(float fixedDeltaTime);

  /**
   * Puts Time.time and Time.frameCount back to zero (a new Play).
   */
  void
  resetTime();

  /**
   * Blocks the Lua state asked its allocator for since it was made.
   */
  NODISCARD uint64
  getAllocationCount() const noexcept;

  NODISCARD SIZE_T
  getBytesInUse() const noexcept;

  /**
   * Runs a full garbage collection.
   */
  void
  collectGarbage();

  /**
   * What scripts can call, for code editors; null outside the editor.
   */
  NODISCARD FORCEINLINE const ScriptApiDescription*
  getApiDescription() const noexcept { return m_apiDescription.get(); }

 private:
  void
  openLibraries();

  /*
   * Runs the script and reads the class it returns into outInfo. False (logged) when it
   * fails; outInfo may then hold some references, which releaseClass frees.
   */
  NODISCARD bool
  compileClass(const ScriptAsset& script, ScriptClassInfo& outInfo);

  void
  releaseClass(ScriptClassInfo& info) noexcept;

  void
  writeTime();

  /*
   * Runs the function and the arguments on top of the stack, protected; logs and pops the
   * error. The stack is left as it was before the function was pushed.
   */
  NODISCARD bool
  protectedCall(int32 argumentCount);

  UniquePtr<ScriptAllocator> m_allocator;
  UniquePtr<ScriptApiDescription> m_apiDescription;
  lua_State* m_state = nullptr;
  UnorderedMap<UUID, ScriptClassInfo> m_classes;
  Scene* m_currentScene = nullptr;
  InputManager* m_input = nullptr;
  // Alive while the InputManager holds a key capture a script asked for.
  WeakPtr<void> m_keyCapture;
  int32 m_timeTable = ScriptClassInfo::NO_REFERENCE;
  double m_time = 0.0;
  float m_deltaTime = 0.0f;
  float m_fixedDeltaTime = 0.0f;
  uint64 m_frameCount = 0;
#if USING(CH_DEBUG_DRAW)
  bool m_bInFixedStep = false;
#endif
};

} // namespace chEngineSDK
