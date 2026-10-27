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
#include "chFileStream.h"
#include "chGameObjectSerializer.h"
#include "chSceneAsset.h"
#include "chScriptManager.h"

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
  if (!assetManager.syncLoadAsset(sceneAsset)) {
    CH_LOG_ERROR(SceneManagerLog,
                 "Failed to load scene asset with UUID {0}.",
                 sceneAsset->getUUID().toString());
    return WeakPtr<Scene>{};
  }

  const SPtr<Scene>& scene = sceneAsset->getScene();
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
SceneManager::addScene(const SPtr<Scene>& scene)
{
  if (!scene || !m_loadedScenes.try_emplace(scene->getUUID(), scene).second) {
    CH_LOG_ERROR(SceneManagerLog, "Cannot keep the scene: it is null or its UUID is taken.");
    return false;
  }
  return true;
}

/*
 */
void
SceneManager::unloadScene(const Scene& scene)
{
  if (m_activeScene.lock().get() == &scene) {
    m_activeScene.reset();
    m_levelRequests.clear();
  }
  m_loadedScenes.erase(scene.getUUID());
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
SceneManager::fixedUpdate(float deltaTime)
{
  if (SPtr<Scene> scene = m_activeScene.lock()) {
    if (scene->isPlaying() && ScriptManager::isStarted()) {
      ScriptManager::instance().beginFixedUpdate(deltaTime);
    }
    scene->fixedUpdate(deltaTime);
  }
}

/*
 */
void
SceneManager::update(float deltaTime)
{
  if (SPtr<Scene> scene = m_activeScene.lock()) {
    if (scene->isPlaying() && ScriptManager::isStarted()) {
      ScriptManager::instance().beginUpdate(deltaTime);
    }
    scene->update(deltaTime);
  }
  applyLevelRequests();
}

/*
 */
bool
SceneManager::readLevelObjects(const SPtr<SceneAsset>& asset,
                               Scene& scene,
                               const SceneLevelId level,
                               Vector<SceneSubLevel>* outSubLevels)
{
  if (asset->isLoaded()) {
    // Someone else uses its scene (the editor has it open), so the objects are copied.
    const SPtr<Scene>& source = asset->getScene();
    if (!source) {
      return false;
    }
    const SIZE_T firstNew = scene.getRootGameObjects().size();
    MemoryDataStream written;
    GameObjectSerializer::write(written, source->getRootGameObjects());
    MemoryDataStream reader(written.getStartPtr(), written.size(), false);
    const bool bRead = GameObjectSerializer::read(reader, scene, nullptr);
    // By index: the list does not change while the new roots get their level.
    const Vector<SPtr<GameObject>>& roots = scene.getRootGameObjects();
    for (SIZE_T i = firstNew; i < roots.size(); ++i) {
      scene.moveToLevel(*roots[i], level);
    }
    if (outSubLevels) {
      *outSubLevels = source->getSubLevels();
    }
    return bRead;
  }

  AssetManager& assetManager = AssetManager::instance();
  if (!assetManager.syncLoadAsset(asset) || !asset->getScene()) {
    CH_LOG_ERROR(SceneManagerLog, "Could not load the scene {0}.", asset->getName());
    return false;
  }
  Scene& source = *asset->getScene();
  if (outSubLevels) {
    *outSubLevels = source.getSubLevels();
  }
  scene.moveRootObjectsFrom(source, level);
  // Its objects live in scene now; the asset's own scene is empty.
  assetManager.unloadAsset(asset);
  return true;
}

/*
 */
WeakPtr<Scene>
SceneManager::openMap(const SPtr<SceneAsset>& asset, const bool bPlaying)
{
  if (!asset) {
    return {};
  }
  // A random id: the scene asset may keep a scene of its own with the asset's id.
  SPtr<Scene> scene = chMakeShared<Scene>(asset->getName(), UUID::createRandom());
  scene->setMapLevel(asset->getName(), asset->getUUID());
  Vector<SceneSubLevel> subLevels;
  if (!readLevelObjects(asset, *scene, Scene::MAP_LEVEL, &subLevels)) {
    return {};
  }
  scene->setSubLevels(subLevels);
  scene->setPlaying(bPlaying);

  if (SPtr<Scene> previous = m_activeScene.lock()) {
    unloadScene(*previous);
  }
  m_loadedScenes[scene->getUUID()] = scene;
  m_activeScene = scene;
  loadStartLevels(*scene);
  CH_LOG_INFO(SceneManagerLog, "Opened the map {0}.", asset->getName());
  return scene;
}

/*
 */
bool
SceneManager::loadLevel(Scene& scene, const SPtr<SceneAsset>& asset)
{
  if (!asset) {
    return false;
  }
  if (scene.findLevel(asset->getUUID())) {
    CH_LOG_WARNING(SceneManagerLog, "The level {0} is already loaded in {1}.",
                   asset->getName(), scene.getName());
    return false;
  }
  const SceneLevelId level = scene.addLevel(asset->getName(), asset->getUUID());
  if (!readLevelObjects(asset, scene, level, nullptr)) {
    scene.removeLevel(level);
    return false;
  }
  CH_LOG_INFO(SceneManagerLog, "Loaded the level {0}.", asset->getName());
  return true;
}

/*
 */
bool
SceneManager::unloadLevel(Scene& scene, const UUID& asset)
{
  const SceneLevel* level = scene.findLevel(asset);
  if (!level || level->id == Scene::MAP_LEVEL) {
    return false;
  }
  // Copied: removeLevel erases the entry level points at.
  const String name = level->name;
  if (!scene.removeLevel(level->id)) {
    return false;
  }
  CH_LOG_INFO(SceneManagerLog, "Unloaded the level {0}.", name);
  return true;
}

/*
 */
void
SceneManager::loadStartLevels(Scene& scene)
{
  if (!AssetManager::isStarted()) {
    return;
  }
  // Copied: a level that fails to load must not change the list being walked.
  const Vector<SceneSubLevel> subLevels = scene.getSubLevels();
  for (const SceneSubLevel& subLevel : subLevels) {
    if (!subLevel.bLoadAtStart) {
      continue;
    }
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(subLevel.asset);
    const SPtr<SceneAsset> sceneAsset = asset ? asset->as<SceneAsset>() : nullptr;
    if (!sceneAsset) {
      CH_LOG_WARNING(SceneManagerLog, "The map {0} lists a sublevel {1} that does not exist.",
                     scene.getName(), subLevel.asset);
      continue;
    }
    loadLevel(scene, sceneAsset);
  }
}

/*
 */
void
SceneManager::requestOpenMap(const SPtr<SceneAsset>& asset)
{
  if (asset) {
    m_levelRequests.push_back({.type = LevelRequestType::OpenMap, .asset = asset->getUUID()});
  }
}

/*
 */
void
SceneManager::requestLoadLevel(const SPtr<SceneAsset>& asset)
{
  if (asset) {
    m_levelRequests.push_back({.type = LevelRequestType::Load, .asset = asset->getUUID()});
  }
}

/*
 */
void
SceneManager::requestUnloadLevel(const UUID& asset)
{
  m_levelRequests.push_back({.type = LevelRequestType::Unload, .asset = asset});
}

/*
 */
bool
SceneManager::isLevelLoadRequested(const UUID& asset) const
{
  for (const LevelRequest& request : m_levelRequests) {
    if (request.type == LevelRequestType::Load && request.asset == asset) {
      return true;
    }
  }
  return false;
}

/*
 */
void
SceneManager::applyLevelRequests()
{
  if (m_levelRequests.empty()) {
    return;
  }
  // Scripts of a level loaded or unloaded here may ask for more (onDestroy); those wait for
  // the next update.
  m_levelRequestBatch.swap(m_levelRequests);
  for (const LevelRequest& request : m_levelRequestBatch) {
    SPtr<Scene> scene = m_activeScene.lock();
    if (!scene) {
      break;
    }
    if (request.type == LevelRequestType::Unload) {
      unloadLevel(*scene, request.asset);
      continue;
    }

    const SPtr<IAsset> asset =
        AssetManager::isStarted() ? AssetManager::instance().getAsset(request.asset) : nullptr;
    const SPtr<SceneAsset> sceneAsset = asset ? asset->as<SceneAsset>() : nullptr;
    if (!sceneAsset) {
      CH_LOG_ERROR(SceneManagerLog, "No scene {0} to load.", request.asset);
      continue;
    }
    if (request.type == LevelRequestType::Load) {
      loadLevel(*scene, sceneAsset);
      continue;
    }

    // What came after the map change was meant for the scene it replaced.
    openMap(sceneAsset, scene->isPlaying());
    m_levelRequests.clear();
    break;
  }
  m_levelRequestBatch.clear();
}

} // namespace chEngineSDK
