/************************************************************************/
/**
 * @file chPackageDialog.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Asks how to package the project and runs the packager.
 */
/************************************************************************/
#include "chPackageDialog.h"

#include "chAlgorithm.h"
#include "chAssetManager.h"
#include "chConfigFile.h"
#include "chEditorScene.h"
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMath.h"
#include "chProjectPackager.h"
#include "chSTDThreading.h"
#include "chSceneAsset.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

#if USING(CH_DISPLAY_SDL3)
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(PackageDialogLog, All);

namespace {
constexpr const ANSICHAR* kPopupName = ICON_FA_BOX_OPEN "  Package Project###PackageDialog";
constexpr const ANSICHAR* kDevelopmentSuffix = "_Development";
constexpr const ANSICHAR* kDefaultStartupScene = "/Game/Maps/Main";
constexpr SIZE_T kPathBufferSize = 1024;
constexpr ImVec4 kWarningColor(1.0f, 0.75f, 0.3f, 1.0f);
constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);
constexpr ImVec4 kSuccessColor(0.5f, 0.9f, 0.5f, 1.0f);

enum class PackageConfiguration : uint8
{
  Release,
  Debug
};
// The arrays below are indexed by PackageConfiguration.
constexpr const ANSICHAR* kConfigurationNames[] = {"Release", "Debug"};
// A debug package is for testing, so its folder says so.
constexpr const ANSICHAR* kConfigurationSuffixes[] = {"", "_Debug"};

// Where the game presets of CMakePresets.json put their binaries, best first.
#if USING(CH_PLATFORM_WIN32)
const ANSICHAR* const kReleaseBuildCandidates[] = {"build/windows-msvc-game/lib/Release",
                                                   "build/noeditor/lib/Release",
                                                   "build/noeditor/lib/RelWithDebInfo"};
const ANSICHAR* const kDebugBuildCandidates[] = {"build/windows-msvc-game/lib/Debug",
                                                 "build/noeditor/lib/Debug"};
constexpr const ANSICHAR* kBuildCommands[] = {
    "cmake --preset windows-msvc-game\ncmake --build --preset windows-msvc-game-release",
    "cmake --preset windows-msvc-game\ncmake --build --preset windows-msvc-game-debug"};
#else
const ANSICHAR* const kReleaseBuildCandidates[] = {"build/linux-clang-game/lib"};
const ANSICHAR* const kDebugBuildCandidates[] = {"build/linux-clang-game-debug/lib"};
constexpr const ANSICHAR* kBuildCommands[] = {
    "cmake --preset linux-clang-game\ncmake --build --preset linux-clang-game",
    "cmake --preset linux-clang-game-debug\ncmake --build --preset linux-clang-game-debug"};
#endif // USING(CH_PLATFORM_WIN32)

// The folders chosen last are remembered in the user's Editor.ini as plain values, not
// settings: the dialog asks for them every time and only proposes the last answer.
constexpr const ANSICHAR* kRememberedSection = "Package";
constexpr const ANSICHAR* kOutputDirectoryKey = "OutputDirectory";
constexpr const ANSICHAR* kBuildDirectoryKeys[] = {"ReleaseBuildDirectory",
                                                   "DebugBuildDirectory"};

// What the editor packages lives with the project (shared through version control), not
// with the user, so it is its own file in the project's Config folder.
constexpr const ANSICHAR* kSettingsSection = "Package";
constexpr const ANSICHAR* kScenesKey = "Scenes";
// A finished package stays on screen this long, unless the mouse is over it.
constexpr double kResultSeconds = 8.0;
constexpr SIZE_T kMaxSceneRows = 8;

enum class BrowseTarget : uint8
{
  None,
  Output,
  Build
};

struct SceneEntry
{
  String path;
  bool bIncluded = false;
};

struct DialogState
{
  bool bOpenRequested = false;
  ANSICHAR gameName[128]{};
  ANSICHAR outputDirectory[kPathBufferSize]{};
  String startupScene;
  // Every scene of the project, sorted by path; the startup scene is always packaged.
  Vector<SceneEntry> scenes;
  // Release unless changed while the dialog is open, so a player never gets a debug build
  // by accident.
  PackageConfiguration configuration = PackageConfiguration::Release;
  // A build of that configuration; empty when none was found.
  Path gameBuild;
  // Why the folder chosen by hand was not taken.
  String buildMessage;
  // The editor's own folder, when it holds the game runtime; used for development packages.
  Path editorBuild;
  // The folder dialog keeps a pointer to it until it answers.
  String defaultLocation;
  BrowseTarget browseTarget = BrowseTarget::None;
  // Written by the folder dialog's callback, which may run on another thread; chosenFolder
  // is only read after bFolderChosen is seen true.
  String chosenFolder;
  Atomic<bool> bFolderChosen{false};

  // The package being written. The worker only touches progress and result, and result is
  // only read after bJobDone is seen true.
  bool bJobShown = false;
  // Set when a job starts, so its window comes in front of the docked panels once.
  bool bJobAppearing = false;
  String jobName;
  Path jobDirectory;
  uint32 missingAssetCount = 0;
  UniquePtr<PackageProgress> progress;
  Thread worker;
  PackageResult result;
  Atomic<bool> bJobDone{false};
  // When the result was first drawn; the window closes kResultSeconds later.
  double resultTime = -1.0;
};

DialogState&
getState()
{
  static DialogState state;
  return state;
}

String
getRemembered(StringView key)
{
  const String* value = EngineConfig::getFile("Editor").getValue(kRememberedSection, key);
  return value ? *value : String();
}

void
remember(StringView key, StringView value)
{
  if (getRemembered(key) != value) {
    EngineConfig::setUserValue("Editor", kRememberedSection, key, value);
  }
}

Path
getSettingsPath()
{
  return EnginePaths::getProjectConfigDirectory().join(
      Path(String(ProjectPackager::PACKAGE_SETTINGS_FILE)));
}

bool
isBuildOf(const Path& directory, PackageConfiguration configuration)
{
  return ProjectPackager::isGameBinaryDirectory(directory) &&
         ProjectPackager::isDebugBuild(directory) ==
             (configuration == PackageConfiguration::Debug);
}

/*
 * The folder chosen last for the configuration, while it still holds such a build; else
 * the folders the game presets write.
 */
Path
findGameBuild(PackageConfiguration configuration)
{
  const SIZE_T index = static_cast<SIZE_T>(configuration);
  const String remembered = getRemembered(kBuildDirectoryKeys[index]);
  if (!remembered.empty()) {
    const Path directory = FileSystem::absolutePath(Path(remembered));
    if (isBuildOf(directory, configuration)) {
      return directory;
    }
  }

  const Span<const ANSICHAR* const> candidates =
      configuration == PackageConfiguration::Debug ? Span<const ANSICHAR* const>(
                                                         kDebugBuildCandidates)
                                                   : Span<const ANSICHAR* const>(
                                                         kReleaseBuildCandidates);
  const Path& engineRoot = EnginePaths::getEngineRootDirectory();
  for (const ANSICHAR* candidate : candidates) {
    const Path directory = engineRoot.join(Path(candidate));
    if (isBuildOf(directory, configuration)) {
      return directory;
    }
  }
  return Path();
}

Path
getDefaultOutputParent()
{
  return EnginePaths::getProjectDirectory().join(Path("Saved/Packages"));
}

Path
getOutputParent()
{
  const String remembered = getRemembered(kOutputDirectoryKey);
  if (!remembered.empty()) {
    return FileSystem::absolutePath(Path(remembered));
  }
  return getDefaultOutputParent();
}

String
getProjectName()
{
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(EnginePaths::getProjectDirectory(), files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == EnginePaths::PROJECT_EXTENSION) {
      return file.getFileName(false);
    }
  }
  return "Game";
}

String
getScenePath(const IAsset& asset)
{
  String path(asset.getAssetPath());
  path += '/';
  path += asset.getName();
  return path;
}

/*
 * What the game plays today: its config, else the open scene, else the usual name.
 */
String
getDefaultStartupScene()
{
  if (const String* configured =
          EngineConfig::getFile("Game").getValue("Game", "StartupScene")) {
    return *configured;
  }
  if (const SPtr<SceneAsset>& open = EditorScene::getAsset()) {
    return getScenePath(*open);
  }
  return kDefaultStartupScene;
}

/*
 * Lists the project's scenes, ticked as the settings file says.
 */
void
loadScenes(DialogState& state)
{
  ConfigFile settings;
  settings.load(getSettingsPath());
  Vector<String> included;
  if (const String* value = settings.getValue(kSettingsSection, kScenesKey)) {
    included = StringUtils::splitString(*value, ',');
  }

  state.scenes.clear();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (asset->isTypeOf<SceneAsset>()) {
      String path = getScenePath(*asset);
      const bool bIncluded = Algorithm::contains(included, path);
      state.scenes.push_back({std::move(path), bIncluded});
    }
  }
  Algorithm::sort(state.scenes, [](const SceneEntry& a, const SceneEntry& b) {
    return a.path < b.path;
  });
}

/*
 * Written only when the ticks changed, so packaging does not touch a file under version
 * control for nothing.
 */
void
saveScenes(const DialogState& state)
{
  Vector<String> included;
  for (const SceneEntry& scene : state.scenes) {
    if (scene.bIncluded && scene.path != state.startupScene) {
      included.push_back(scene.path);
    }
  }
  const String value = StringUtils::join(included, ",");

  ConfigFile settings;
  settings.load(getSettingsPath());
  const String* saved = settings.getValue(kSettingsSection, kScenesKey);
  if ((saved ? *saved : String()) == value) {
    return;
  }
  settings.setValue(kSettingsSection, kScenesKey, value);
  if (!settings.save(getSettingsPath())) {
    CH_LOG_ERROR(PackageDialogLog, "Could not write {0}", getSettingsPath());
  }
}

#if USING(CH_DISPLAY_SDL3)
/*
 */
void SDLCALL
onFolderChosen(void* userData, const ANSICHAR* const* fileList, int32 filter)
{
  CH_PARAMETER_UNUSED(userData);
  CH_PARAMETER_UNUSED(filter);
  DialogState& state = getState();
  state.chosenFolder.clear();
  if (!fileList) {
    // SDL keeps the error per thread, so it must be read here.
    CH_LOG_ERROR(PackageDialogLog, "Could not open the folder dialog: {0}", SDL_GetError());
  }
  else if (fileList[0]) {
    state.chosenFolder = fileList[0];
  }
  state.bFolderChosen.store(true, std::memory_order_release);
}
#endif // USING(CH_DISPLAY_SDL3)

void
browseFolder(DialogState& state, BrowseTarget target, const Path& start)
{
  if (state.browseTarget != BrowseTarget::None) {
    return;
  }
#if USING(CH_DISPLAY_SDL3)
  state.browseTarget = target;
  // A trailing separator tells SDL the location is a folder, and the Windows shell does not
  // parse forward slashes.
  state.defaultLocation = start.toString() + "/";
#if USING(CH_PLATFORM_WIN32)
  state.defaultLocation = StringUtils::replaceAllChars(state.defaultLocation, '/', '\\');
#endif // USING(CH_PLATFORM_WIN32)
  // The SDL3 platform backend keeps the SDL window id in PlatformHandle.
  SDL_Window* window = SDL_GetWindowFromID(static_cast<SDL_WindowID>(
      reinterpret_cast<SIZE_T>(ImGui::GetMainViewport()->PlatformHandle)));
  SDL_ShowOpenFolderDialog(onFolderChosen, nullptr, window, state.defaultLocation.c_str(),
                           false);
#else
  CH_PARAMETER_UNUSED(target);
  CH_PARAMETER_UNUSED(start);
  CH_LOG_ERROR(PackageDialogLog, "There is no folder dialog without SDL3.");
#endif // USING(CH_DISPLAY_SDL3)
}

void
takeChosenFolder(DialogState& state)
{
  if (state.browseTarget == BrowseTarget::None ||
      !state.bFolderChosen.load(std::memory_order_acquire)) {
    return;
  }
  state.bFolderChosen.store(false, std::memory_order_relaxed);
  const BrowseTarget target = state.browseTarget;
  state.browseTarget = BrowseTarget::None;
  if (state.chosenFolder.empty()) {
    return;
  }

  const String folder = Path(state.chosenFolder).toString();
  if (target == BrowseTarget::Output) {
    StringUtils::copyToBuffer(state.outputDirectory, folder);
    return;
  }

  const Path directory(folder);
  const ANSICHAR* configurationName =
      kConfigurationNames[static_cast<SIZE_T>(state.configuration)];
  state.buildMessage.clear();
  if (!ProjectPackager::isGameBinaryDirectory(directory)) {
    state.buildMessage = folder + " holds no " +
                         String(ProjectPackager::GAME_EXECUTABLE_NAME) +
                         String(ProjectPackager::EXECUTABLE_EXTENSION) + ".";
  }
  else if (!isBuildOf(directory, state.configuration)) {
    state.buildMessage = folder + " is not a " + configurationName + " build.";
  }
  else {
    state.gameBuild = directory;
    remember(kBuildDirectoryKeys[static_cast<SIZE_T>(state.configuration)], folder);
    return;
  }
  CH_LOG_WARNING(PackageDialogLog, "{0}", state.buildMessage);
}

void
finishWithError(DialogState& state, String error)
{
  CH_LOG_ERROR(PackageDialogLog, "Package failed: {0}", error);
  state.result = PackageResult{};
  state.result.error = std::move(error);
  state.bJobDone.store(true, std::memory_order_release);
}

/*
 * Finds the assets on the main thread (the asset manager is not thread safe), then copies
 * them on a worker so the editor keeps drawing.
 */
void
startPackage(DialogState& state, const Path& binaryDirectory, bool bDevelopment)
{
  state.bJobShown = true;
  state.bJobAppearing = true;
  state.bJobDone.store(false, std::memory_order_relaxed);
  state.resultTime = -1.0;
  state.missingAssetCount = 0;
  state.jobName = state.gameName;
  state.progress = chMakeUnique<PackageProgress>();

  String folderName(state.gameName);
  const SIZE_T configurationIndex = static_cast<SIZE_T>(state.configuration);
  folderName +=
      bDevelopment ? kDevelopmentSuffix : kConfigurationSuffixes[configurationIndex];
  const Path outputParent = FileSystem::absolutePath(Path(String(state.outputDirectory)));
  state.jobDirectory = outputParent.join(Path(std::move(folderName)));

  AssetManager& assetManager = AssetManager::instance();
  Vector<UUID> roots;
  for (const SceneEntry& scene : state.scenes) {
    if (scene.path != state.startupScene && !scene.bIncluded) {
      continue;
    }
    const SPtr<SceneAsset> asset = assetManager.findScene(scene.path);
    if (!asset) {
      continue;
    }
    // The startup scene goes first, the game opens it.
    if (scene.path == state.startupScene) {
      roots.insert(roots.begin(), asset->getUUID());
    }
    else {
      roots.push_back(asset->getUUID());
    }
  }
  if (roots.empty() || !assetManager.findScene(state.startupScene)) {
    finishWithError(state, "There is no scene " + state.startupScene + ".");
    return;
  }
  saveScenes(state);

  Vector<UUID> assets;
  Vector<UUID> missing;
  ProjectPackager::collectAssets(
      roots,
      [&assetManager](const UUID& uuid) -> const Vector<UUID>* {
        const SPtr<IAsset> asset = assetManager.getAsset(uuid);
        return asset ? &asset->getReferencedAssets() : nullptr;
      },
      assets, missing);
  for (const UUID& uuid : missing) {
    CH_LOG_WARNING(PackageDialogLog, "A packaged asset references {0}, which does not exist.",
                   uuid);
  }
  state.missingAssetCount = static_cast<uint32>(missing.size());

  PackageDesc desc{.gameName = state.gameName,
                   .outputDirectory = state.jobDirectory,
                   .binaryDirectory = binaryDirectory,
                   .engineContentDirectory = EnginePaths::getEngineContentDirectory(),
                   .projectDirectory = EnginePaths::getProjectDirectory(),
                   .startupScene = state.startupScene};
  desc.assetFiles.reserve(assets.size());
  for (const UUID& uuid : assets) {
    const SPtr<IAsset> asset = assetManager.getAsset(uuid);
    const Path assetFile = FileSystem::absolutePath(asset->getFilePath());
    // A plain file (a script) keeps its UUID in its .meta file, so the game needs it too.
    if (asset->isPlainFile()) {
      desc.assetFiles.push_back(AssetMeta::getPath(assetFile));
    }
    desc.assetFiles.push_back(assetFile);
  }

  // Kept only when it is not the default, so the default follows the project.
  remember(kOutputDirectoryKey, outputParent == getDefaultOutputParent()
                                    ? StringView()
                                    : StringView(outputParent.toString()));

  state.worker = Thread([&state, desc = std::move(desc)]() {
    state.result = ProjectPackager::package(desc, state.progress.get());
    if (!state.result.bSucceeded && !state.result.bCancelled) {
      CH_LOG_ERROR(PackageDialogLog, "Package failed: {0}", state.result.error);
    }
    state.bJobDone.store(true, std::memory_order_release);
  });
}

void
endJob(DialogState& state)
{
  if (state.worker.joinable()) {
    state.worker.join();
  }
  state.progress.reset();
  state.bJobShown = false;
}

void
openFolder(const Path& directory)
{
#if USING(CH_DISPLAY_SDL3)
  const String url = "file:///" + directory.toString();
  if (!SDL_OpenURL(url.c_str())) {
    CH_LOG_ERROR(PackageDialogLog, "Could not open {0}: {1}", url, SDL_GetError());
  }
#else
  CH_PARAMETER_UNUSED(directory);
#endif // USING(CH_DISPLAY_SDL3)
}

/*
 * In the bottom right corner like the import progress, while the copy runs and for a
 * moment after.
 */
void
renderJobWindow(DialogState& state)
{
  if (!state.bJobShown) {
    return;
  }
  const bool bDone = state.bJobDone.load(std::memory_order_acquire);
  if (bDone && state.worker.joinable()) {
    state.worker.join();
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 corner(viewport->WorkPos.x + viewport->WorkSize.x,
                      viewport->WorkPos.y + viewport->WorkSize.y);
  const float margin = ImGui::GetFontSize();
  ImGui::SetNextWindowPos(ImVec2(corner.x - margin, corner.y - margin), ImGuiCond_Appearing,
                          ImVec2(1.0f, 1.0f));
  const float width = ImGui::GetFontSize() * 22.0f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, FLT_MAX));
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoDocking |
                                      ImGuiWindowFlags_NoSavedSettings;
  bool bClose = false;
  if (state.bJobAppearing) {
    ImGui::SetNextWindowFocus();
    state.bJobAppearing = false;
  }
  if (ImGui::Begin(ICON_FA_BOX_OPEN "  Packaging###PackageProgress", nullptr, kFlags)) {
    ImGui::TextUnformatted(state.jobName.c_str());
    const ImVec2 barSize(-FLT_MIN, 0.0f);
    if (!bDone) {
      const PackageProgress& progress = *state.progress;
      const uint32 total = progress.totalFiles.load(std::memory_order_relaxed);
      const uint32 copied = progress.copiedFiles.load(std::memory_order_relaxed);
      if (total == 0) {
        ImGui::TextDisabled("Listing files...");
        ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), barSize, "");
      }
      else {
        ImGui::TextDisabled("Copying file %u of %u", copied, total);
        ImGui::ProgressBar(static_cast<float>(copied) / static_cast<float>(total), barSize);
      }
      const bool bCancelling = state.progress->bCancelRequested.load();
      ImGui::BeginDisabled(bCancelling);
      if (ImGui::Button(bCancelling ? "Cancelling..." : "Cancel")) {
        state.progress->bCancelRequested.store(true);
      }
      ImGui::EndDisabled();
    }
    else {
      if (state.resultTime < 0.0) {
        state.resultTime = ImGui::GetTime();
      }
      const PackageResult& result = state.result;
      if (result.bSucceeded) {
        ImGui::PushStyleColor(ImGuiCol_Text, kSuccessColor);
        ImGui::TextWrapped(ICON_FA_CHECK_CIRCLE "  Done: %u assets and %u binary files.",
                           result.assetFileCount, result.binaryFileCount);
        ImGui::PopStyleColor();
        ImGui::TextWrapped("%s", state.jobDirectory.toString().c_str());
        if (state.missingAssetCount > 0) {
          ImGui::PushStyleColor(ImGuiCol_Text, kWarningColor);
          ImGui::TextWrapped("%u referenced assets do not exist; see the Output Log.",
                             state.missingAssetCount);
          ImGui::PopStyleColor();
        }
        if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Open Folder")) {
          openFolder(state.jobDirectory);
        }
        ImGui::SameLine();
      }
      else if (result.bCancelled) {
        ImGui::TextDisabled("Cancelled; nothing was kept.");
      }
      else {
        ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
        ImGui::TextWrapped(ICON_FA_TIMES_CIRCLE "  %s", result.error.c_str());
        ImGui::PopStyleColor();
      }
      bClose = ImGui::Button("Close");

      // A failure waits for the user; a success or a cancel goes away on its own.
      if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        state.resultTime = ImGui::GetTime();
      }
      else if (!result.error.empty() && !result.bCancelled) {
        state.resultTime = ImGui::GetTime();
      }
      bClose = bClose || ImGui::GetTime() - state.resultTime > kResultSeconds;
    }
  }
  ImGui::End();
  if (bClose) {
    endJob(state);
  }
}

void
renderStartupSceneCombo(DialogState& state)
{
  if (!ImGui::BeginCombo("Startup scene", state.startupScene.c_str())) {
    return;
  }
  for (const SceneEntry& scene : state.scenes) {
    if (ImGui::Selectable(scene.path.c_str(), scene.path == state.startupScene)) {
      state.startupScene = scene.path;
    }
  }
  ImGui::EndCombo();
}

void
renderSceneList(DialogState& state)
{
  ImGui::SeparatorText("Scenes");
  ImGui::TextDisabled("Each ticked scene goes in with everything it uses.");
  const SIZE_T rows = Math::min(Math::max(state.scenes.size(), SIZE_T(1)), kMaxSceneRows);
  const float height = static_cast<float>(rows) * ImGui::GetFrameHeightWithSpacing() +
                       ImGui::GetStyle().WindowPadding.y * 2.0f;
  if (ImGui::BeginChild("##Scenes", ImVec2(0.0f, height), ImGuiChildFlags_Borders)) {
    if (state.scenes.empty()) {
      ImGui::TextDisabled("The project has no scenes yet.");
    }
    for (SceneEntry& scene : state.scenes) {
      const bool bStartup = scene.path == state.startupScene;
      ImGui::BeginDisabled(bStartup);
      bool bChecked = scene.bIncluded || bStartup;
      if (ImGui::Checkbox(scene.path.c_str(), &bChecked)) {
        scene.bIncluded = bChecked;
      }
      ImGui::EndDisabled();
      if (bStartup) {
        ImGui::SameLine();
        ImGui::TextDisabled("(startup)");
      }
    }
  }
  ImGui::EndChild();
}

void
renderBuildSection(DialogState& state)
{
  ImGui::SeparatorText("Game runtime");
  const SIZE_T configurationIndex = static_cast<SIZE_T>(state.configuration);
  if (ImGui::BeginCombo("Configuration", kConfigurationNames[configurationIndex])) {
    for (SIZE_T i = 0; i < std::size(kConfigurationNames); ++i) {
      if (ImGui::Selectable(kConfigurationNames[i], i == configurationIndex)) {
        state.configuration = static_cast<PackageConfiguration>(i);
        state.gameBuild = findGameBuild(state.configuration);
        state.buildMessage.clear();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SetItemTooltip("Release for players. Debug keeps checks and is slower; to step "
                        "through the game, debug chGame from the IDE instead.");

  if (!state.gameBuild.empty()) {
    ImGui::TextColored(kSuccessColor, ICON_FA_CHECK_CIRCLE "  %s runtime found",
                       kConfigurationNames[configurationIndex]);
    ImGui::SetItemTooltip("chGame built without the editor, copied into the package and "
                          "renamed to the game:\n%s",
                          state.gameBuild.toString().c_str());
  }
  else {
    ImGui::PushStyleColor(ImGuiCol_Text, kWarningColor);
    ImGui::TextWrapped(ICON_FA_EXCLAMATION_TRIANGLE
                       "  No %s runtime was found. The runtime is chGame built without the "
                       "editor; the editor only copies it. Build it from the engine folder "
                       "with:",
                       kConfigurationNames[configurationIndex]);
    ImGui::PopStyleColor();
    ImGui::Indent();
    ImGui::TextUnformatted(kBuildCommands[configurationIndex]);
    ImGui::Unindent();
    if (ImGui::Button(ICON_FA_COPY "  Copy Commands")) {
      ImGui::SetClipboardText(kBuildCommands[configurationIndex]);
    }
    ImGui::SameLine();
    if (ImGui::Button("Look Again")) {
      state.gameBuild = findGameBuild(state.configuration);
    }
  }

  // Only for a runtime built somewhere the game presets do not write.
  if (ImGui::TreeNode("Advanced")) {
    if (!state.gameBuild.empty()) {
      ImGui::TextWrapped("%s", state.gameBuild.toString().c_str());
    }
    if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Choose Runtime Folder...")) {
      browseFolder(state, BrowseTarget::Build,
                   state.gameBuild.empty() ? EnginePaths::getEngineRootDirectory()
                                           : state.gameBuild);
    }
    if (!state.buildMessage.empty()) {
      ImGui::TextColored(kErrorColor, "%s", state.buildMessage.c_str());
    }
    ImGui::TreePop();
  }
}
} // namespace

/*
 */
void
PackageDialog::open()
{
  DialogState& state = getState();
  if (state.bJobShown && !state.bJobDone.load(std::memory_order_acquire)) {
    CH_LOG_WARNING(PackageDialogLog, "A package is being written; wait for it to finish.");
    return;
  }
  StringUtils::copyToBuffer(state.gameName, getProjectName());
  StringUtils::copyToBuffer(state.outputDirectory, getOutputParent().toString());
  state.startupScene = getDefaultStartupScene();
  loadScenes(state);
  state.configuration = PackageConfiguration::Release;
  state.gameBuild = findGameBuild(state.configuration);
  state.buildMessage.clear();
  const Path& editorFolder = EnginePaths::getPluginDirectory();
  state.editorBuild =
      ProjectPackager::isGameBinaryDirectory(editorFolder) ? editorFolder : Path();
  state.bOpenRequested = true;
}

/*
 */
bool
PackageDialog::isPackaging()
{
  const DialogState& state = getState();
  return state.bJobShown && !state.bJobDone.load(std::memory_order_acquire);
}

/*
 */
void
PackageDialog::render()
{
  DialogState& state = getState();
  takeChosenFolder(state);
  renderJobWindow(state);
  if (state.bOpenRequested) {
    ImGui::OpenPopup(kPopupName);
    state.bOpenRequested = false;
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  const float width = ImGui::GetFontSize() * 40.0f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, FLT_MAX));
  bool bOpen = true;
  if (!ImGui::BeginPopupModal(kPopupName, &bOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::InputText("Game name", state.gameName, sizeof(state.gameName));
  renderStartupSceneCombo(state);
  ImGui::InputText("##Output", state.outputDirectory, sizeof(state.outputDirectory));
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FOLDER_OPEN "##BrowseOutput")) {
    browseFolder(state, BrowseTarget::Output, Path(String(state.outputDirectory)));
  }
  ImGui::SameLine();
  ImGui::TextUnformatted("Output folder");
  ImGui::TextDisabled("The package goes into a subfolder named after the game.");

  renderSceneList(state);
  renderBuildSection(state);

  if (EditorScene::isDirty()) {
    ImGui::PushStyleColor(ImGuiCol_Text, kWarningColor);
    ImGui::TextWrapped(ICON_FA_EXCLAMATION_TRIANGLE "  The open scene has unsaved changes; "
                                                    "the package uses what is saved.");
    ImGui::PopStyleColor();
  }

  ImGui::Separator();
  const bool bValidName = ProjectPackager::isValidGameName(state.gameName);
  if (!bValidName) {
    ImGui::TextColored(kErrorColor, "The game name cannot be used as a file name.");
  }
  ImGui::BeginDisabled(!bValidName || state.browseTarget != BrowseTarget::None ||
                       isPackaging());
  if (!state.gameBuild.empty()) {
    if (ImGui::Button(ICON_FA_BOX_OPEN "  Package")) {
      endJob(state);
      startPackage(state, state.gameBuild, false);
      ImGui::CloseCurrentPopup();
    }
  }
  else {
    ImGui::BeginDisabled(state.editorBuild.empty());
    if (ImGui::Button(ICON_FA_BOX_OPEN "  Package for Development")) {
      endJob(state);
      startPackage(state, state.editorBuild, true);
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Copies the editor's own binaries: the game runs, but carries "
                          "editor code and the profiler. Not for players.");
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Close")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}

/*
 */
void
PackageDialog::shutDown()
{
  DialogState& state = getState();
  if (state.worker.joinable()) {
    state.progress->bCancelRequested.store(true);
    state.worker.join();
  }
  state.progress.reset();
}

} // namespace chEngineSDK
