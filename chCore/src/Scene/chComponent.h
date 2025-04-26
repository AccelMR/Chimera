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

namespace chEngineSDK {

class GameObject;

class CH_CORE_EXPORT Component
{
 public:
  /**
   * Default constructor
   */
  Component() = default;
  
  /**
   * Virtual destructor
   */
  virtual ~Component() = default;
  
  /**
   * Initialize the component
   * 
   * @return True if initialization was successful
   */
  virtual bool initialize() { return true; }
  
  /**
   * Update the component
   * 
   * @param deltaTime Time elapsed since the last update
   */
  virtual void update(float deltaTime) {}
  
  /**
   * Set the owner of this component
   * 
   * @param owner Owner GameObject
   */
  void setOwner(GameObject* owner) { m_owner = owner; }
  
  /**
   * Get the owner of this component
   * 
   * @return Owner GameObject
   */
  NODISCARD GameObject* getOwner() const { return m_owner; }
  
  /**
   * Enable/disable the component
   * 
   * @param enabled Whether the component should be enabled
   */
  void setEnabled(bool enabled) { m_enabled = enabled; }
  
  /**
   * Check if the component is enabled
   * 
   * @return True if the component is enabled
   */
  NODISCARD bool isEnabled() const { return m_enabled; }

 private:
  GameObject* m_owner = nullptr;
  bool m_enabled = true;
};

} // namespace chEngineSDK