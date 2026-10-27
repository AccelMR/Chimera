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

CH_LOG_DECLARE_EXTERN(CH_CORE_EXPORT, SceneManagerLog);

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

  /**
   * Keeps a scene made elsewhere (the copy Play in the editor runs), so it can be made
   * active. False when a scene with its UUID is already kept.
   */
  NODISCARD bool
  addScene(const SPtr<Scene>& scene);

  /**
   * Forgets the scene; it is destroyed once nothing else holds it. Unloading the active
   * scene leaves no active scene.
   */
  void
  unloadScene(const Scene& scene);

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
   * Runs the fixed step of the active scene.
   */
  void
  fixedUpdate(float deltaTime);

  /**
   * Runs the per frame update of the active scene, then applies the level requests made
   * meanwhile.
   */
  void
  update(float deltaTime);

  /**
   * Makes a new scene holding the objects of asset as its map, plus the sublevels its list
   * loads at start, and makes it the active one; the scene it replaces is forgotten. A scene
   * asset already loaded (open in the editor) is copied and stays loaded; otherwise it is
   * unloaded once its objects moved. Never during an update. Null when asset cannot load.
   */
  WeakPtr<Scene>
  openMap(const SPtr<SceneAsset>& asset, bool bPlaying);

  /**
   * Adds the objects of asset to scene as a new level, after the others. Never during an
   * update of scene. False when it cannot load or is already a level of scene.
   */
  bool
  loadLevel(Scene& scene, const SPtr<SceneAsset>& asset);

  /**
   * Destroys the objects of the level asset loaded. Never during an update of scene. False
   * when it is not a sublevel of scene.
   */
  bool
  unloadLevel(Scene& scene, const UUID& asset);

  /**
   * Loads the sublevels the map of scene lists to load at start.
   */
  void
  loadStartLevels(Scene& scene);

  /**
   * Replaces the active scene with asset's map when the running update ends; the new one
   * plays when the old one did.
   */
  void
  requestOpenMap(const SPtr<SceneAsset>& asset);

  /**
   * Loads asset as a level of the active scene when the running update ends.
   */
  void
  requestLoadLevel(const SPtr<SceneAsset>& asset);

  void
  requestUnloadLevel(const UUID& asset);

  /**
   * True while a load of asset waits for the end of the update.
   */
  NODISCARD bool
  isLevelLoadRequested(const UUID& asset) const;

 private:
  enum class LevelRequestType
  {
    OpenMap,
    Load,
    Unload
  };

  struct LevelRequest
  {
    LevelRequestType type = LevelRequestType::Load;
    UUID asset = UUID::null();
  };

  void
  applyLevelRequests();

  /**
   * Puts the objects of asset into scene as objects of level. outSubLevels, when given, gets
   * the asset's list of sublevels.
   */
  NODISCARD bool
  readLevelObjects(const SPtr<SceneAsset>& asset,
                   Scene& scene,
                   SceneLevelId level,
                   Vector<SceneSubLevel>* outSubLevels);

  UnorderedMap<UUID, SPtr<Scene>> m_loadedScenes;
  WeakPtr<Scene> m_activeScene;
  Vector<LevelRequest> m_levelRequests;
  // The requests being applied; a member so its memory is kept.
  Vector<LevelRequest> m_levelRequestBatch;
};

} // namespace chEngineSDK
