/************************************************************************/
/**
 * @file chScene.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  Scene class containing a hierarchy of GameObjects.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGameObject.h"

namespace chEngineSDK {

class CH_CORE_EXPORT Scene
{
 public:
  /**
   * Constructor
   * 
   * @param name Name of the scene
   */
  Scene(const String& name);
  
  /**
   * Destructor
   */
  ~Scene() = default;
  
  /**
   * Create a new GameObject in the scene
   * 
   * @param name Name of the GameObject
   * @param parent Optional parent GameObject
   * @return Shared pointer to the created GameObject
   */
  SPtr<GameObject> 
  createGameObject(const String& name, SPtr<GameObject> parent = nullptr);
  
  /**
   * Find a GameObject by name
   * 
   * @param name Name of the GameObject to find
   * @return Shared pointer to the GameObject, or nullptr if not found
   */
  NODISCARD SPtr<GameObject> 
  findGameObject(const String& name) const;
  
  /**
   * Get all root GameObjects in the scene
   * 
   * @return Vector of root GameObjects
   */
  NODISCARD const Vector<SPtr<GameObject>>& 
  getRootGameObjects() const { return m_rootGameObjects; }
  
  /**
   * Get the name of the scene
   * 
   * @return Name of the scene
   */
  NODISCARD const String& 
  getName() const { return m_name; }
  
  /**
   * Update all GameObjects in the scene
   * 
   * @param deltaTime Time elapsed since the last update
   */
  void 
  update(float deltaTime);

 private:
  String m_name;
  Vector<SPtr<GameObject>> m_rootGameObjects;
  UnorderedMap<String, SPtr<GameObject>> m_gameObjectsMap; // Para búsqueda rápida por nombre
};

} // namespace chEngineSDK