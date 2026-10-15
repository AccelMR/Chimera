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
#include "chEditorTheme.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chPath.h"
#include "chMaterialAsset.h"
#include "chTextureAsset.h"
#include "chPrefabAsset.h"
#include "chSceneAsset.h"

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
LinearColor UIHelpers::rendererColor = LinearColor::Black;

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

  const nfdfilteritem_t nfdFilter{"Supported Files", supportedExtensionsStr.c_str()};

  nfdresult_t result = NFD::OpenDialog(outPath, &nfdFilter, 1, pathToOpen.toString().c_str());

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


/*
 */
const ANSICHAR*
UIHelpers::getFolderIcon(bool bOpen)
{
  return bOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
}

} // namespace chUIHelpers
} // namespace chEngineSDK
