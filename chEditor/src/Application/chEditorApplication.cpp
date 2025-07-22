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
#include "chStringUtils.h"

#include "chContentAssetUI.h"
#include "chMainMenuBarUI.h"

#if USING(CH_IMPORTERS)
#include "chAssetImporter.h"
#include "chAssetImporterManager.h"
#endif // USING(CH_IMPORTERS)

#include "imgui.h"

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
EditorApplication::~EditorApplication() {
  CH_LOG_INFO(EditorApp, "Destroying EditorApplication instance.");
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

  UIHelpers::render(graphicAPI, commandBuffer);
}

/*
 */
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  SPtr<DisplaySurface> display = getDisplaySurface();
  CH_ASSERT(display && "Display surface must not be null.");

#if USING(CH_IMPORTERS)
  AssetImporterManager::startUp();
  AssetImporterManager::instance().initialize();
  loadImporters();
#endif // USING(CH_IMPORTERS)

  AssetManager::startUp();
  AssetManager::instance().initialize();
  AssetManager::instance().lazyLoadAssetsFromDirectory(
      EnginePaths::getAbsoluteGameAssetDirectory());

  m_nastyRenderer = std::make_shared<NastyRenderer>();
  m_nastyRenderer->initialize(display->getWidth(), display->getHeight());
  m_nastyRenderer->setClearColors({UIHelpers::rendererColor});
  m_nastyRenderer->bindInputEvents();

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
  m_contentAssetUI->setNastyRenderer(m_nastyRenderer);

  m_mainMenuBar = chMakeUnique<MainMenuBarUI>();
  m_mainMenuBar->setNastyRenderer(m_nastyRenderer);

  CH_LOG_INFO(EditorApp, "Editor components initialized successfully.");
}

/*
 */
void
EditorApplication::bindEvents() {
  CH_LOG_INFO(EditorApp, "Binding editor events.");
  CH_ASSERT(EventDispatcherManager::isStarted());

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  eventDispatcher.OnKeyDown.connect([this](const KeyBoardData& data) {
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
    }
  });

  eventDispatcher.OnKeyUp.connect([this](const KeyBoardData& keyData) {
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

  eventDispatcher.OnKeyPressed.connect([this](const KeyBoardData&) {
    // Handle key pressed events specific to the editor
  });

  eventDispatcher.OnMouseButtonDown.connect([this](const MouseButtonData&) {
    // Handle mouse button down events specific to the editor
  });

  eventDispatcher.OnMouseButtonUp.connect([this](const MouseButtonData&) {
    // Handle mouse button up events specific to the editor
  });

  eventDispatcher.OnMouseMove.connect([this](const MouseMoveData&) {
    // Handle mouse move events specific to the editor
  });
  eventDispatcher.OnMouseWheel.connect([this](const MouseWheelData&) {
    // Handle mouse wheel events specific to the editor
  });
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
  graphicAPI.execute("initImGui", {display, getRenderComponents().swapChain});

  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");
  UIHelpers::bindEvnetWindowEvent(eventHandler);
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
EditorApplication::loadImporters() {
  CH_LOG_INFO(EditorApp, "Loading asset importers.");
#if USING(CH_IMPORTERS)
  //AssetImporterManager& importerManager = AssetImporterManager::instance();
  const  Path importersPath(
#if USING(CH_DEBUG_MODE)
    "build/debug-x64/lib/Importers"
#else // USING(CH_DEBUG_MODE)
    "Importers"
#endif // USING(CH_DEBUG_MODE)
  );
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(importersPath, files, directories);
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
                                                             FileSystem::absolutePath(importersPath));
      if (library.expired()) {
        CH_LOG_ERROR(EditorApp, "Failed to load importer library: {0}", file.toString());
        continue;
      }
      typedef void (*LoadPluginFunc)();
      LoadPluginFunc loadPlugin = library.lock()->getSymbol<LoadPluginFunc>("loadPlugin");
      if (!loadPlugin) {
        CH_LOG_ERROR(EditorApp, "Failed to find loadPlugin function in {0}", file.toString());
        continue;
      }
      loadPlugin(); // This should register the importer
      CH_LOG_INFO(EditorApp, "Loaded importer library: {0}", file.toString());
    }
  }
  #endif // USING(CH_IMPORTERS)
}

} // namespace chEngineSDK
