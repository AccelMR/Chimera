/************************************************************************/
/**
 * @file chComponent.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Base component class that can be attached to GameObjects.
 */
/************************************************************************/
#include "chComponent.h"

#include "chGameObject.h"

namespace chEngineSDK {

/*
 */
void
Component::fixedUpdate(float deltaTime)
{
  CH_PARAMETER_UNUSED(deltaTime);
}

/*
 */
void
Component::update(float deltaTime)
{
  CH_PARAMETER_UNUSED(deltaTime);
}

/*
 */
void
Component::serialize(DataStream& stream) const
{
  CH_PARAMETER_UNUSED(stream);
}

/*
 */
bool
Component::deserialize(DataStream& stream)
{
  CH_PARAMETER_UNUSED(stream);
  return true;
}

/*
 */
void
Component::collectReferences(Vector<UUID>& outReferences) const
{
  CH_PARAMETER_UNUSED(outReferences);
}

/*
 */
bool
Component::getWorldBounds(AABox& outBounds) const
{
  CH_PARAMETER_UNUSED(outBounds);
  return false;
}

/*
 */
void
Component::setEnabled(bool enabled)
{
  if (m_enabled == enabled) {
    return;
  }
  m_enabled = enabled;

  if (m_owner && m_owner->getScene()) {
    setRegistered(enabled);
  }
}

/*
 */
void
Component::setRegistered(bool registered)
{
  if (m_registered == registered) {
    return;
  }
  m_registered = registered;

  if (registered) {
    onRegister();
  }
  else {
    onUnregister();
  }
}

} // namespace chEngineSDK
