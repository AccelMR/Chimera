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
#include "chTransform.h"
#include "chComponent.h"

namespace chEngineSDK {

class CH_CORE_EXPORT GameObject
{
 public:
  /**
   * Constructor
   * 
   * @param name Name of the GameObject
   */
  GameObject(const String& name);
  
  /**
   * Destructor
   */
  ~GameObject() = default;
  
  /**
   * Add a child GameObject
   * 
   * @param child GameObject to add as child
   */
  void addChild(SPtr<GameObject> child);
  
  /**
   * Remove a child GameObject
   * 
   * @param child GameObject to remove
   * @return True if child was removed
   */
  bool removeChild(SPtr<GameObject> child);
  
  /**
   * Get the GameObject's transformation
   * 
   * @return Reference to the transform
   */
  NODISCARD Transform& getTransform() { return m_transform; }
  
  /**
   * Get the GameObject's name
   * 
   * @return Name of the GameObject
   */
  NODISCARD const String& getName() const { return m_name; }
  
  /**
   * Add a component to the GameObject
   * 
   * @tparam T Component type
   * @param component Component to add
   * @return Shared pointer to the added component
   */
  template<typename T>
  SPtr<T> addComponent(SPtr<T> component) {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    m_components.push_back(component);
    component->setOwner(this);
    return component;
  }
  
  /**
   * Create and add a component to the GameObject
   * 
   * @tparam T Component type
   * @param args Arguments to pass to the component constructor
   * @return Shared pointer to the created component
   */
  template<typename T, typename... Args>
  SPtr<T> addComponent(Args&&... args) {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    SPtr<T> component = chMakeShared<T>(std::forward<Args>(args)...);
    m_components.push_back(component);
    component->setOwner(this);
    return component;
  }
  
  /**
   * Get a component of a specific type
   * 
   * @tparam T Component type
   * @return Shared pointer to the component, or nullptr if not found
   */
  template<typename T>
  NODISCARD SPtr<T> getComponent() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    for (const auto& component : m_components) {
      SPtr<T> result = std::dynamic_pointer_cast<T>(component);
      if (result) {
        return result;
      }
    }
    return nullptr;
  }
  
  /**
   * Get all components of a specific type
   * 
   * @tparam T Component type
   * @return Vector of components of the specified type
   */
  template<typename T>
  NODISCARD Vector<SPtr<T>> getComponents() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    Vector<SPtr<T>> result;
    for (const auto& component : m_components) {
      SPtr<T> typedComponent = std::dynamic_pointer_cast<T>(component);
      if (typedComponent) {
        result.push_back(typedComponent);
      }
    }
    return result;
  }
  
  /**
   * Get all components
   * 
   * @return Vector of all components
   */
  NODISCARD const Vector<SPtr<Component>>& getAllComponents() const { return m_components; }
  
  /**
   * Get all child GameObjects
   * 
   * @return Vector of child GameObjects
   */
  NODISCARD const Vector<SPtr<GameObject>>& getChildren() const { return m_children; }
  
  /**
   * Get the parent GameObject
   * 
   * @return Shared pointer to the parent, or nullptr if no parent
   */
  NODISCARD SPtr<GameObject> getParent() const { return m_parent.lock(); }
  
  /**
   * Set the parent GameObject
   * 
   * @param parent New parent GameObject
   */
  void setParent(SPtr<GameObject> parent);
  
  /**
   * Update the GameObject and all its children
   * 
   * @param deltaTime Time elapsed since the last update
   */
  void update(float deltaTime);
  
  /**
   * Set whether the GameObject is active
   * 
   * @param active Whether the GameObject should be active
   */
  void setActive(bool active) { m_active = active; }
  
  /**
   * Check if the GameObject is active
   * 
   * @return True if the GameObject is active
   */
  NODISCARD bool isActive() const { return m_active; }

 private:
  String m_name;
  Transform m_transform;
  Vector<SPtr<Component>> m_components;
  Vector<SPtr<GameObject>> m_children;
  WeakPtr<GameObject> m_parent;
  bool m_active = true;
};

} // namespace chEngineSDK