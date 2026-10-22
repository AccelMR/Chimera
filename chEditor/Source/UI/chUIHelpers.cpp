/************************************************************************/
/**
 * @file chUIHelpers.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chUIHelpers.h"

#include "chConsoleVariable.h"
#include "chDisplayEventHandle.h"
#include "chDisplaySurface.h"
#include "chEditorTheme.h"
#include "chIGraphicsAPI.h"
#include "chFileSystem.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPath.h"
#include "chMaterialAsset.h"
#include "chTextureAsset.h"
#include "chPrefabAsset.h"
#include "chSceneAsset.h"
#include "chSkeletonAsset.h"
#include "chAnimationClipAsset.h"
#include "chScriptAsset.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"
#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
namespace chUIHelpers {

CH_LOG_DECLARE_STATIC(UIImguiHelper, All);

namespace {
// Off by default: windows outside the main one cost a swap chain each, and Wayland does
// not let applications place their windows.
ConsoleVariable<bool> g_cvarFloatingWindows("Editor.FloatingWindows",
                                            false,
                                            "Lets panels leave the main window.",
                                            "FloatingWindows");
} // namespace

bool UIHelpers::bShowDemoWindow = false;
bool UIHelpers::bRenderImGui = true;
bool UIHelpers::bResetLayout = false;
LinearColor UIHelpers::rendererColor = LinearColor::BLACK;

/*
 */
void
UIHelpers::newFrame()
{
#if USING(CH_DISPLAY_SDL3)
  ImGui_ImplSDL3_NewFrame();
#endif // USING(CH_DISPLAY_SDL3)

  // ImGui reads the style and the default font when the frame starts.
  EditorTheme::applyPending();
  ImGui::NewFrame();
}

AssetIcon
UIHelpers::getIconFromAssetType(const SPtr<IAsset>& asset)
{
  if (asset->isTypeOf<ModelAsset>()) {
    return {AssetType::Model, ICON_FA_CUBE};
  }
  if (asset->isTypeOf<TextureAsset>()) {
    return {AssetType::Texture, ICON_FA_IMAGE};
  }
  if (asset->isTypeOf<MaterialAsset>()) {
    return {AssetType::Material, ICON_FA_PALETTE};
  }
  if (asset->isTypeOf<PrefabAsset>()) {
    return {AssetType::Prefab, ICON_FA_CUBES};
  }
  if (asset->isTypeOf<SceneAsset>()) {
    return {AssetType::Scene, ICON_FA_MAP};
  }
  if (asset->isTypeOf<SkeletonAsset>()) {
    return {AssetType::Skeleton, ICON_FA_BONE};
  }
  if (asset->isTypeOf<AnimationClipAsset>()) {
    return {AssetType::Animation, ICON_FA_RUNNING};
  }
  if (asset->isTypeOf<ScriptAsset>()) {
    return {AssetType::Script, ICON_FA_FILE_CODE};
  }
  return {AssetType::Unknown, ICON_FA_FILE};
}

/*
 */
void
UIHelpers::initConfigFlags()
{
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  if (g_cvarFloatingWindows.get()) {
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  }
}

/*
 */
HEvent
UIHelpers::initializeImGui(const SPtr<DisplaySurface>& display,
                           const SPtr<DisplayEventHandle>& eventHandler,
                           const ANSICHAR* iniFile)
{
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = iniFile;

  initConfigFlags();
  EditorTheme::initialize();

#if USING(CH_DISPLAY_SDL3)
  // The windows ImGui opens need the same window flags the graphics API asked for, and the
  // backend only adds this one when it is started for it.
  const uint64 windowFlags = IGraphicsAPI::instance().getPlatformWindowFlags();
  if ((windowFlags & SDL_WINDOW_VULKAN) != 0) {
    ImGui_ImplSDL3_InitForVulkan(display->getPlatformHandler());
  }
  else {
    ImGui_ImplSDL3_InitForOther(display->getPlatformHandler());
  }
#else
  CH_PARAMETER_UNUSED(display);
#endif // USING(CH_DISPLAY_SDL3)

  return bindEventWindow(eventHandler);
}

/*
 */
void
UIHelpers::shutDownImGui()
{
  if (!ImGui::GetCurrentContext()) {
    return;
  }
#if USING(CH_DISPLAY_SDL3)
  if (ImGui::GetIO().BackendPlatformUserData) {
    ImGui_ImplSDL3_Shutdown();
  }
#endif // USING(CH_DISPLAY_SDL3)
  ImGui::DestroyContext();
}

/*
 */
NODISCARD HEvent
UIHelpers::bindEventWindow(const SPtr<DisplayEventHandle>& eventHandler) {
  if (!eventHandler) {
    CH_LOG_ERROR(UIImguiHelper, "Event handler is null. Cannot bind SDL_Event processing.");
    return HEvent(); // Return an invalid event handle
  }

  return eventHandler->addUpdateInjection([](const Any& platformEvent) -> bool {
#if USING(CH_DISPLAY_SDL3)
    const SDL_Event* event = nullptr;
    if (!AnyUtils::tryGetValue<const SDL_Event*>(platformEvent, event) || !event) {
      CH_LOG_ERROR(UIImguiHelper, "Invalid argument type passed to display event handler.");
      return false;
    }

    // Without new frames ImGui never drains its input queue, so it would keep growing.
    if (!bRenderImGui) {
      return false;
    }
    return ImGui_ImplSDL3_ProcessEvent(event);
#else
    CH_PARAMETER_UNUSED(platformEvent);
    CH_LOG_ERROR(UIImguiHelper, "SDL3 is not enabled. Cannot process SDL_Event.");
    return false;
#endif // USING(CH_DISPLAY_SDL3)
  });
}

/*
 */
const ANSICHAR*
UIHelpers::getFolderIcon(bool bOpen)
{
  return bOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
}

/*
 */
bool
UIHelpers::beginOverlayChild(const ANSICHAR* id, float backgroundAlpha, int32 windowFlags)
{
  ImVec4 background = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
  background.w *= backgroundAlpha;
  ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().WindowRounding);
  return ImGui::BeginChild(id, ImVec2(0.0f, 0.0f),
                           ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY |
                               ImGuiChildFlags_AlwaysUseWindowPadding,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                               static_cast<ImGuiWindowFlags>(windowFlags));
}

/*
 */
void
UIHelpers::endOverlayChild()
{
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

/*
 */
void
UIHelpers::openFileInSystem(const Path& file)
{
  const String path = FileSystem::absolutePath(file).toString();
#if USING(CH_DISPLAY_SDL3)
  const String url = (path.starts_with('/') ? "file://" : "file:///") + path;
  if (!SDL_OpenURL(url.c_str())) {
    CH_LOG_WARNING(UIImguiHelper, "Could not open {0}: {1}", path, SDL_GetError());
  }
#else
  CH_LOG_WARNING(UIImguiHelper, "Opening {0} needs a display backend.", path);
#endif // USING(CH_DISPLAY_SDL3)
}

} // namespace chUIHelpers
} // namespace chEngineSDK
