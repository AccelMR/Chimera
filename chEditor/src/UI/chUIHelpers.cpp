/************************************************************************/
/**
 * @file chUIHelpers.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chUIHelpers.h"

#include "chEnginePaths.h"
#include "chDisplayEventHandle.h"
#include "chIGraphicsAPI.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPath.h"
#include "chTextureAsset.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"
#include "imgui.h"
#include <nfd.hpp>

#if USING(CH_DISPLAY_SDL3)

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
namespace chUIHelpers {

CH_LOG_DECLARE_STATIC(UIImguiHelper, All);

bool UIHelpers::bShowDemoWindow = false;
bool UIHelpers::bRenderImGui = true;
float UIHelpers::baseFontSize = 1.4f; // Default font size
LinearColor UIHelpers::backgroundColor = LinearColor::Pink;
LinearColor UIHelpers::rendererColor = LinearColor::Black;

/*
 */
void
UIHelpers::newFrame(IGraphicsAPI& graphicAPI) {
#if USING(CH_DISPLAY_SDL3)
  ImGui_ImplSDL3_NewFrame();
#endif // USING(CH_DISPLAY_SDL3)

  ImGui::NewFrame();
  graphicAPI.execute("newFrameImGui");
}

/*
 */
void
UIHelpers::render(IGraphicsAPI& graphicAPI, const SPtr<ICommandBuffer>& commandBuffer) {
  ImGui::Render();
  ImGuiIO& io = ImGui::GetIO();
  // Update and Render additional Platform Windows
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  graphicAPI.execute("renderImGui", {commandBuffer});
}

AssetIcon
UIHelpers::getIconFromAssetType(const SPtr<IAsset>& asset) {
  if (asset->isTypeOf<ModelAsset>()) {
    return {AssetType::Model,
            ICON_FA_CUBE,
            {String(ICON_FA_CUBE) + " " + asset->getName()}}; // Example icon for model assets
  }
  if (asset->isTypeOf<TextureAsset>()) {
    return {
        AssetType::Texture,
        ICON_FA_IMAGE,
        {String(ICON_FA_IMAGE) + " " + asset->getName()}}; // Example icon for texture assets
  }
  // Add more asset types and their corresponding icons as needed
  return {AssetType::Unknown, ICON_FA_FILE}; // Default icon for unknown asset types
}

/*
 */
void
UIHelpers::initStyle() {
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  // ImGuiIO& io = ImGui::GetIO();

  style.FontScaleMain = UIHelpers::baseFontSize;

  // // Bake a fixed style scale. (until we have a solution for dynamic style
  // // scaling, changing this requires resetting Style + calling this again)
  style.ScaleAllSizes(UIHelpers::baseFontSize);

  // // unnecessary. We leave both here for documentation purpose)
  // // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
  style.FontScaleDpi = UIHelpers::baseFontSize;

  // // [Experimental] Automatically overwrite style.FontScaleDpi
  // // in Begin() when Monitor DPI changes. This will scale fonts
  // // but _NOT_ scale sizes/padding for now.
  // io.ConfigDpiScaleFonts = true;

  // // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
  // io.ConfigDpiScaleViewports = true;

  // // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can
  // look
  // // identical to regular ones.
  // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
  //   style.WindowRounding = 0.0f;
  //   style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  // }

  // // light style from Pacôme Danhiez (user itamago)
  // // https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
  // style.Alpha = 1.0f;
  // style.FrameRounding = 3.0f;
  // style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  // style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  // style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
  // // style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  // style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
  // style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
  // style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
  // style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
  // style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  // style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  // style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
  // style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
  // style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
  // style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
  // style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
  // style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
  // style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
  // style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
  // // style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
  // style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
  // style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  // style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
  // style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
  // style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // // style.Colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  // // style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
  // // style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  // style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
  // style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  // style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  // // style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
  // // style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
  // // style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
  // style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  // style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  // style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  // style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  // style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  // // style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

  // for (int i = 0; i <= ImGuiCol_COUNT; i++) {
  //   ImVec4& col = style.Colors[i];
  //   float H, S, V;
  //   ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

  //   if (S < 0.1f) {
  //     V = 1.0f - V;
  //   }
  //   ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
  //   if (col.w < 1.00f) {
  //     col.w *= 1;
  //   }
  // }

  ImVec4* colors = style.Colors;
  // Catppuccin Mocha palette
  const ImVec4 rosewater = ImVec4(0.96f, 0.89f, 0.89f, 1.00f);
  const ImVec4 flamingo = ImVec4(0.95f, 0.79f, 0.79f, 1.00f);
  const ImVec4 pink = ImVec4(0.96f, 0.76f, 0.89f, 1.00f);
  const ImVec4 mauve = ImVec4(0.79f, 0.64f, 0.96f, 1.00f);
  const ImVec4 red = ImVec4(0.96f, 0.48f, 0.42f, 1.00f);
  const ImVec4 maroon = ImVec4(0.92f, 0.49f, 0.64f, 1.00f);
  const ImVec4 peach = ImVec4(0.96f, 0.67f, 0.41f, 1.00f);
  const ImVec4 yellow = ImVec4(0.96f, 0.93f, 0.51f, 1.00f);
  const ImVec4 green = ImVec4(0.65f, 0.89f, 0.63f, 1.00f);
  const ImVec4 teal = ImVec4(0.58f, 0.90f, 0.78f, 1.00f);
  const ImVec4 sky = ImVec4(0.54f, 0.88f, 0.96f, 1.00f);
  const ImVec4 sapphire = ImVec4(0.45f, 0.76f, 0.98f, 1.00f);
  const ImVec4 blue = ImVec4(0.54f, 0.69f, 0.96f, 1.00f);
  const ImVec4 lavender = ImVec4(0.71f, 0.73f, 0.96f, 1.00f);
  const ImVec4 text = ImVec4(0.80f, 0.82f, 0.96f, 1.00f);
  //const ImVec4 subtext1 = ImVec4(0.73f, 0.75f, 0.89f, 1.00f);
  const ImVec4 subtext0 = ImVec4(0.65f, 0.68f, 0.82f, 1.00f);
  //const ImVec4 overlay2 = ImVec4(0.58f, 0.61f, 0.76f, 1.00f);
  const ImVec4 overlay1 = ImVec4(0.50f, 0.54f, 0.69f, 1.00f);
  const ImVec4 overlay0 = ImVec4(0.43f, 0.47f, 0.62f, 1.00f);
  const ImVec4 surface2 = ImVec4(0.36f, 0.40f, 0.56f, 1.00f);
  const ImVec4 surface1 = ImVec4(0.29f, 0.33f, 0.49f, 1.00f);
  const ImVec4 surface0 = ImVec4(0.23f, 0.26f, 0.42f, 1.00f);
  const ImVec4 base = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
  const ImVec4 mantle = ImVec4(0.09f, 0.09f, 0.13f, 1.00f);
  //const ImVec4 crust = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);

  // Main colors
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = subtext0;
  colors[ImGuiCol_WindowBg] = ImVec4(base.x, base.y, base.z, 0.85f);
  colors[ImGuiCol_ChildBg] = ImVec4(surface0.x, surface0.y, surface0.z, 0.2f);
  colors[ImGuiCol_PopupBg] = ImVec4(surface0.x, surface0.y, surface0.z, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(overlay0.x, overlay0.y, overlay0.z, 0.6f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_FrameBg] = ImVec4(surface0.x, surface0.y, surface0.z, 0.6f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(surface1.x, surface1.y, surface1.z, 0.8f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(surface2.x, surface2.y, surface2.z, 0.9f);
  colors[ImGuiCol_TitleBg] = ImVec4(mantle.x, mantle.y, mantle.z, 0.95f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(surface0.x, surface0.y, surface0.z, 0.95f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(mantle.x, mantle.y, mantle.z, 0.7f);

  // Interactive elements with varied colors
  colors[ImGuiCol_Button] = ImVec4(surface1.x, surface1.y, surface1.z, 0.8f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(mauve.x, mauve.y, mauve.z, 0.3f);
  colors[ImGuiCol_ButtonActive] = ImVec4(mauve.x, mauve.y, mauve.z, 0.5f);
  colors[ImGuiCol_Header] = ImVec4(surface1.x, surface1.y, surface1.z, 0.6f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(pink.x, pink.y, pink.z, 0.4f);
  colors[ImGuiCol_HeaderActive] = ImVec4(pink.x, pink.y, pink.z, 0.6f);

  // Selection and highlighting with flamingo/rosewater
  colors[ImGuiCol_CheckMark] = flamingo;
  colors[ImGuiCol_SliderGrab] = ImVec4(rosewater.x, rosewater.y, rosewater.z, 0.8f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(flamingo.x, flamingo.y, flamingo.z, 1.0f);

  // Scrollbars with teal accent
  colors[ImGuiCol_ScrollbarBg] = ImVec4(surface0.x, surface0.y, surface0.z, 0.4f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(teal.x, teal.y, teal.z, 0.6f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(sky.x, sky.y, sky.z, 0.8f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(sapphire.x, sapphire.y, sapphire.z, 0.9f);

  // Separators and resize handles with peach
  colors[ImGuiCol_Separator] = ImVec4(peach.x, peach.y, peach.z, 0.4f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(peach.x, peach.y, peach.z, 0.6f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(yellow.x, yellow.y, yellow.z, 0.8f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(overlay0.x, overlay0.y, overlay0.z, 0.3f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(maroon.x, maroon.y, maroon.z, 0.6f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(red.x, red.y, red.z, 0.8f);

  // Tabs with green accent
  colors[ImGuiCol_Tab] = ImVec4(surface0.x, surface0.y, surface0.z, 0.6f);
  colors[ImGuiCol_TabHovered] = ImVec4(green.x, green.y, green.z, 0.4f);
  colors[ImGuiCol_TabSelected] = ImVec4(green.x, green.y, green.z, 0.6f);
  colors[ImGuiCol_TabDimmed] = ImVec4(surface0.x, surface0.y, surface0.z, 0.4f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(surface1.x, surface1.y, surface1.z, 0.6f);

  // Docking with lavender
  colors[ImGuiCol_DockingPreview] = ImVec4(lavender.x, lavender.y, lavender.z, 0.4f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(base.x, base.y, base.z, 1.0f);

  // Plot colors distributed across palette
  colors[ImGuiCol_PlotLines] = blue;
  colors[ImGuiCol_PlotLinesHovered] = sky;
  colors[ImGuiCol_PlotHistogram] = green;
  colors[ImGuiCol_PlotHistogramHovered] = teal;

  // Table colors
  colors[ImGuiCol_TableHeaderBg] = ImVec4(surface1.x, surface1.y, surface1.z, 0.8f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(overlay1.x, overlay1.y, overlay1.z, 0.8f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(overlay0.x, overlay0.y, overlay0.z, 0.5f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(surface0.x, surface0.y, surface0.z, 0.1f);

  // Text selection
  colors[ImGuiCol_TextSelectedBg] = ImVec4(lavender.x, lavender.y, lavender.z, 0.3f);

  // Drag and drop
  colors[ImGuiCol_DragDropTarget] = ImVec4(lavender.x, lavender.y, lavender.z, 0.8f);

  // Navigation highlight
  colors[ImGuiCol_NavCursor] = ImVec4(lavender.x, lavender.y, lavender.z, 0.8f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(text.x, text.y, text.z, 0.8f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(overlay0.x, overlay0.y, overlay0.z, 0.2f);

  // Modal window dim
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(overlay0.x, overlay0.y, overlay0.z, 0.4f);

  // Styling properties for minimalist look
  style.WindowRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.PopupRounding = 6.0f;
  style.ScrollbarRounding = 12.0f;
  style.GrabRounding = 6.0f;
  style.TabRounding = 6.0f;
  style.ChildRounding = 6.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;

  style.WindowPadding = ImVec2(12.0f, 12.0f);
  style.FramePadding = ImVec2(8.0f, 6.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
  style.IndentSpacing = 20.0f;
  style.ScrollbarSize = 16.0f;
  style.GrabMinSize = 12.0f;

  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.6f;
}

/*
 */
void
UIHelpers::initFontConfig() {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

  io.Fonts->AddFontDefault();

  float iconFontSize = UIHelpers::baseFontSize * 20.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced

  // Merge in icons from Font Awesome
  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = iconFontSize;

  // Build an absolute path when possible (recommend defining CH_CONTENT_DIR via CMake)
  const String fontIconFilePath =
      EnginePaths::getEditorContentDirectory().toString() + "/Fonts/fa-solid-900.ttf";

  io.Fonts->AddFontFromFileTTF(fontIconFilePath.c_str(), iconFontSize, &icons_config, icons_ranges);
}

/*
 */
NODISCARD HEvent
UIHelpers::bindEventWindow(const SPtr<DisplayEventHandle>& eventHandler) {
  if (!eventHandler) {
    CH_LOG_ERROR(UIImguiHelper, "Event handler is null. Cannot bind SDL_Event processing.");
    return HEvent(); // Return an invalid event handle
  }

  return eventHandler->addUpdateInjection([](const Vector<Any>& args) -> bool {
#if USING(CH_DISPLAY_SDL3)
    CH_ASSERT(args.size() == 1 && "Expected exactly one argument of type SDL_Event.");
    if (args.empty()) {
      CH_LOG_ERROR(UIImguiHelper, "No arguments passed to display event handler.");
      return false;
    }

    SDL_Event event;
    if (!AnyUtils::tryGetValue<SDL_Event>(args[0], event)) {
      CH_LOG_ERROR(UIImguiHelper, "Invalid argument type passed to display event handler.");
      return false;
    }

    // Process SDL3 events with ImGui
    return ImGui_ImplSDL3_ProcessEvent(&event);
#else
    CH_PAMRAMETER_UNUSED(args);
    CH_LOG_ERROR(UIImguiHelper, "SDL3 is not enabled. Cannot process SDL_Event.");
    return false;
#endif // USING(CH_DISPLAY_SDL3)
  });
}

/*
 */
Path
UIHelpers::openFileExplorer(const Path& pathToOpen, const Vector<String>& filters) {
  NFD::Guard nfdGuard;
  NFD::UniquePath outPath;

  // This is too specific to the NFD library, but we need to convert the filters
  String supportedExtensionsStr;
  for (SIZE_T i = 0; i < filters.size(); ++i) {
    if (i > 0) {
      supportedExtensionsStr += ",";
    }
    supportedExtensionsStr += filters[i];
  }

  const Vector<nfdfilteritem_t> nfdFilter = {
      {"Supported Files", supportedExtensionsStr.c_str()}};

  nfdresult_t result = NFD::OpenDialog(outPath, nfdFilter.data(), nfdFilter.size(),
                                       pathToOpen.toString().c_str());

  if (result == NFD_OKAY) {
    CH_LOG_INFO(UIImguiHelper, "Selected file: {0}", outPath.get());

    const Path selectedFilePath(outPath.get());
    return selectedFilePath;
  }
  else if (result == NFD_CANCEL) {
    CH_LOG_INFO(UIImguiHelper, "User cancelled the file selection.");
  }
  else {
    CH_LOG_ERROR(UIImguiHelper, "Error opening file dialog: {0}", NFD::GetError());
  }
  return Path(); // Return an empty path if no file was selected or an error occurred
}

} // namespace chUIHelpers
} // namespace chEngineSDK
