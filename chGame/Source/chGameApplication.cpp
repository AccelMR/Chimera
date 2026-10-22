/************************************************************************/
/**
 * @file chGameApplication.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Runtime that plays a project without the editor.
 */
/************************************************************************/
#include "chGameApplication.h"

#include "chAssetManager.h"
#include "chCameraComponent.h"
#include "chConsoleVariable.h"
#include "chEnginePaths.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chProfiler.h"
#include "chScene.h"
#include "chSceneAsset.h"
#include "chSceneManager.h"
#include "chScriptManager.h"
#include "chSceneRenderer.h"
#include "chUUID.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(GameApp, All);

namespace {
ConsoleVariable<String> g_cvarStartupScene(
    "Game.StartupScene",
    "/Game/Maps/Main",
    "Scene the game opens: a virtual path such as /Game/Maps/Main, or a scene name.",
    "Scene");

// Where the view starts when the scene has no camera component.
const Vector3 kDefaultCameraPosition(-5.0f, 0.0f, 0.0f);
} // namespace

/*
 */
GameApplication::GameApplication() = default;

/*
 */
GameApplication::~GameApplication() = default;

/*
 */
void
GameApplication::onPostInitialize()
{
  AssetManager::startUp();
  AssetManager& assetManager = AssetManager::instance();
  assetManager.initialize();
  assetManager.lazyLoadAssetsFromDirectory(EnginePaths::getGameAssetDirectory());
  ScriptManager::startUp();

  SceneManager::startUp();
  m_sceneRenderer = chMakeUnique<SceneRenderer>();

  const ISwapChain& swapChain = *getSwapChain();
  m_camera = Camera(kDefaultCameraPosition, Vector3::ZERO,
                    static_cast<float>(swapChain.getWidth()),
                    static_cast<float>(swapChain.getHeight()));
  openStartupScene();
}

/*
 */
void
GameApplication::openStartupScene()
{
  const String& sceneName = g_cvarStartupScene.get();
  SceneManager& sceneManager = SceneManager::instance();
  if (const SPtr<SceneAsset> asset = AssetManager::instance().findScene(sceneName)) {
    const SPtr<Scene> scene = sceneManager.loadScene(asset).lock();
    if (scene && sceneManager.setActiveScene(scene)) {
      scene->setPlaying(true);
      m_scene = scene;
      CH_LOG_INFO(GameApp, "Playing scene {0}.", sceneName);
      return;
    }
  }

  CH_LOG_ERROR(GameApp,
               "Could not open the startup scene {0} in {1}. Set Game.StartupScene in the "
               "project's Config/Game.ini or pass -Scene=<path>. Showing an empty scene.",
               sceneName, EnginePaths::getProjectDirectory());
  m_scene = chMakeShared<Scene>("Empty", UUID::createRandom());
  m_scene->setPlaying(true);
}

/*
 */
void
GameApplication::destroyModules()
{
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }

  m_sceneRenderer.reset();
  m_scene.reset();
  if (SceneManager::isStarted()) {
    SceneManager::shutDown();
  }
  // Assets hold GPU buffers and textures, so they go while the graphics API still exists.
  if (AssetManager::isStarted()) {
    AssetManager::shutDown();
  }
  // After the assets: a scene asset owns its scene, whose script components run onDestroy
  // when it closes.
  if (ScriptManager::isStarted()) {
    ScriptManager::shutDown();
  }

  WindowedApplication::destroyModules();
}

/*
 */
void
GameApplication::fixedUpdate(const float deltaTime)
{
  SceneManager::instance().fixedUpdate(deltaTime);
}

/*
 */
void
GameApplication::update(const float deltaTime)
{
  {
    CH_PROFILE_SCOPE("SceneUpdate");
    SceneManager::instance().update(deltaTime);
  }
  {
    CH_PROFILE_SCOPE("UpdateTransforms");
    m_scene->updateTransforms();
  }

  // After the transforms, which the camera follows.
  const ISwapChain& swapChain = *getSwapChain();
  const float width = static_cast<float>(swapChain.getWidth());
  const float height = static_cast<float>(swapChain.getHeight());
  if (const CameraComponent* camera = m_scene->getActiveCamera()) {
    camera->fillCamera(m_camera, width, height);
  }
  else {
    if (!m_bWarnedNoCamera) {
      CH_LOG_WARNING(GameApp, "The scene has no camera component; using a default view.");
      m_bWarnedNoCamera = true;
    }
    m_camera.setViewportSize(width, height);
  }

  m_sceneRenderer->extract(*m_scene, m_camera, LinearColor::BLACK);
}

/*
 */
void
GameApplication::onRender(ICommandList& commandList, const ISwapChain& swapChain)
{
  RenderGraph& graph = m_sceneRenderer->beginFrame();
  const RGTextureHandle backBuffer =
      graph.importTexture("BackBuffer", swapChain.getCurrentTexture(),
                          swapChain.getCurrentTextureView(), ResourceState::Undefined,
                          ResourceState::Present);
  m_sceneRenderer->addScenePasses(backBuffer);
  m_sceneRenderer->execute(commandList);
}

} // namespace chEngineSDK
