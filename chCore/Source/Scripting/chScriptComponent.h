/************************************************************************/
/**
 * @file chScriptComponent.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Runs a Lua script on a GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chComponent.h"
#include "chScriptManager.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class ScriptAsset;

/**
 * Exists so game logic written in Lua lives on a GameObject like any other behaviour. The
 * script returns a class table; each component gets its own instance of it (self), whose
 * missing fields are read from the class. onStart(self) runs before the first update, then
 * update(self, deltaTime) every frame and fixedUpdate(self, deltaTime) at the fixed rate;
 * onDestroy(self) runs when the component leaves the scene (object destroyed, component
 * disabled, scene closed, Play stopped). Scripts only run while the scene plays. An error
 * is logged with its traceback and stops this component until it is registered again.
 * The values the script lists in its properties are set in self before onStart, from this
 * component's overrides or else the script's defaults.
 */
class CH_CORE_EXPORT ScriptComponent : public Component
{
 public:
  ScriptComponent();

  explicit ScriptComponent(const SPtr<ScriptAsset>& script);

  ~ScriptComponent() override;

  /**
   * Stops the running instance (onDestroy) first. Null removes the script.
   */
  void
  setScript(const SPtr<ScriptAsset>& script);

  /**
   * Null when there is none or its asset is missing.
   */
  NODISCARD FORCEINLINE const SPtr<ScriptAsset>&
  getScript() const noexcept { return m_script; }

  /**
   * Kept when the asset is missing, so saving again keeps the reference.
   */
  NODISCARD FORCEINLINE const UUID&
  getScriptId() const noexcept { return m_scriptId; }

  /**
   * True from onStart until onDestroy.
   */
  NODISCARD FORCEINLINE bool
  isRunning() const noexcept { return m_instance != ScriptClassInfo::NO_REFERENCE; }

  /**
   * Makes the instance and runs onStart now if that did not happen yet, so another script
   * can use this one before its first update. False when it cannot run: the script fails,
   * the component is not in a playing scene or is disabled.
   */
  NODISCARD bool
  start();

#if USING(CH_EDITOR)
  /**
   * Stops the running instance (onDestroy) and starts a new one (onStart), also after an
   * error. False when it cannot run, as start.
   */
  bool
  restart();
#endif

  /**
   * The values this component gives to properties of its script, instead of their defaults.
   * Saved by name, so a property the script no longer lists keeps its value until removed.
   */
  NODISCARD FORCEINLINE const Vector<ScriptPropertyOverride>&
  getOverrides() const noexcept { return m_overrides; }

  /**
   * Null when the component does not override name.
   */
  NODISCARD const ScriptPropertyValue*
  findOverride(StringView name) const;

  /**
   * Adds or replaces the override of name. A running instance gets the value in self.
   */
  void
  setOverride(StringView name, const ScriptPropertyValue& value);

  /**
   * Back to the default: a running instance gets the default in self. False when name had
   * no override.
   */
  bool
  removeOverride(StringView name);

  /**
   * The class of the script, compiled when needed (also outside Play, so the editor can list
   * its properties). Null without a script or when it is not loaded.
   */
  NODISCARD const ScriptClassInfo*
  getScriptClass() const;

  /**
   * Registry reference of the instance (self) while running.
   */
  NODISCARD FORCEINLINE int32
  getInstance() const noexcept { return m_instance; }

  /**
   * True after an error, until the component is registered again.
   */
  NODISCARD FORCEINLINE bool
  hasFailed() const noexcept { return m_bFailed; }

  void
  fixedUpdate(float deltaTime) override;

  void
  update(float deltaTime) override;

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

  void
  remapObjectReferences(const UnorderedMap<UUID, UUID>& newIds) override;

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  void
  onUnregister() override;

 private:
  /*
   * Makes the instance and runs onStart the first time. False when the script cannot run.
   */
  NODISCARD bool
  ensureStarted();

  void
  stop();

  void
  fail();

  /*
   * Gives the running instance the current value of name (override or default).
   */
  void
  writeToInstance(StringView name);

  SPtr<ScriptAsset> m_script;
  UUID m_scriptId = UUID::null();
  Vector<ScriptPropertyOverride> m_overrides;
  const ScriptClassInfo* m_class = nullptr;
  int32 m_instance = ScriptClassInfo::NO_REFERENCE;
  bool m_bFailed = false;
#if USING(CH_EDITOR)
  // Revision of the class when it failed: a newer one is tried again.
  uint32 m_failedRevision = 0;
  // Revision of the class the instance was made with: after a reload it gets the
  // properties that were added.
  uint32 m_instanceRevision = 0;
#endif
};

DECLARE_TYPE_TRAITS(ScriptComponent)

} // namespace chEngineSDK
