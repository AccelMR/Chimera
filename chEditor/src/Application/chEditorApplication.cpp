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
#include "chCommandParser.h"
#include "chEventDispatcherManager.h"
#include "chDynamicLibManager.h"
#include "chICommandBuffer.h"
#include "chIDescriptorPool.h"
#include "chIDescriptorSet.h"
#include "chIGraphicsAPI.h"
#include "chIRenderPass.h"
#include "chISwapChain.h"
#include "chITextureView.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPath.h"
#include "chUIHelpers.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

#include "chContentAssetUI.h"
#include "chMainMenuBarUI.h"
#include "chOutputLogUI.h"

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "imgui.h"

CH_LOG_DECLARE_STATIC(EditorApp, All);

#include "chMultiStageRenderer.h"
#include "chRenderStageIO.h"
#include "chRenderStageFactory.h"
#include "Stages/chGBufferStage.h"

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

Radian g_FOV(Degree(45.0f));
float g_farPlane = 10000.0f;
float g_nearPlane = 0.1f;
Vector3 initialCameraPos(-5.0f, 0.0f, 0.0f);

/*
 */
EditorApplication::EditorApplication() {
  CH_LOG_INFO(EditorApp, "Creating EditorApplication instance.");
}

/*
 */
EditorApplication::~EditorApplication() {
  m_outputLogUI.reset();
}

/*
 */
NODISCARD LinearColor
EditorApplication::getBackgroundColor() const {
  // Return the background color for the editor
  return UIHelpers::backgroundColor;
}

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
RendererOutput
EditorApplication::onRender(float deltaTime) {
  RendererOutput renderOut = m_nastyRenderer->onRender(deltaTime);
  //RendererOutput renderOut = m_multiStageRenderer->onRender(deltaTime);
  return renderOut;
}

/*
 */
void
EditorApplication::onPresent(const RendererOutput& rendererOutput,
                             const SPtr<ICommandBuffer>& commandBuffer, uint32 swapChainWidth,
                             uint32 swapChainHeight) {

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
  CH_PAMRAMETER_UNUSED(swapChainWidth);
  CH_PAMRAMETER_UNUSED(swapChainHeight);

  if (!UIHelpers::bRenderImGui) {
    // If ImGui rendering is disabled, skip the rendering process
    return;
  }

  UIHelpers::newFrame(graphicAPI);

  renderFullScreenRenderer(rendererOutput);
  m_mainMenuBar->renderMainMenuBar();
  m_contentAssetUI->renderContentAssetUI();
  m_outputLogUI->renderOutputLogUI();

  UIHelpers::render(graphicAPI, commandBuffer);
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
  AssetManager::instance().initialize();
  AssetManager::instance().lazyLoadAssetsFromDirectory(
      EnginePaths::getAbsoluteGameAssetDirectory());

  const String defaultSceneName = "DefaultScene";
  const String sceneName = CommandParser::instance().getParam("scene", defaultSceneName);

  SceneManager::startUp();
  SceneManager& sceneManager = SceneManager::instance();

  m_nastyRenderer = std::make_shared<NastyRenderer>();
  m_nastyRenderer->initialize(display->getWidth(), display->getHeight());
  m_nastyRenderer->setClearColors({UIHelpers::rendererColor});
  m_nastyRenderer->bindInputEvents();

  RenderStageFactory::startUp();
  RenderStageFactory::instance().registerStageType<GBufferStage>();

  m_multiStageRenderer = chMakeShared<MultiStageRenderer>();

  auto gbufferStage = RenderStageFactory::instance().createStage<GBufferStage>();
  m_gbufferStageId = m_multiStageRenderer->addStage(gbufferStage);

  m_multiStageRenderer->initialize(display->getWidth(), display->getHeight());
  m_multiStageRenderer->setClearColors({UIHelpers::rendererColor});

  setupSceneData();

  initImGui(display);

  SamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter = SamplerFilter::Linear;
  samplerInfo.minFilter = SamplerFilter::Linear;
  samplerInfo.addressModeU = SamplerAddressMode::ClampToEdge;
  samplerInfo.addressModeV = SamplerAddressMode::ClampToEdge;
  samplerInfo.addressModeW = SamplerAddressMode::ClampToEdge;

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
  m_defaultSampler = graphicAPI.createSampler(samplerInfo);

  m_contentAssetUI = chMakeUnique<ContentAssetUI>();
  m_mainMenuBar = chMakeUnique<MainMenuBarUI>();
  m_outputLogUI = chMakeUnique<OutputLogUI>();

  m_contentAssetUI->setMultiStageRenderer(m_multiStageRenderer);
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

  m_onKeyUpEvent = eventDispatcher.OnKeyUp.connect([this](const KeyBoardData& keyData) {
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
EditorApplication::initImGui(const SPtr<DisplaySurface>& display) {
  CH_LOG_INFO(EditorApp, "Initializing ImGui for the editor.");

  ImGui::CreateContext();

  UIHelpers::initStyle();
  UIHelpers::initFontConfig();

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
  graphicAPI.execute("initImGui", {ImGui::GetCurrentContext(), display, getRenderComponents().swapChain});

  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");
  m_updateInjection = UIHelpers::bindEventWindow(eventHandler);
  CH_ASSERT(m_updateInjection.isValid() && "Update injection event must be valid.");

  width = display->getWidth();
  height = display->getHeight();
}

/*
 */
void
EditorApplication::renderFullScreenRenderer(const RendererOutput& rendererOutput) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize({static_cast<float>(rendererOutput.width-50),
                            static_cast<float>(rendererOutput.height-50)},
    ImGuiCond_Always);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoFocusOnAppearing;

  if (!ImGui::Begin("Renderer Fullscreen", nullptr, window_flags)) {
    ImGui::End();
  }
  m_nastyRenderer->setFocused(ImGui::IsWindowFocused());

  if (rendererOutput.colorTarget) {
    auto it = m_textureDescriptorSets.find(rendererOutput.colorTarget);

    SPtr<IDescriptorSet> descriptorSet = nullptr;
    if (it == m_textureDescriptorSets.end()) {
      IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
      Any result = graphicAPI.execute(
          "addImGuiTexture", {Any(m_defaultSampler), Any(rendererOutput.colorTarget)});

      if (AnyUtils::tryGetValue<SPtr<IDescriptorSet>>(result, descriptorSet) &&
          descriptorSet) {
        m_textureDescriptorSets[rendererOutput.colorTarget] = descriptorSet;
      }
    }
    else {
      descriptorSet = it->second;
    }

    if (descriptorSet) {
      // La imagen ocupará toda la ventana
      ImVec2 windowSize = ImGui::GetWindowSize();
      ImGui::Image(reinterpret_cast<ImTextureID>(descriptorSet->getRaw()), windowSize);
    }
  }
  ImGui::End();
}

/*
*/
void
EditorApplication::loadCodecs() {
  CH_LOG_INFO(EditorApp, "Loading asset codecs.");
#if USING(CH_CODECS)
  //AssetCodecManager& codecManager = AssetCodecManager::instance();
  const  Path codecsPath(
#if USING(CH_DEBUG_MODE)
    "build/debug-x64/lib/Codecs"
#else // USING(CH_DEBUG_MODE)
    "Codecs"
#endif // USING(CH_DEBUG_MODE)
  );
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(codecsPath, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == ".so" ||
        file.getExtension() == ".dll") { //ugly but works for now
        //TODO:  fix this, loadDynamicLibrary adds a d at the end of the file name
        // but these dlls are not named with a d at the end
        // so we need to remove the d from the end of the file name
        String fileName = file.getFileName(false);
        fileName.pop_back();
        WeakPtr <DynamicLibrary> library =
            DynamicLibraryManager::instance().loadDynLibrary(fileName,
                                                             FileSystem::absolutePath(codecsPath));
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

/*
*/
void
EditorApplication::setupSceneData() {
    // Create basic scene data
  RenderStageIO sceneData;

  // Create a basic camera (you can customize this)
  auto cameraData = chMakeShared<CameraData>();
  auto camera = chMakeShared<Camera>(initialCameraPos,
                                     Vector3::ZERO,
                                     width,
                                     height);
  camera->setProjectionType(CameraProjectionType::Perspective);
  camera->setFieldOfView(g_FOV);
  camera->setClipPlanes(g_nearPlane, g_farPlane);
  camera->updateMatrices();
  cameraData->camera = camera;

  // Create a basic model (you can customize this)
  auto modelData = chMakeShared<ModelData>();

  sceneData.setOutput<CameraData>(cameraData);
  sceneData.setOutput<ModelData>(modelData);

  m_multiStageRenderer->setSceneData(sceneData);

  CH_LOG_INFO(EditorApp, "Scene data configured for multi-stage renderer");
}

} // namespace chEngineSDK
