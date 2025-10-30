/************************************************************************/
/**
 * @file chSceneManager.cpp
 * @author AccelMR
 * @date 2025/10/26
 * @brief
 *  Scene Manager module for managing scene graph and game objects.
 */
/************************************************************************/

#include "chSceneManager.h"

#include "chAssetManager.h"
#include "chSceneAsset.h"

#if USING(CH_DEBUG_MODE)
#define CH_SCENE_LOG_LEVEL All
#else
#define CH_SCENE_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)

CH_LOG_DEFINE_CATEGORY_SHARED(SceneManagerLog, CH_SCENE_LOG_LEVEL);

namespace chEngineSDK {

#if USING (CH_EDITOR)
/*
*/
WeakPtr<Scene>
SceneManager::createAndLoadScene(const String& name) {
  UUID newID = UUID::createRandom();
  if (m_loadedScenes.contains(newID)){
    CH_LOG(SceneManagerLog,
           Error,
           "Scene with UUID {0} already exists. Cannot create scene '{1}'.",
             newID.toString(),
             name);
    return WeakPtr<Scene>{};
  }

  SPtr<Scene> newScene = chMakeShared<Scene>(name, newID);
  m_loadedScenes[newID] = newScene;
  CH_LOG(SceneManagerLog,
         Info,
         "Created new scene '{0}' with UUID {1}.",
         name,
         newID.toString());
  return WeakPtr<Scene>(newScene);
}
#endif // USING (CH_EDITOR)

/*
*/
WeakPtr<Scene>
SceneManager::loadScene(WeakPtr<SceneAsset> asset){
  AssetManager& assetManager = AssetManager::instance();

  if (asset.expired()) {
    CH_LOG_ERROR(SceneManagerLog,
                 "Cannot load scene from null asset.");
    return WeakPtr<Scene>{};
  }

  SPtr<SceneAsset> sceneAsset = asset.lock();
  if (!assetManager.loadAsset(sceneAsset)) {
    CH_LOG_ERROR(SceneManagerLog,
                 "Failed to load scene asset with UUID {0}.",
                 sceneAsset->getUUID().toString());
    return WeakPtr<Scene>{};
  }

  SPtr<Scene> scene = sceneAsset->getScene().lock();
  if (!scene) {
    CH_LOG_ERROR(SceneManagerLog,
                  "Scene asset with UUID {0} does not contain a valid scene.",
                  sceneAsset->getUUID().toString());
    return WeakPtr<Scene>{};
  }
  m_loadedScenes[sceneAsset->getUUID()] = scene;
  CH_LOG_INFO(SceneManagerLog,
               "Loaded scene '{0}' with UUID {1} from asset.",
               scene->getName(),
               sceneAsset->getUUID().toString());

  return scene;
}

/*
*/
bool
SceneManager::setActiveScene(WeakPtr<Scene> scene){
  if (scene.expired()) {
    CH_LOG_ERROR(SceneManagerLog,
                  "Cannot set active scene to null.");
    return false;
  }

  //Verify that the scene is in the loaded scenes
  SPtr<Scene> scenePtr = scene.lock();
  auto it = m_loadedScenes.find(scenePtr->getUUID());
  if (it == m_loadedScenes.end()) {
    CH_LOG_ERROR(SceneManagerLog,
                  "Scene with UUID {0} is not loaded. Cannot set as active scene.",
                  scenePtr->getUUID().toString());
    return false;
  }

  CH_LOG_INFO(SceneManagerLog,
               "Set active scene to '{0}' with UUID {1}.",
               scenePtr->getName(),
               scenePtr->getUUID().toString());

  m_activeScene = scene;
  return true;
}

/*
*/
void
SceneManager::update(float) {
}

} // namespace chEngineSDK
