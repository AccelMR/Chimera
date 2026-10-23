/************************************************************************/
/**
 * @file chScriptComponent.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Runs a Lua script on a GameObject.
 */
/************************************************************************/
#include "chScriptComponent.h"

#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chScene.h"
#include "chScriptAsset.h"

namespace chEngineSDK {

/*
 */
ScriptComponent::ScriptComponent(const SPtr<ScriptAsset>& script)
{
  setScript(script);
}

/*
 */
ScriptComponent::~ScriptComponent()
{
  // Unregistering already stopped it; this only matters for a component that never left a
  // scene properly.
  if (isRunning() && ScriptManager::isStarted()) {
    ScriptManager::instance().releaseReference(m_instance);
  }
}

/*
 */
void
ScriptComponent::setScript(const SPtr<ScriptAsset>& script)
{
  stop();
  m_script = script;
  m_scriptId = script ? script->getUUID() : UUID::null();
  m_bFailed = false;
}

/*
 */
void
ScriptComponent::fixedUpdate(const float deltaTime)
{
  if (!ensureStarted() || m_class->fixedUpdate == ScriptClassInfo::NO_REFERENCE) {
    return;
  }
  ScriptManager& scriptManager = ScriptManager::instance();
  scriptManager.setCurrentScene(getOwner()->getScene());
  if (!scriptManager.callMethod(m_class->fixedUpdate, m_instance, deltaTime)) {
    fail();
  }
}

/*
 */
void
ScriptComponent::update(const float deltaTime)
{
  if (!ensureStarted() || m_class->update == ScriptClassInfo::NO_REFERENCE) {
    return;
  }
  ScriptManager& scriptManager = ScriptManager::instance();
  scriptManager.setCurrentScene(getOwner()->getScene());
  if (!scriptManager.callMethod(m_class->update, m_instance, deltaTime)) {
    fail();
  }
}

/*
 */
bool
ScriptComponent::start()
{
  const Scene* scene = getOwner() ? getOwner()->getScene() : nullptr;
  if (!isRegistered() || !scene || !scene->isPlaying()) {
    return false;
  }
  return ensureStarted();
}

/*
 */
bool
ScriptComponent::ensureStarted()
{
  if (isRunning()) {
    return true;
  }
  if (!m_script || !ScriptManager::isStarted()) {
    return false;
  }
  ScriptManager& scriptManager = ScriptManager::instance();
#if USING(CH_EDITOR)
  if (m_bFailed) {
    const ScriptClassInfo* scriptClass = scriptManager.getClass(*m_script);
    m_bFailed = !scriptClass || scriptClass->revision == m_failedRevision;
  }
#endif
  if (m_bFailed) {
    return false;
  }

  scriptManager.setCurrentScene(getOwner()->getScene());
  m_class = scriptManager.getClass(*m_script);
  if (!m_class || !m_class->bValid) {
    if (!m_class) {
      CH_LOG_ERROR(ScriptLog, "Script {0} on {1} is not loaded.", m_script->getName(),
                   getOwner()->getName());
    }
#if USING(CH_EDITOR)
    m_failedRevision = m_class ? m_class->revision : 0;
#endif
    m_class = nullptr;
    m_bFailed = true;
    return false;
  }

  m_instance = scriptManager.createInstance(*m_class, *getOwner());
  if (m_class->onStart != ScriptClassInfo::NO_REFERENCE &&
      !scriptManager.callMethod(m_class->onStart, m_instance)) {
    fail();
    return false;
  }
  return true;
}

/*
 */
void
ScriptComponent::stop()
{
  if (isRunning() && ScriptManager::isStarted()) {
    ScriptManager& scriptManager = ScriptManager::instance();
    scriptManager.setCurrentScene(getOwner()->getScene());
    if (m_class->onDestroy != ScriptClassInfo::NO_REFERENCE) {
      // An error here is logged; the instance goes anyway.
      static_cast<void>(scriptManager.callMethod(m_class->onDestroy, m_instance));
    }
    scriptManager.releaseReference(m_instance);
  }
  m_instance = ScriptClassInfo::NO_REFERENCE;
  m_class = nullptr;
}

/*
 */
void
ScriptComponent::fail()
{
  CH_LOG_WARNING(ScriptLog, "Script {0} on {1} stopped after an error.", m_script->getName(),
                 getOwner()->getName());
  if (isRunning() && ScriptManager::isStarted()) {
    ScriptManager::instance().releaseReference(m_instance);
  }
#if USING(CH_EDITOR)
  m_failedRevision = m_class ? m_class->revision : 0;
#endif
  m_instance = ScriptClassInfo::NO_REFERENCE;
  m_class = nullptr;
  m_bFailed = true;
}

#if USING(CH_EDITOR)
/*
 */
bool
ScriptComponent::restart()
{
  stop();
  m_bFailed = false;
  return start();
}
#endif

/*
 */
void
ScriptComponent::onUnregister()
{
  stop();
  m_bFailed = false;
}

/*
 */
void
ScriptComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  stream.write(&version, sizeof(version));
  stream.write(&m_scriptId, sizeof(m_scriptId));
}

/*
 */
bool
ScriptComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  UUID scriptId = UUID::null();
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION ||
      stream.read(&scriptId, sizeof(scriptId)) != sizeof(scriptId)) {
    return false;
  }

  SPtr<ScriptAsset> script;
  if (!scriptId.isNull() && AssetManager::isStarted()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(scriptId);
    script = asset ? asset->as<ScriptAsset>() : nullptr;
  }
  setScript(script);
  m_scriptId = scriptId;
  if (!script && !scriptId.isNull()) {
    CH_LOG_WARNING(ScriptLog, "A script component lost its script {0}.", scriptId);
  }
  return true;
}

/*
 */
void
ScriptComponent::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_scriptId.isNull()) {
    outReferences.push_back(m_scriptId);
  }
}

} // namespace chEngineSDK
