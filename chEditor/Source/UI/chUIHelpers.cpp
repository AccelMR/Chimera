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
#include "chEngineConfig.h"
#include "chEnginePaths.h"
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
#include "chAnimationGraphAsset.h"
#include "chScriptAsset.h"
#include "chStringUtils.h"

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

ConsoleVariable<String> g_cvarScriptEditor(
    "Editor.ScriptEditor",
    "code",
    "Command of the code editor scripts open in, given the project folder and the script "
    "(code, codium, cursor), so its language server reads the project's .luarc.json. Empty, "
    "or not found on the PATH, opens scripts with the system's program.",
    "ScriptEditor");

constexpr SIZE_T kCommandBufferSize = 256;

/*
 * The command as a file: a path as it is, else the first PATH folder that has it. Windows
 * also tries the extensions it runs; a bare name is never taken there, because editors
 * keep a shell script of that name next to their .cmd for WSL.
 */
Path
findCommand(const String& command)
{
  if (command.find('/') != String::npos || command.find('\\') != String::npos) {
    return FileSystem::isFile(Path(command)) ? Path(command) : Path();
  }
#if USING(CH_DISPLAY_SDL3)
  const ANSICHAR* const pathVariable = SDL_getenv("PATH");
  if (!pathVariable) {
    return Path();
  }
# if USING(CH_PLATFORM_WIN32)
  constexpr ANSICHAR kSeparator = ';';
  const bool bHasExtension = command.find('.') != String::npos;
  const Vector<String> candidates =
      bHasExtension ? Vector<String>{command}
                    : Vector<String>{command + ".exe", command + ".cmd", command + ".bat"};
# else
  constexpr ANSICHAR kSeparator = ':';
  const Vector<String> candidates{command};
# endif // USING(CH_PLATFORM_WIN32)
  const StringView folders(pathVariable);
  SIZE_T start = 0;
  while (start <= folders.size()) {
    SIZE_T end = folders.find(kSeparator, start);
    if (end == StringView::npos) {
      end = folders.size();
    }
    const StringView folder = folders.substr(start, end - start);
    if (!folder.empty()) {
      for (const String& candidate : candidates) {
        const Path file = Path(String(folder)).join(Path(candidate));
        if (FileSystem::isFile(file)) {
          return file;
        }
      }
    }
    start = end + 1;
  }
#endif // USING(CH_DISPLAY_SDL3)
  return Path();
}

/*
 * Starts the editor with the project folder and the file, without waiting for it.
 */
bool
launchScriptEditor(const Path& editor, const Path& folder, const Path& file)
{
#if USING(CH_DISPLAY_SDL3)
  const SDL_PropertiesID properties = SDL_CreateProperties();
  SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
# if USING(CH_PLATFORM_WIN32)
  // A .cmd only runs through cmd.exe, whose quoting the SDL argument list does not follow;
  // /s with the whole line in quotes keeps quoted paths with spaces as they are.
  const auto quote = [](const Path& path) {
    return "\"" + StringUtils::replaceAllChars(path.toString(), '/', '\\') + "\"";
  };
  const String extension = StringUtils::toLower(editor.getExtension());
  const String arguments = quote(editor) + " " + quote(folder) + " " + quote(file);
  const String commandLine = extension == ".cmd" || extension == ".bat"
                                 ? "cmd.exe /d /s /c \"" + arguments + "\""
                                 : arguments;
  SDL_SetStringProperty(properties, SDL_PROP_PROCESS_CREATE_CMDLINE_STRING,
                        commandLine.c_str());
# else
  const ANSICHAR* arguments[] = {editor.toString().c_str(), folder.toString().c_str(),
                                 file.toString().c_str(), nullptr};
  SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, arguments);
# endif // USING(CH_PLATFORM_WIN32)
  SDL_Process* process = SDL_CreateProcessWithProperties(properties);
  SDL_DestroyProperties(properties);
  if (!process) {
    CH_LOG_WARNING(UIImguiHelper, "Could not start {0}: {1}", editor, SDL_GetError());
    return false;
  }
  // Only the handle goes; the editor keeps running.
  SDL_DestroyProcess(process);
  return true;
#else
  CH_PARAMETER_UNUSED(editor);
  CH_PARAMETER_UNUSED(folder);
  CH_PARAMETER_UNUSED(file);
  return false;
#endif // USING(CH_DISPLAY_SDL3)
}
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
  if (asset->isTypeOf<AnimationGraphAsset>()) {
    return {AssetType::AnimationGraph, ICON_FA_PROJECT_DIAGRAM};
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

/*
 */
void
UIHelpers::openScript(const Path& file)
{
  const String& command = g_cvarScriptEditor.get();
  if (!command.empty() && EnginePaths::hasProject()) {
    const Path editor = findCommand(command);
    if (!editor.empty()) {
      if (launchScriptEditor(editor, FileSystem::absolutePath(EnginePaths::getProjectDirectory()),
                             FileSystem::absolutePath(file))) {
        return;
      }
    }
    else {
      CH_LOG_WARNING(UIImguiHelper, "The script editor '{0}' is not on the PATH; opening the "
                     "script with the system's program.", command);
    }
  }
  openFileInSystem(file);
}

/*
 */
void
UIHelpers::renderScriptEditorSettings()
{
  ANSICHAR buffer[kCommandBufferSize];
  StringUtils::copyToBuffer(buffer, g_cvarScriptEditor.get());
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
  if (ImGui::InputText("Command", buffer, sizeof(buffer))) {
    g_cvarScriptEditor.set(String(buffer), ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", g_cvarScriptEditor.getHelp().c_str());
  bool bSave = ImGui::IsItemDeactivatedAfterEdit();
  if (ImGui::Button("Reset to Default")) {
    g_cvarScriptEditor.set(g_cvarScriptEditor.getDefault(), ConsoleVariableSource::Console);
    bSave = true;
  }
  if (bSave) {
    EngineConfig::setUserValue("Editor", "Editor", "ScriptEditor", g_cvarScriptEditor.get());
  }
}

} // namespace chUIHelpers
} // namespace chEngineSDK
