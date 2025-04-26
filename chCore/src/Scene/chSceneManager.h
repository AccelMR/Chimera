/************************************************************************/
/**
 * @file chSceneManager.h
 * @author AccelMR
 * @date 2025/04/20
 * @brief
 *  Scene Manager module for managing scene graph and game objects.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chModule.h"
#include "chScene.h"

namespace chEngineSDK {

class CH_CORE_EXPORT SceneManager : public Module<SceneManager>
{
 public:
  /**
   * Default constructor
   */
  SceneManager() = default;
  
  /**
   * Destructor
   */
  ~SceneManager() = default;

  /**
   * Create a new empty scene
   * 
   * @param name Name of the scene
   * @return Shared pointer to the created scene
   */
  SPtr<Scene> createScene(const String& name);
  
  /**
   * Load a scene from file
   * 
   * @param path Path to the scene file
   * @return Shared pointer to the loaded scene, or nullptr if loading failed
   */
  SPtr<Scene> loadScene(const Path& path);
  
  /**
   * Save a scene to file
   * 
   * @param scene Scene to save
   * @param path Path where to save the scene
   * @return True if saving was successful
   */
  bool saveScene(SPtr<Scene> scene, const Path& path);
  
  /**
   * Set the active scene
   * 
   * @param scene Scene to set as active
   */
  void setActiveScene(SPtr<Scene> scene);
  
  /**
   * Get the currently active scene
   * 
   * @return Shared pointer to the active scene
   */
  NODISCARD SPtr<Scene> getActiveScene() const { return m_activeScene; }
  
  /**
   * Get a scene by name
   * 
   * @param name Name of the scene to get
   * @return Shared pointer to the scene, or nullptr if not found
   */
  NODISCARD SPtr<Scene> getScene(const String& name) const;
  
  /**
   * Update all scenes
   * 
   * @param deltaTime Time elapsed since the last update
   */
  void update(float deltaTime);

 private:
  UnorderedMap<String, SPtr<Scene>> m_scenes;
  SPtr<Scene> m_activeScene;
};

} // namespace chEngineSDK