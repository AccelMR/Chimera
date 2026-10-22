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
 */
class CH_CORE_EXPORT ScriptComponent : public Component
{
 public:
  ScriptComponent() = default;

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

  SPtr<ScriptAsset> m_script;
  UUID m_scriptId = UUID::null();
  const ScriptClassInfo* m_class = nullptr;
  int32 m_instance = ScriptClassInfo::NO_REFERENCE;
  bool m_bFailed = false;
};

DECLARE_TYPE_TRAITS(ScriptComponent)

} // namespace chEngineSDK
