/************************************************************************/
/**
 * @file chEditorApplication.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @details
 *  Chimera Editor application class.
 */
/************************************************************************/

#include "chEditorApplication.h"
#include "chAssetManager.h"
#include "chCommandLine.h"
#include "chEventDispatcherManager.h"
#include "chDynamicLibManager.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPath.h"
#include "chUIHelpers.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

#include "chContentAssetUI.h"
#include "chEditorSelection.h"
#include "chMainMenuBarUI.h"
#include "chOutputLogUI.h"
#include "chSceneGraphUI.h"
#include "chInspectorUI.h"
#include "chGameObjAssetUI.h"

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)
#include "imgui_impl_sdl3.h"
#endif // USING(CH_DISPLAY_SDL3)

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;


/*
 */
EditorApplication::EditorApplication() {
  CH_LOG_INFO(EditorApp, "Creating EditorApplication instance.");
}

/*
 */
EditorApplication::~EditorApplication() {}

/*
 */
void
EditorApplication::onPostInitialize() {
  CH_LOG_INFO(EditorApp, "Post-initialization of EditorApplication.");

  initializeEditorComponents();
  bindEvents();
}

/*
 */
void
EditorApplication::destroyModules()
{
  CH_LOG_INFO(EditorApp, "EditorApplication destroying modules.");
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }

  m_onKeyDownEvent.disconnect();
  m_onKeyUpEvent.disconnect();
  m_updateInjection.disconnect();

  // The renderer gives its textures back to the ImGui context, and the SDL backend uses the
  // window, which the parent destroys.
  m_imguiRenderer.reset();
  if (ImGui::GetCurrentContext()) {
#if USING(CH_DISPLAY_SDL3)
    if (ImGui::GetIO().BackendPlatformUserData) {
      ImGui_ImplSDL3_Shutdown();
    }
#endif // USING(CH_DISPLAY_SDL3)
    ImGui::DestroyContext();
  }

  m_gameObjectAssetUI.reset();
  m_inspectorUI.reset();
  m_sceneGraphUI.reset();
  m_outputLogUI.reset();
  m_mainMenuBar.reset();
  m_contentAssetUI.reset();

  m_nastyRenderer.reset();
  m_activeScene.reset();
  EditorSelection::setSelectedGameObject(nullptr);
  EditorSelection::setGameObjectAssetPreview(nullptr);

  if (SceneManager::isStarted()) {
    SceneManager::shutDown();
  }
  // Assets hold GPU buffers and textures, so they go while the graphics API still exists.
  if (AssetManager::isStarted()) {
    AssetManager::shutDown();
  }
#if USING(CH_CODECS)
  // The codec objects live in the codec libraries, which stay loaded until the parent
  // shuts down the library manager.
  if (AssetCodecManager::isStarted()) {
    AssetCodecManager::shutDown();
  }
#endif // USING(CH_CODECS)

  WindowedApplication::destroyModules();
}

/*
 */
void
EditorApplication::onRender(ICommandList& commandList,
                            const ISwapChain& swapChain,
                            float deltaTime)
{
  const uint32 width = swapChain.getWidth();
  const uint32 height = swapChain.getHeight();
  if (width != m_nastyRenderer->getWidth() || height != m_nastyRenderer->getHeight()) {
    m_nastyRenderer->resize(width, height);
  }

  if (UIHelpers::bRenderImGui) {
    renderUI();
  }

  // The scene draws under the UI, so it only takes the keyboard and mouse that no ImGui
  // window wants.
  const ImGuiIO& io = ImGui::GetIO();
  m_nastyRenderer->setFocused(!UIHelpers::bRenderImGui ||
                              (!io.WantCaptureMouse && !io.WantCaptureKeyboard));

  const ITextureView& backBuffer = swapChain.getCurrentTextureView();
  m_nastyRenderer->onRender(commandList, backBuffer, deltaTime);

  if (UIHelpers::bRenderImGui) {
    m_imguiRenderer->render(commandList, *ImGui::GetDrawData(), backBuffer,
                            swapChain.getFormat(), width, height);
    m_imguiRenderer->renderFloatingWindows(commandList);
  }
}

/*
 */
void
EditorApplication::onPostPresent()
{
  m_imguiRenderer->presentFloatingWindows();
}

/*
 */
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  SPtr<DisplaySurface> display = getDisplaySurface();
  CH_ASSERT(display && "Display surface must not be null.");

#if USING(CH_CODECS)
  AssetCodecManager::startUp();
  AssetCodecManager::instance().initialize();
  loadCodecs();
#endif // USING(CH_CODECS)

  AssetManager::startUp();
  AssetManager& assetManager = AssetManager::instance();
  assetManager.initialize();
  assetManager.lazyLoadAssetsFromDirectory(EnginePaths::getGameAssetDirectory());

  const String sceneName = CommandLine::getValue("scene", "DefaultScene");
  SceneManager::startUp();
  SceneManager& sceneManager = SceneManager::instance();

  WeakPtr<SceneAsset> sceneAsset = assetManager.loadSceneByName(sceneName);
  SPtr<Scene> scene = nullptr;
  if (!sceneAsset.expired()) {
    scene = sceneManager.loadScene(sceneAsset).lock();
  }
  if (scene == nullptr) {
    CH_LOG_WARNING(EditorApp,
                   "Failed to load scene '{0}'. Creating a new empty scene.",
                   sceneName);
    scene = sceneManager.createAndLoadScene(sceneName).lock();
  }

  const bool bSceneLoaded = sceneManager.setActiveScene(scene);
  if (!bSceneLoaded) {
    CH_LOG_FATAL(EditorApp,
                 "Failed to set scene '{0}' as the active scene.",
                 sceneName);
    return;
  }
  CH_LOG_INFO(EditorApp, "Loaded scene '{0}' successfully.", sceneName);

  m_activeScene = scene;
  m_nastyRenderer = std::make_shared<NastyRenderer>();
  // The scene is drawn straight into the swap chain image.
  const ISwapChain& swapChain = *getSwapChain();
  m_nastyRenderer->initialize(swapChain.getWidth(), swapChain.getHeight(),
                              swapChain.getFormat());
  m_nastyRenderer->setClearColors({UIHelpers::rendererColor});
  m_nastyRenderer->bindInputEvents();

  initImGui(display);

  m_contentAssetUI = chMakeUnique<ContentAssetUI>();
  m_mainMenuBar = chMakeUnique<MainMenuBarUI>();
  m_outputLogUI = chMakeUnique<OutputLogUI>();
  m_sceneGraphUI = chMakeUnique<SceneGraphUI>();
  m_inspectorUI = chMakeUnique<InspectorUI>();
  m_gameObjectAssetUI = chMakeUnique<GameObjectAssetUI>();

  m_contentAssetUI->setNastyRenderer(m_nastyRenderer);
  m_outputLogUI->updateAvailableCategories();

  CH_LOG_INFO(EditorApp, "Editor components initialized successfully.");
}

/*
 */
void
EditorApplication::bindEvents() {
  CH_LOG_INFO(EditorApp, "Binding editor events.");
  CH_ASSERT(EventDispatcherManager::isStarted());

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  m_onKeyDownEvent = eventDispatcher.OnKeyDown.connect([this](const KeyBoardData& data) {
    // Handle key down events specific to the editor
    if (data.hasModifier(KeyBoardModifier::LCTRL) ||
        data.hasModifier(KeyBoardModifier::RCTRL)) {
      if (data.key == chKeyBoard::Key::S) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+S pressed, saving the current document.");
        // Handle Ctrl+S for saving the current document
      }
      else if (data.key == chKeyBoard::Key::O) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+O pressed, opening a document.");
        // Handle Ctrl+O for opening a document
      }

      if (data.key == chKeyBoard::Key::F5) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+F5 pressed, reloading the current document.");
        m_outputLogUI->updateAvailableCategories();
      }
    }
  });

  m_onKeyUpEvent = eventDispatcher.OnKeyUp.connect([](const KeyBoardData& keyData) {
    switch (keyData.key) {
    case chKeyBoard::Key::F10: {
      CH_LOG_DEBUG(EditorApp, "F10 pressed, toggling ImGui rendering.");
      UIHelpers::bRenderImGui = !UIHelpers::bRenderImGui;
      if (UIHelpers::bRenderImGui) {
        CH_LOG_DEBUG(EditorApp, "ImGui rendering enabled.");
      }
      else {
        CH_LOG_DEBUG(EditorApp, "ImGui rendering disabled.");
      }
    } break;

    default:
      break;
    }
  });

  // eventDispatcher.OnKeyPressed.connect([this](const KeyBoardData&) {
  //   // Handle key pressed events specific to the editor
  // });

  // eventDispatcher.OnMouseButtonDown.connect([this](const MouseButtonData&) {
  //   // Handle mouse button down events specific to the editor
  // });

  // eventDispatcher.OnMouseButtonUp.connect([this](const MouseButtonData&) {
  //   // Handle mouse button up events specific to the editor
  // });

  // eventDispatcher.OnMouseMove.connect([this](const MouseMoveData&) {
  //   // Handle mouse move events specific to the editor
  // });
  // eventDispatcher.OnMouseWheel.connect([this](const MouseWheelData&) {
  //   // Handle mouse wheel events specific to the editor
  // });
}

/*
 */
void
EditorApplication::initImGui(const SPtr<DisplaySurface>& display)
{
  CH_LOG_INFO(EditorApp, "Initializing ImGui for the editor.");

  ImGui::CreateContext();

  // ImGui keeps the pointer, not a copy, so the string must outlive the context.
  static const String iniFilePath =
      FileSystem::absolutePath(EnginePaths::getConfigDirectory().join(Path("imgui.ini")))
          .toString();
  ImGui::GetIO().IniFilename = iniFilePath.c_str();

  UIHelpers::initStyle();
  UIHelpers::initFontConfig();

#if USING(CH_DISPLAY_SDL3)
  // Windows ImGui opens get SDL_WINDOW_VULKAN, which Vulkan needs to make a surface on them.
  ImGui_ImplSDL3_InitForVulkan(display->getPlatformHandler());
#endif // USING(CH_DISPLAY_SDL3)
  m_imguiRenderer = chMakeUnique<ImGuiRenderer>();

  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");
  m_updateInjection = UIHelpers::bindEventWindow(eventHandler);
  CH_ASSERT(m_updateInjection.isValid() && "Update injection event must be valid.");
}

/*
 */
void
EditorApplication::renderUI()
{
  UIHelpers::newFrame();

  m_mainMenuBar->renderMainMenuBar();
  m_contentAssetUI->renderContentAssetUI();
  m_outputLogUI->renderOutputLogUI();
  m_sceneGraphUI->renderSceneGraphUI();
  m_inspectorUI->renderInspectorUI();
  m_gameObjectAssetUI->renderGameObjectAssetUI();

  ImGui::Render();
  // Opens, closes and resizes the windows outside the main one before they are drawn.
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
  }
}

/*
*/
void
EditorApplication::loadCodecs() {
  CH_LOG_INFO(EditorApp, "Loading asset codecs.");
#if USING(CH_CODECS)
  //AssetCodecManager& codecManager = AssetCodecManager::instance();
  const Path& codecsPath = EnginePaths::getCodecDirectory();
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(codecsPath, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == DynamicLibrary::EXTENSION) {
      WeakPtr<DynamicLibrary> library =
          DynamicLibraryManager::instance().loadDynLibrary(file.getFileName(), codecsPath);
      if (library.expired()) {
        CH_LOG_ERROR(EditorApp, "Failed to load codec library: {0}", file.toString());
        continue;
      }
      typedef void (*LoadPluginFunc)();
      LoadPluginFunc loadPlugin = library.lock()->getSymbol<LoadPluginFunc>("loadPlugin");
      if (!loadPlugin) {
        CH_LOG_ERROR(EditorApp, "Failed to find loadPlugin function in {0}", file.toString());
        continue;
      }
      loadPlugin(); // This should register the codec
      CH_LOG_INFO(EditorApp, "Loaded codec library: {0}", file.toString());
    }
  }
  #endif // USING(CH_CODECS)
}

} // namespace chEngineSDK
