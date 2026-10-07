/************************************************************************/
/**
 * @file chGameObject.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  GameObject class representing an entity in the scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chComponent.h"
#include "chObject.h"
#include "chTransform.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists as the node of a scene hierarchy: it has a transform relative to its parent, owns
 * its children and its components. Make it with Scene::createGameObject and remove it with
 * Scene::destroyGameObject, so the scene can register and unregister its components.
 */
class CH_CORE_EXPORT GameObject : public Object
{
 public:
  /**
   * scene may be null for an object outside any scene; its components are then never
   * registered.
   */
  GameObject(const String& name, Scene* scene);

  ~GameObject() override;

  GameObject(const GameObject&) = delete;

  GameObject&
  operator=(const GameObject&) = delete;

  /**
   * Creates a component of type T (which needs DECLARE_TYPE_TRAITS) and registers it when
   * the object is in a scene.
   */
  template<typename T, typename... Args>
  T&
  addComponent(Args&&... args);

  /**
   * First component created as exactly T, or null.
   */
  template<typename T>
  NODISCARD T*
  getComponent() const;

  NODISCARD FORCEINLINE const Vector<UniquePtr<Component>>&
  getComponents() const { return m_components; }

  void
  update(float deltaTime);

  NODISCARD FORCEINLINE Transform&
  getTransform() { return m_transform; }

  NODISCARD FORCEINLINE const Transform&
  getTransform() const { return m_transform; }

  NODISCARD FORCEINLINE Scene*
  getScene() const { return m_scene; }

  NODISCARD FORCEINLINE GameObject*
  getParent() const { return m_parent; }

  NODISCARD FORCEINLINE const Vector<SPtr<GameObject>>&
  getChildren() const { return m_children; }

 private:
  friend class Scene;

  Component&
  attachComponent(UniquePtr<Component>&& component, const UUID& typeId);

  /**
   * Registers or unregisters the components of this object and of all its children. An
   * object that leaves its scene forgets it.
   */
  void
  setRegisteredRecursive(bool registered);

  void
  updateWorldMatrices(const Matrix4& parentWorldMatrix, bool parentChanged);

  Transform m_transform;
  Vector<UniquePtr<Component>> m_components;
  Vector<SPtr<GameObject>> m_children;
  GameObject* m_parent = nullptr;
  Scene* m_scene = nullptr;
};

/*
 */
template<typename T, typename... Args>
T&
GameObject::addComponent(Args&&... args)
{
  static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");
  static_assert(TypeTraits<T>::DECLARED, "T needs DECLARE_TYPE_TRAITS.");
  // UniquePtr<T> does not convert to UniquePtr<Component>, because their deleters differ.
  UniquePtr<Component> component(new T(std::forward<Args>(args)...));
  return static_cast<T&>(attachComponent(std::move(component), TypeTraits<T>::getTypeId()));
}

/*
 */
template<typename T>
T*
GameObject::getComponent() const
{
  static_assert(TypeTraits<T>::DECLARED, "T needs DECLARE_TYPE_TRAITS.");
  const UUID& typeId = TypeTraits<T>::getTypeId();
  for (const UniquePtr<Component>& component : m_components) {
    if (component->getTypeId() == typeId) {
      return static_cast<T*>(component.get());
    }
  }
  return nullptr;
}

} // namespace chEngineSDK
