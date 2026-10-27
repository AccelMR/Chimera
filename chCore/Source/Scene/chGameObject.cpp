/************************************************************************/
/**
 * @file chGameObject.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @brief
 *  GameObject class representing an entity in the scene.
 */
/************************************************************************/
#include "chGameObject.h"

#include "chAlgorithm.h"
#include "chBox.h"
#include "chScene.h"
#include "chScriptManager.h"
#include "chScriptObjectTypes.h"
#include "chStringUtils.h"

namespace chEngineSDK {

/*
 */
GameObject::GameObject(const String& name, Scene* scene)
 : Object(name, UUID::createRandom()),
   m_scene(scene)
{
  m_transform.m_owner = this;
}

/*
 */
GameObject::~GameObject()
{
  // Children kept alive elsewhere must not stay registered in a scene they left.
  if (m_scene) {
    setRegisteredRecursive(false);
  }
  if (m_scriptHandle != ScriptClassInfo::NO_REFERENCE) {
    ScriptTraits<GameObject>::release(*this);
  }
}

/*
 */
bool
GameObject::isDestroyRequested() const
{
  for (const GameObject* current = this; current; current = current->m_parent) {
    if (current->m_bDestroyRequested) {
      return true;
    }
  }
  return false;
}

/*
 */
GameObject*
GameObject::getOutermostPrefabInstance()
{
  GameObject* outermost = nullptr;
  for (GameObject* current = this; current; current = current->m_parent) {
    if (!current->m_prefab.isNull()) {
      outermost = current;
    }
  }
  return outermost;
}

/*
 */
bool
GameObject::getHierarchyWorldBounds(AABox& outBounds) const
{
  bool bFound = false;
  const auto addBounds = [&outBounds, &bFound](const AABox& bounds) {
    if (bFound) {
      outBounds += bounds;
    }
    else {
      outBounds = bounds;
      bFound = true;
    }
  };

  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  for (const UniquePtr<Component>& component : m_components) {
    if (component->getWorldBounds(bounds)) {
      addBounds(bounds);
    }
  }
  for (const SPtr<GameObject>& child : m_children) {
    if (child->getHierarchyWorldBounds(bounds)) {
      addBounds(bounds);
    }
  }
  return bFound;
}

/*
 */
void
GameObject::formatTo(String& output, const FormatSpec& spec) const
{
  CH_PARAMETER_UNUSED(spec);
  output += "GameObject(";
  output += m_name;
  output += ", ";
  output += m_uuid.toString();
  output += ')';
}

/*
 */
String
GameObject::toString() const
{
  return StringUtils::toString(*this);
}

/*
 */
Component&
GameObject::attachComponent(UniquePtr<Component>&& component, const UUID& typeId)
{
  component->m_owner = this;
  component->m_typeId = typeId;
  Component& attached = *component;
  m_components.push_back(std::move(component));

  if (m_scene && attached.isEnabled()) {
    attached.setRegistered(true);
  }
  return attached;
}

/*
 */
void
GameObject::removeComponent(Component& component)
{
  CH_ASSERT(component.getOwner() == this);
  if (component.isRegistered()) {
    component.setRegistered(false);
  }
  Algorithm::removeFirstIf(m_components, [&component](const UniquePtr<Component>& owned) {
    return owned.get() == &component;
  });
}

/*
 */
void
GameObject::setRegisteredRecursive(bool registered)
{
  for (const UniquePtr<Component>& component : m_components) {
    if (component->isEnabled()) {
      component->setRegistered(registered);
    }
  }
  for (const SPtr<GameObject>& child : m_children) {
    child->setRegisteredRecursive(registered);
  }
  if (!registered && m_scene) {
    if (m_dirtyTransformSlot != NO_SLOT) {
      m_scene->removeDirtyTransform(*this);
    }
    m_scene = nullptr;
  }
}

/*
 */
void
GameObject::markTransformDirty()
{
  if (m_scene) {
    m_scene->addDirtyTransform(*this);
  }
}

/*
 */
void
GameObject::updateWorldMatrices(const Matrix4& parentWorldMatrix)
{
  m_transform.updateWorldMatrix(parentWorldMatrix);
  for (const UniquePtr<Component>& component : m_components) {
    if (component->isRegistered()) {
      component->onTransformChanged();
    }
  }

  for (const SPtr<GameObject>& child : m_children) {
    child->updateWorldMatrices(m_transform.getWorldMatrix());
  }
}

} // namespace chEngineSDK
