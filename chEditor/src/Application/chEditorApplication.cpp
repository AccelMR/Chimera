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

#include "../../assets/fonts/IconsFontAwsome5.h"

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
// Variable to control the visibility of the ImGui demo window
static bool bShowDemoWindow = false;
// Variable to control if ImGui should render
static bool bRenderImGui = true;
// Variable to control the visibility of the file explorer
static bool bShowContentWindow = false;
static float baseFontSize = 1.15f; // Default font size for ImGui

enum class AssetType { Model, Texture, Material, Shader, Unknown };

struct AssetIcon {
  AssetType type;
  const char* icon; // FontAwesome icon name
  const String IconName = "";
};

static AssetIcon
getIconFromAssetType(const SPtr<IAsset>& asset) {
  if (asset->isTypeOf<ModelAsset>()) {
    return {
      AssetType::Model,
      ICON_FA_CUBE,
      {String(ICON_FA_CUBE) + " " + asset->getName()}
    }; // Example icon for model assets
  }
  // Add more asset types and their corresponding icons as needed
  return {AssetType::Unknown, ICON_FA_FILE}; // Default icon for unknown asset types
}

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
      if (ImGui::SliderFloat("Font Size", &ImguiVars::baseFontSize, 1.0f, 5.0f, "%.1f",
                             ImGuiSliderFlags_AlwaysClamp)) {
        ImGui::GetStyle().FontScaleMain = ImguiVars::baseFontSize;
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
        openFileExplorer(EnginePaths::getAbsoluteGameAssetDirectory(), supportedExtensions);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  renderContentBrowser();
  renderDeleteConfirmation();

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
  AssetManager::instance().lazyLoadAssetsFromDirectory(
      EnginePaths::getAbsoluteGameAssetDirectory());
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

  io.Fonts->AddFontDefault();
  float iconFontSize = ImguiVars::baseFontSize * 20.0f / 3.0f ; // FontAwesome fonts need to have their sizes reduced

  // merge in icons from Font Awesome
  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = iconFontSize;
  io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS,
                               iconFontSize,
                               &icons_config,
                               icons_ranges);

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();

  style.FontScaleMain = ImguiVars::baseFontSize;

  // Bake a fixed style scale. (until we have a solution for dynamic style
  // scaling, changing this requires resetting Style + calling this again)
  style.ScaleAllSizes(ImguiVars::baseFontSize);

  // unnecessary. We leave both here for documentation purpose)
  // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
  style.FontScaleDpi = ImguiVars::baseFontSize;

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

  // light style from Pacôme Danhiez (user itamago)
  // https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
  style.Alpha = 1.0f;
  style.FrameRounding = 3.0f;
  style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
  // style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
  // style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  // style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
  // style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  // style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
  // style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
  // style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  // style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

  for (int i = 0; i <= ImGuiCol_COUNT; i++) {
    ImVec4& col = style.Colors[i];
    float H, S, V;
    ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

    if (S < 0.1f) {
      V = 1.0f - V;
    }
    ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
    if (col.w < 1.00f) {
      col.w *= 1;
    }
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
EditorApplication::renderContentBrowser() {
  if (ImGui::Begin("Content Browser", &ImguiVars::bShowContentWindow)) {
    // Search bar
    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextWithHint("##search", "Search assets...", searchBuffer,
                                 sizeof(searchBuffer))) {
      // Filter will be applied below
    }

    ImGui::Separator();

    // Asset type filter buttons
    static bool showAllTypes = true;
    static bool showModels = true;
    static bool showTextures = true;
    static bool showMaterials = true;
    static bool showOther = true;

    if (ImGui::Button("All")) {
      showAllTypes = true;
      showModels = showTextures = showMaterials = showOther = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("Models")) {
      showAllTypes = false;
      showModels = true;
      showTextures = showMaterials = showOther = false;
    }
    ImGui::SameLine();

    if (ImGui::Button("Textures")) {
      showAllTypes = false;
      showTextures = true;
      showModels = showMaterials = showOther = false;
    }
    ImGui::SameLine();

    if (ImGui::Button("Materials")) {
      showAllTypes = false;
      showMaterials = true;
      showModels = showTextures = showOther = false;
    }
    ImGui::SameLine();

    if (ImGui::Button("Other")) {
      showAllTypes = false;
      showOther = true;
      showModels = showTextures = showMaterials = false;
    }

    ImGui::Separator();

    // View mode toggle
    static bool gridView = true;
    if (ImGui::RadioButton("Grid View", gridView)) {
      gridView = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("List View", !gridView)) {
      gridView = false;
    }

    // Grid size slider for grid view
    static float gridSize = 80.0f;
    if (gridView) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      ImGui::SliderFloat("Size", &gridSize, 50.0f, 150.0f, "%.0f");
    }

    ImGui::Separator();

    // Asset display area
    ImGui::BeginChild("AssetArea", ImVec2(0, 0), false);

    // Convert search to lowercase for case-insensitive search
    String searchStr = String(searchBuffer);
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    if (gridView) {
      // Grid view implementation
      float windowWidth = ImGui::GetContentRegionAvail().x;
      int columns = static_cast<int>(windowWidth / (gridSize + 10.0f));
      if (columns < 1) {
        columns = 1;
      }

      int currentColumn = 0;

      for (const auto& asset : m_assets) {
        // Filter by search
        if (!searchStr.empty()) {
          String assetName = asset->getName();
          std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
          if (assetName.find(searchStr) == String::npos) {
            continue;
          }
        }

        // Filter by asset type
        ImguiVars::AssetIcon assetIcon = ImguiVars::getIconFromAssetType(asset);
        if (!showAllTypes) {
          bool shouldShow = false;
          switch (assetIcon.type) {
          case ImguiVars::AssetType::Model:
            shouldShow = showModels;
            break;
          case ImguiVars::AssetType::Texture:
            shouldShow = showTextures;
            break;
          case ImguiVars::AssetType::Material:
            shouldShow = showMaterials;
            break;
          default:
            shouldShow = showOther;
            break;
          }
          if (!shouldShow) {
            continue;
          }
        }

        if (assetIcon.type == ImguiVars::AssetType::Unknown) {
          continue; // Skip unknown asset types
        }

        // Position for grid layout
        if (currentColumn > 0) {
          ImGui::SameLine();
        }

        ImGui::PushID(asset->getUUID().toString().c_str());
        ImGui::BeginGroup();

        // Create unique ID for this asset
        String buttonId = chString::format("##asset_{0}", asset->getUUID().toString());

        // Asset icon button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

        if (ImGui::Button(buttonId.c_str(), ImVec2(gridSize, gridSize))) {
          // Asset selection logic
          handleAssetSelection(asset);
        }

        ImGui::PopStyleColor(3);

        // Draw icon on button
        ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImVec2 buttonMax = ImGui::GetItemRectMax();
        ImVec2 iconPos = ImVec2(buttonMin.x + (gridSize - 32) * 0.5f,
                                buttonMin.y + (gridSize - 32) * 0.5f - 10);

        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                            IM_COL32(255, 255, 255, 255), assetIcon.icon);

        // Asset name (truncated if too long)
        String displayName = asset->getName();
        if (displayName.length() > 12) {
          displayName = displayName.substr(0, 9) + "...";
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             (gridSize - ImGui::CalcTextSize(displayName.c_str()).x) * 0.5f);
        ImGui::Text("%s", displayName.c_str());

        // Asset state indicator
        ImVec4 stateColor = getAssetStateColor(asset);
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(buttonMax.x - 8, buttonMin.y + 8), 3.0f,
            ImGui::ColorConvertFloat4ToU32(stateColor));

        ImGui::EndGroup();

        // Make the group selectable for context menu
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          ImGui::OpenPopup(
              chString::format("AssetContext_{0}", asset->getUUID().toString()).c_str());
        }

        // Context menu with explicit ID
        if (ImGui::BeginPopup(
                chString::format("AssetContext_{0}", asset->getUUID().toString()).c_str())) {
          renderAssetContextMenu(asset);
          ImGui::EndPopup();
        }

        ImGui::PopID(); // Pop the unique ID for this asset

        // Tooltip
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("Name: %s", asset->getName());
          ImGui::Text("Type: %s", asset->getTypeName());
          ImGui::Text("UUID: %s", asset->getUUID().toString().c_str());
          ImGui::Text("State: %s", getAssetStateString(asset).c_str());
          ImGui::Text("Original Path: %s", asset->getOriginalPath());
          ImGui::EndTooltip();
        }

        currentColumn = (currentColumn + 1) % columns;
      }
    }
    else {
      // List view implementation
      if (ImGui::BeginTable("AssetTable", 4,
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                                ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const auto& asset : m_assets) {
          // Filter by search
          if (!searchStr.empty()) {
            String assetName = asset->getName();
            std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
            if (assetName.find(searchStr) == String::npos) {
              continue;
            }
          }

          // Filter by asset type
          ImguiVars::AssetIcon assetIcon = ImguiVars::getIconFromAssetType(asset);
          if (!showAllTypes) {
            bool shouldShow = false;
            switch (assetIcon.type) {
            case ImguiVars::AssetType::Model:
              shouldShow = showModels;
              break;
            case ImguiVars::AssetType::Texture:
              shouldShow = showTextures;
              break;
            case ImguiVars::AssetType::Material:
              shouldShow = showMaterials;
              break;
            default:
              shouldShow = showOther;
              break;
            }
            if (!shouldShow) {
              continue;
            }
          }

          if (assetIcon.type == ImguiVars::AssetType::Unknown) {
            continue; // Skip unknown asset types
          }

          ImGui::TableNextRow();

          // Icon column
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", assetIcon.icon);

          // Name column
          ImGui::TableSetColumnIndex(1);
          String selectableId = chString::format("{0}##asset_{1}", asset->getName(),
                                                 asset->getUUID().toString());
          if (ImGui::Selectable(selectableId.c_str(), false,
                                ImGuiSelectableFlags_SpanAllColumns)) {
            handleAssetSelection(asset);
          }

          // Context menu
          if (ImGui::BeginPopupContextItem()) {
            renderAssetContextMenu(asset);
            ImGui::EndPopup();
          }

          // Type column
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%s", asset->getTypeName());

          // State column
          ImGui::TableSetColumnIndex(3);
          ImVec4 stateColor = getAssetStateColor(asset);
          ImGui::TextColored(stateColor, "%s", getAssetStateString(asset).c_str());
        }

        ImGui::EndTable();
      }
    }

    ImGui::EndChild();
  }
  ImGui::End();
}

/*
*/
void
EditorApplication::renderDeleteConfirmation() {
  if (!m_showDeleteConfirmation || !m_assetToDelete) {
    m_showDeleteConfirmation = false;
    return; // No asset to delete or confirmation not needed
  }
  // Render the delete confirmation popup
  ImGui::OpenPopup("Delete Asset?");

  if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (m_assetToDelete) {
      ImGui::Text("Are you sure you want to delete this asset?");
      ImGui::Separator();
      ImGui::Text("Name: %s", m_assetToDelete->getName());
      ImGui::Text("Type: %s", m_assetToDelete->getTypeName());
      ImGui::Separator();
      ImGui::Text("This action cannot be undone!");

      // Buttons
      if (ImGui::Button("Delete", ImVec2(120, 0))) {
        // Perform deletion
        const String assetWithExt(String(m_assetToDelete->getName()) + ".chAss");
        const Path fullAssetToDelete =
            EnginePaths::getAbsoluteGameAssetDirectory() / assetWithExt;
        const bool bRemovedCorrectly = FileSystem::removeFile(fullAssetToDelete);

        if (bRemovedCorrectly) {
          AssetManager::instance().removeAsset(m_assetToDelete->getUUID());
          m_assets = AssetManager::instance().getAllAssets();
          CH_LOG_DEBUG(EditorApp, "Deleted asset: {0}", m_assetToDelete->getName());
        }
        else {
          CH_LOG_ERROR(EditorApp, "Failed to delete asset file: {0}",
                       m_assetToDelete->getName());
        }

        m_assetToDelete = nullptr;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_assetToDelete = nullptr;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
}

// Helper method to handle asset selection
void
EditorApplication::handleAssetSelection(const SPtr<IAsset>& asset) {
  CH_LOG_DEBUG(EditorApp, "Selected asset: {0}", asset->getName());

  if (AssetManager::instance().loadAsset(asset)) {
    CH_LOG_DEBUG(EditorApp, "Loading asset: {0}", asset->getName());

    // Handle different asset types
    if (asset->isTypeOf<ModelAsset>()) {
      m_nastyRenderer->loadModel(asset->as<ModelAsset>()->getModel());
      CH_LOG_DEBUG(EditorApp, "Loaded model asset: {0}", asset->getName());
    }
    // Add more asset type handling here as needed
    // else if (asset->isTypeOf<TextureAsset>()) { ... }
    // else if (asset->isTypeOf<MaterialAsset>()) { ... }
  }
  else {
    CH_LOG_ERROR(EditorApp, "Failed to load asset: {0}", asset->getName());
  }
}

// Helper method to render context menu for assets
void
EditorApplication::renderAssetContextMenu(const SPtr<IAsset>& asset) {
  if (ImGui::MenuItem("Load")) {
    handleAssetSelection(asset);
  }

  if (ImGui::MenuItem("Unload", nullptr, false, asset->isLoaded())) {
    CH_LOG_DEBUG(EditorApp, "Unloading asset: {0}", asset->getName());
    AssetManager::instance().unloadAsset(asset->getUUID());
    m_nastyRenderer->loadModel(nullptr);
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Rename")) {
    AssetManager::instance().renameAsset(asset, "new_name");
  }

  if (ImGui::MenuItem("Show in Explorer")) {
    // Implement file explorer opening logic
    CH_LOG_DEBUG(EditorApp, "Opening file explorer for: {0}", asset->getName());
  }

  if (ImGui::MenuItem("Properties")) {
    // Implement asset properties window
    CH_LOG_DEBUG(EditorApp, "Opening properties for: {0}", asset->getName());
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Delete", nullptr, false, asset->isUnloaded())) {
    // Set the asset to delete and open the confirmation popup
    m_assetToDelete = asset;
    m_showDeleteConfirmation = true;

    ImGui::CloseCurrentPopup();
  }
}

// Helper method to get asset state color
ImVec4
EditorApplication::getAssetStateColor(const SPtr<IAsset>& asset) {
  switch (asset->getState()) {
  case AssetState::Loaded:
    return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
  case AssetState::Loading:
    return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
  case AssetState::Unloaded:
    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
  case AssetState::Unloading:
    return ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
  case AssetState::Failed:
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
  default:
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
  }
}

// Helper method to get asset state string
String
EditorApplication::getAssetStateString(const SPtr<IAsset>& asset) {
  switch (asset->getState()) {
  case AssetState::Loaded:
    return "Loaded";
  case AssetState::Loading:
    return "Loading";
  case AssetState::Unloaded:
    return "Unloaded";
  case AssetState::Unloading:
    return "Unloading";
  case AssetState::Failed:
    return "Failed";
  default:
    return "Unknown";
  }
}

} // namespace chEngineSDK
