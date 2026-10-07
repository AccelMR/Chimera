/************************************************************************/
/**
 * @file chComponent.h
 * @author AccelMR
 * @date 2025/04/25
 * @brief
 *  Base component class that can be attached to GameObjects.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {

/**
 * Exists so a GameObject gets its behaviour from parts it can mix, instead of from a class
 * hierarchy. A component is registered while its GameObject is in a scene and it is enabled;
 * that is when it may add things to the scene (a ModelComponent adds what the renderer
 * draws). Components are made with GameObject::addComponent, which owns them.
 */
class CH_CORE_EXPORT Component
{
 public:
  Component() = default;

  virtual ~Component() = default;

  Component(const Component&) = delete;

  Component&
  operator=(const Component&) = delete;

  virtual void
  update(float deltaTime);

  /**
   * A disabled component is unregistered: it stays on its GameObject but leaves the scene.
   */
  void
  setEnabled(bool enabled);

  NODISCARD FORCEINLINE bool
  isEnabled() const { return m_enabled; }

  NODISCARD FORCEINLINE bool
  isRegistered() const { return m_registered; }

  NODISCARD FORCEINLINE GameObject*
  getOwner() const { return m_owner; }

  /**
   * TypeTraits<T>::getTypeId() of the class it was created as.
   */
  NODISCARD FORCEINLINE const UUID&
  getTypeId() const { return m_typeId; }

 protected:
  virtual void
  onRegister() {}

  virtual void
  onUnregister() {}

  /**
   * Called after the scene computed a new world matrix for the owner, while registered.
   */
  virtual void
  onTransformChanged() {}

 private:
  friend class GameObject;

  void
  setRegistered(bool registered);

  GameObject* m_owner = nullptr;
  // Kept by value: the UUID TypeTraits returns lives in the module that asked for it.
  UUID m_typeId = UUID::null();
  bool m_enabled = true;
  bool m_registered = false;
};

} // namespace chEngineSDK
