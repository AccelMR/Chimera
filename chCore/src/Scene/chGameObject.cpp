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

namespace chEngineSDK {

/*
 */
GameObject::GameObject(const String& name, Scene* scene)
 : Object(name, UUID::createRandom()),
   m_scene(scene)
{}

/*
 */
GameObject::~GameObject()
{
  // Children kept alive elsewhere must not stay registered in a scene they left.
  if (m_scene) {
    setRegisteredRecursive(false);
  }
}

/*
 */
void
GameObject::update(float deltaTime)
{
  for (const UniquePtr<Component>& component : m_components) {
    if (component->isEnabled()) {
      component->update(deltaTime);
    }
  }
  for (const SPtr<GameObject>& child : m_children) {
    child->update(deltaTime);
  }
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
  if (!registered) {
    m_scene = nullptr;
  }
}

/*
 */
void
GameObject::updateWorldMatrices(const Matrix4& parentWorldMatrix, bool parentChanged)
{
  const bool bChanged = parentChanged || m_transform.isDirty();
  if (bChanged) {
    m_transform.updateWorldMatrix(parentWorldMatrix);
    for (const UniquePtr<Component>& component : m_components) {
      if (component->isRegistered()) {
        component->onTransformChanged();
      }
    }
  }

  for (const SPtr<GameObject>& child : m_children) {
    child->updateWorldMatrices(m_transform.getWorldMatrix(), bChanged);
  }
}

} // namespace chEngineSDK
