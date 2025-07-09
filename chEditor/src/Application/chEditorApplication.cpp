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

#include "chEventDispatcherManager.h"
#include "chICommandBuffer.h"
#include "chIDescriptorPool.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chIRenderPass.h"
#include "chLogger.h"

//TODO: probably move this to its own classs
#include "imgui.h"
#if USING(CH_DISPLAY_SDL3)
#include "imgui_impl_sdl3.h"

// TODO: For now only vulkan is supported
#include "imgui_impl_vulkan.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#endif // #include "chDisplayEventHandle.h"

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {

struct VulkanContextData {
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  uint32 graphicsQueueFamilyIndex;
  VkQueue graphicsQueue;
};

static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;

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
void
EditorApplication::onPostInitialize() {
  CH_LOG_INFO(EditorApp, "Post-initialization of EditorApplication.");

  initializeEditorComponents();
  bindEvents();
}

/*
*/
void
EditorApplication::onRender(const float deltaTime, const SPtr<ICommandBuffer>& commandBuffer) {
  CH_LOG_INFO(EditorApp, "Rendering EditorApplication with delta time: {}", deltaTime);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(commandBuffer->getRaw()));


}

/*
*/
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  SPtr<DisplaySurface> display = getDisplaySurface();
  CH_ASSERT(display && "Display surface must not be null.");

  ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    SDL_Window* window = display->getPlatformHandler();

    IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
    const Map<String, Any>& apiContext = graphicAPI.getAPIContext();

    SPtr<VulkanContextData> vulkanContext;
    if (!AnyUtils::tryGetValue<SPtr<VulkanContextData>>(apiContext.at("vulkanContext"), vulkanContext)) {
      CH_LOG_ERROR(EditorApp, "Failed to retrieve Vulkan context data from API context.");
      return;
    }

    const ApplicationRenderContext& renderContext = getRenderComponents();

    SPtr<IDescriptorPool> descriptorPool = graphicAPI.createDescriptorPool({
      8,
      Vector<Pair<DescriptorType, uint32>>{
        { DescriptorType::CombinedImageSampler, 8 }
      }
    });

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = vulkanContext->instance;
    init_info.PhysicalDevice = vulkanContext->physicalDevice;
    init_info.Device = vulkanContext->device;
    init_info.QueueFamily = vulkanContext->graphicsQueueFamilyIndex;
    init_info.Queue = vulkanContext->graphicsQueue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = static_cast<VkDescriptorPool>(descriptorPool->getRaw());
    init_info.RenderPass = static_cast<VkRenderPass>(renderContext.swapChain->getRenderPass()->getRaw());
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount =  renderContext.swapChain->getTextureCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //init_info.DescriptorPoolSize = 1;
    ImGui_ImplVulkan_Init(&init_info);

    getEventHandler()->addUpdateInjection(
    [this](const Vector<Any>& args) -> bool {
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

      return ImGui_ImplSDL3_ProcessEvent(&event);
    }
  );

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

  eventDispatcher.OnKeyUp.connect(
    [this](const KeyBoardData&) {
      // Handle key up events specific to the editor
    });

  eventDispatcher.OnKeyPressed.connect(
    [this](const KeyBoardData&) {
      // Handle key pressed events specific to the editor
    });

  eventDispatcher.OnMouseButtonDown.connect(
    [this](const MouseButtonData&) {
      // Handle mouse button down events specific to the editor
    });
  eventDispatcher.OnMouseButtonUp.connect(
    [this](const MouseButtonData&) {
      // Handle mouse button up events specific to the editor
    });

  eventDispatcher.OnMouseMove.connect(
    [this](const MouseMoveData&) {
      // Handle mouse move events specific to the editor
    });
  eventDispatcher.OnMouseWheel.connect(
    [this](const MouseWheelData&) {
      // Handle mouse wheel events specific to the editor
    });

}

} // namespace chEngineSDK
