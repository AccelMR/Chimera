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
#include "chIRenderPass.h"
#include "chISwapChain.h"
#include "chLogger.h"

// TODO: probably move this to its own classs
#include "imgui.h"
#if USING(CH_DISPLAY_SDL3)
#include "imgui_impl_sdl3.h"

// TODO: For now only vulkan is supported
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#endif // #include "chDisplayEventHandle.h"

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {

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
  CH_PAMRAMETER_UNUSED(deltaTime);

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();
  graphicAPI.execute("newFrameImGui");
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

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

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
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

  const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(
      main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style
                   // scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi =
      main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
                  // unnecessary. We leave both here for documentation purpose)
  io.ConfigDpiScaleFonts = true; // [Experimental] Automatically overwrite style.FontScaleDpi
                                 // in Begin() when Monitor DPI changes. This will scale fonts
                                 // but _NOT_ scale sizes/padding for now.
  io.ConfigDpiScaleViewports =
      true; // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
  // identical to regular ones.
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  IGraphicsAPI& graphicAPI = IGraphicsAPI::instance();

  SPtr<IDescriptorPool> descriptorPool = graphicAPI.createDescriptorPool(
    {100, Vector<Pair<DescriptorType, uint32>>{{DescriptorType::CombinedImageSampler, 100}}});
  graphicAPI.execute("initImGui", {display, getRenderComponents().swapChain, descriptorPool });

  getEventHandler()->addUpdateInjection([this](const Vector<Any>& args) -> bool {
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

    return !ImGui_ImplSDL3_ProcessEvent(&event);
  });

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
      } else if (data.key == chKeyBoard::Key::O) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+O pressed, opening a document.");
        // Handle Ctrl+O for opening a document
      }
    }
  });

  eventDispatcher.OnKeyUp.connect([this](const KeyBoardData&) {
    // Handle key up events specific to the editor
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

} // namespace chEngineSDK
