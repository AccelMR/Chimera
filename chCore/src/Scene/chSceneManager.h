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

#include "chLogger.h"
#include "chScene.h"
#include "chUUID.h"

CH_LOG_DECLARE_EXTERN(SceneManagerLog);

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

#if USING (CH_EDITOR)
  /**
   * Create a new empty scene
   *
   * @param name Name of the scene
   * @return Shared pointer to the created scene
   */
  WeakPtr<Scene>
  createAndLoadScene(const String& name);
#endif // USING (CH_EDITOR)

  /**
   * Load a scene from file
   *
   * @param uuid UUID of the scene asset to load
   * @return Shared pointer to the loaded scene, or nullptr if loading failed
   */
  WeakPtr<Scene>
  loadScene(WeakPtr<SceneAsset> asset);

  NODISCARD bool
  setActiveScene(WeakPtr<Scene> scene);

  /**
   * Get the currently active scene
   *
   * @return Shared pointer to the active scene
   */
  NODISCARD WeakPtr<Scene>
  getActiveScene() const {
    return m_activeScene;
  }

  /**
   * Get a scene by name
   *
   * @param UUID unique id of the scene you want
   * @return Shared pointer to the scene, or nullptr if not found
   */
  NODISCARD WeakPtr<Scene>
  getScene(const UUID& uuid) const;

  /**
   * Update all scenes
   *
   * @param deltaTime Time elapsed since the last update
   */
  void
  update(float deltaTime);

 private:
  UnorderedMap<UUID, SPtr<Scene>> m_loadedScenes;
  WeakPtr<Scene> m_activeScene;
};

} // namespace chEngineSDK
