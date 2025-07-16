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

#include "chAssetImporter.h"
#include "chAssetManager.h"
#include "chAssetManagerImporter.h"
#include "chEventDispatcherManager.h"
#include "chICommandBuffer.h"
#include "chIDescriptorPool.h"
#include "chIDescriptorSet.h"
#include "chIGraphicsAPI.h"
#include "chIRenderPass.h"
#include "chISwapChain.h"
#include "chITextureView.h"
#include "chLogger.h"
#include "chMeshImporter.h"
#include "chModelAsset.h"
#include "chPath.h"

//  TODO: probably move this to its own classs
#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#endif // USING(CH_DISPLAY_SDL3)

#include <nfd.hpp>

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {

namespace ImguiVars {
static bool bShowDemoWindow =
    false;                       // Variable to control the visibility of the ImGui demo window
static bool bRenderImGui = true; // Variable to control if ImGui should render
static bool bShowContentWindow =
    false; // Variable to control the visibility of the file explorer
} // namespace ImguiVars

namespace RenderVars {
static LinearColor backgroundColor =
    LinearColor::Pink; // Default background color for the editor
static LinearColor rendererColor = LinearColor::Black; // Default renderer color
} // namespace RenderVars

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
  return RenderVars::backgroundColor;
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

  if (!ImguiVars::bRenderImGui) {
    // If ImGui rendering is disabled, skip the rendering process
    return;
  }

  graphicAPI.execute("newFrameImGui");

#if USING(CH_DISPLAY_SDL3)
  ImGui_ImplSDL3_NewFrame();
#endif // USING(CH_DISPLAY_SDL3)

  ImGui::NewFrame();

  renderFullScreenRenderer(rendererOutput);

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Render")) {
      ImGui::ColorEdit4("Background Color", RenderVars::backgroundColor.toFloatPtr(),
                        ImGuiColorEditFlags_NoInputs);

      ImGui::Separator(); //--------------------------------------------------------------
      if (ImGui::ColorEdit4("Renderer Color", RenderVars::rendererColor.toFloatPtr(),
                            ImGuiColorEditFlags_NoInputs)) {
        m_nastyRenderer->setClearColors({RenderVars::rendererColor});
      }

      ImGui::Separator(); //--------------------------------------------------------------

      ImGui::MenuItem("Show ImGui Demo Window", nullptr, &ImguiVars::bShowDemoWindow);

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Asset")) {
      if (ImGui::MenuItem("Import Model")) {
        auto importer = AssetManagerImporter::instance().getImporter<MeshImpotrter>();
        CH_ASSERT(importer && "MeshImpotrter importer must not be null.");

        Vector<String> supportedExtensions = importer->getSupportedExtensions();
        openFileExplorer(EnginePaths::getAbsoluteAssetFolder(), supportedExtensions);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  renderAssetContent();

  ImGui::Render();
  ImGuiIO& io = ImGui::GetIO();
  // Update and Render additional Platform Windows
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  graphicAPI.execute("renderImGui", {commandBuffer});
}

/*
 */
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  SPtr<DisplaySurface> display = getDisplaySurface();
  CH_ASSERT(display && "Display surface must not be null.");

  AssetManagerImporter::startUp();
  AssetManagerImporter::instance().initialize();

  AssetManager::startUp();
  AssetManager::instance().initialize();
  AssetManager::instance().lazyLoadAssetsFromDirectory(EnginePaths::getAbsoluteAssetFolder());
  m_assets = AssetManager::instance().getAllAssets();

  m_nastyRenderer = std::make_shared<NastyRenderer>();
  m_nastyRenderer->initialize(display->getWidth(), display->getHeight());
  m_nastyRenderer->setClearColors({RenderVars::rendererColor});
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
      ImguiVars::bRenderImGui = !ImguiVars::bRenderImGui;
      if (ImguiVars::bRenderImGui) {
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
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
  io.ConfigFlags |=
      ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  const float mainScale =
#if USING(CH_DISPLAY_SDL3)
      SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
#else
      1.0f;
  // Fallback to a default scale if SDL3 is not used1.0f;
#endif // USING(CH_DISPLAY_SDL3)

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();

  // Bake a fixed style scale. (until we have a solution for dynamic style
  // scaling, changing this requires resetting Style + calling this again)
  style.ScaleAllSizes(mainScale);

  // unnecessary. We leave both here for documentation purpose)
  // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
  style.FontScaleDpi = mainScale;

  // [Experimental] Automatically overwrite style.FontScaleDpi
  // in Begin() when Monitor DPI changes. This will scale fonts
  // but _NOT_ scale sizes/padding for now.
  io.ConfigDpiScaleFonts = true;

  // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
  io.ConfigDpiScaleViewports = true;

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
  // identical to regular ones.
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
  graphicAPI.execute("initImGui", {display, getRenderComponents().swapChain});

  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");

  eventHandler->addUpdateInjection([this](const Vector<Any>& args) -> bool {
#if USING(CH_DISPLAY_SDL3)
    CH_ASSERT(args.size() == 1 && "Expected exactly one argument of type SDL_Event.");
    if (args.empty()) {
      CH_LOG_ERROR(EditorApp, "No arguments passed to display event handler.");
      return false;
    }

    SDL_Event event;
    if (!AnyUtils::tryGetValue<SDL_Event>(args[0], event)) {
      CH_LOG_ERROR(EditorApp, "Invalid argument type passed to display event handler.");
      return false;
    }

    // Process SDL3 events with ImGui
    return ImGui_ImplSDL3_ProcessEvent(&event);
#else
    CH_PAMRAMETER_UNUSED(args);
    CH_LOG_ERROR(EditorApp, "SDL3 is not enabled. Cannot process SDL_Event.");
    return false;
#endif // USING(CH_DISPLAY_SDL3)
  });
}

/*
 */
void
EditorApplication::openFileExplorer(const Path& pathToOpen, const Vector<String>& filters) {
  NFD::Guard nfdGuard;
  NFD::UniquePath outPath;

  String supportedExtensionsStr;
  for (const String& ext : filters) {
    String extNoAst = ext.substr(2, ext.size() - 2); // Remove the leading dot
    if (!supportedExtensionsStr.empty()) {
      supportedExtensionsStr += ",";
    }
    supportedExtensionsStr += extNoAst;
  }

  const Vector<nfdfilteritem_t> nfdFilter = {
      {"Supported Files", supportedExtensionsStr.c_str()}};

  nfdresult_t result = NFD::OpenDialog(outPath, nfdFilter.data(), nfdFilter.size(),
                                       pathToOpen.toString().c_str());

  if (result == NFD_OKAY) {
    CH_LOG_INFO(EditorApp, "Selected file: {0}", outPath.get());

    const Path selectedFilePath(outPath.get());

    auto meshManager = AssetManagerImporter::instance().getImporter<MeshImpotrter>();
    auto importedModel = std::reinterpret_pointer_cast<ModelAsset>(
        meshManager->importAsset(selectedFilePath, selectedFilePath.getFileName(false)));
    m_assets = AssetManager::instance().getAllAssets();

    if (importedModel) {
      CH_LOG_INFO(EditorApp, "Successfully imported model: {0}", selectedFilePath.toString());
      m_nastyRenderer->loadModel(importedModel->getModel());
    }
    else {
      CH_LOG_ERROR(EditorApp, "Failed to import model: {0}", selectedFilePath.toString());
    }
  }
  else if (result == NFD_CANCEL) {
    CH_LOG_INFO(EditorApp, "User cancelled the file selection.");
  }
  else {
    CH_LOG_ERROR(EditorApp, "Error opening file dialog: {0}", NFD::GetError());
  }
}

/*
 */
void
EditorApplication::renderFullScreenRenderer(const RendererOutput& rendererOutput) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize({1280.0f, 720.0f}, ImGuiCond_Always);

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

  if (ImGui::Begin("Renderer Fullscreen", nullptr, window_flags)) {
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
  }
  ImGui::End();
}

/*
 */
void
EditorApplication::renderAssetContent() {
  if (ImGui::Begin("Assets", &ImguiVars::bShowContentWindow)) {
    for (const auto& asset : m_assets) {
      const String ToShow =
          chString::format("{0}: {1}", asset->getName(), asset->getTypeName());

      if (ImGui::Selectable(ToShow.c_str())) {
        CH_LOG_DEBUG(EditorApp, "Selected asset: {0}", asset->getName());
        if (AssetManager::instance().loadAsset(asset)) {
          if (asset->isTypeOf<ModelAsset>()) {
            // If the asset is a ModelAsset, load it into the renderer
            m_nastyRenderer->loadModel(asset->as<ModelAsset>()->getModel());
          }
        }
      }
    }
  }
  ImGui::End();
}
} // namespace chEngineSDK
