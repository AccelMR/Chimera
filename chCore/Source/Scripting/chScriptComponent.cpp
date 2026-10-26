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

#include "chAlgorithm.h"
#include "chAssetFile.h"
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
  const UUID scriptId = script ? script->getUUID() : UUID::null();
  // Values of another script would only show as unused.
  if (scriptId != m_scriptId) {
    m_overrides.clear();
  }
  m_script = script;
  m_scriptId = scriptId;
  m_bFailed = false;
}

/*
 */
const ScriptPropertyValue*
ScriptComponent::findOverride(const StringView name) const
{
  for (const ScriptPropertyOverride& entry : m_overrides) {
    if (entry.name == name) {
      return &entry.value;
    }
  }
  return nullptr;
}

/*
 */
void
ScriptComponent::setOverride(const StringView name, const ScriptPropertyValue& value)
{
  bool bFound = false;
  for (ScriptPropertyOverride& entry : m_overrides) {
    if (entry.name == name) {
      entry.value = value;
      bFound = true;
      break;
    }
  }
  if (!bFound) {
    m_overrides.push_back({.name = String(name), .value = value});
  }
  writeToInstance(name);
}

/*
 */
bool
ScriptComponent::removeOverride(const StringView name)
{
  const bool bRemoved = Algorithm::removeFirstIf(
      m_overrides, [name](const ScriptPropertyOverride& entry) { return entry.name == name; });
  if (bRemoved) {
    writeToInstance(name);
  }
  return bRemoved;
}

/*
 */
const ScriptClassInfo*
ScriptComponent::getScriptClass() const
{
  if (!m_script || !ScriptManager::isStarted()) {
    return nullptr;
  }
  return ScriptManager::instance().getClass(*m_script);
}

/*
 */
void
ScriptComponent::writeToInstance(const StringView name)
{
  if (!isRunning() || !ScriptManager::isStarted()) {
    return;
  }
  for (const ScriptProperty& property : m_class->properties) {
    if (property.name != name) {
      continue;
    }
    const ScriptPropertyValue* value = findOverride(name);
    if (!value || value->type != property.defaultValue.type) {
      value = &property.defaultValue;
    }
    ScriptManager::instance().setInstanceProperty(m_instance, name, *value,
                                                  getOwner()->getScene());
    return;
  }
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
#if USING(CH_EDITOR)
    if (m_class->revision != m_instanceRevision) {
      m_instanceRevision = m_class->revision;
      ScriptManager::instance().addMissingProperties(m_instance, *m_class, m_overrides,
                                                     getOwner()->getScene());
    }
#endif
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

  m_instance = scriptManager.createInstance(*m_class, *getOwner(), m_overrides);
#if USING(CH_EDITOR)
  m_instanceRevision = m_class->revision;
#endif
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
  const uint32 count = static_cast<uint32>(m_overrides.size());
  stream.write(&count, sizeof(count));
  for (const ScriptPropertyOverride& entry : m_overrides) {
    AssetFile::writeString(stream, entry.name);
    ScriptPropertyUtils::writeValue(stream, entry.value);
  }
}

/*
 */
bool
ScriptComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  UUID scriptId = UUID::null();
  uint32 count = 0;
  if (!AssetFile::readValue(stream, version) || version != SERIALIZATION_VERSION ||
      !AssetFile::readValue(stream, scriptId) || !AssetFile::readValue(stream, count) ||
      count > ScriptPropertyUtils::MAX_PROPERTIES) {
    return false;
  }
  Vector<ScriptPropertyOverride> overrides(count);
  for (ScriptPropertyOverride& entry : overrides) {
    if (!AssetFile::readString(stream, entry.name, ScriptPropertyUtils::MAX_NAME_LENGTH) ||
        !ScriptPropertyUtils::readValue(stream, entry.value)) {
      return false;
    }
  }

  SPtr<ScriptAsset> script;
  if (!scriptId.isNull() && AssetManager::isStarted()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(scriptId);
    script = asset ? asset->as<ScriptAsset>() : nullptr;
  }
  setScript(script);
  m_scriptId = scriptId;
  m_overrides = std::move(overrides);
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
  // So the prefabs a script spawns load with the scene and go into packages.
  for (const ScriptPropertyOverride& entry : m_overrides) {
    if (entry.value.type == ScriptPropertyType::Prefab && !entry.value.object.isNull()) {
      outReferences.push_back(entry.value.object);
    }
  }
}

/*
 */
void
ScriptComponent::remapObjectReferences(const UnorderedMap<UUID, UUID>& newIds)
{
  for (ScriptPropertyOverride& entry : m_overrides) {
    if (entry.value.type != ScriptPropertyType::GameObject) {
      continue;
    }
    const auto it = newIds.find(entry.value.object);
    if (it != newIds.end()) {
      entry.value.object = it->second;
    }
  }
}

} // namespace chEngineSDK
