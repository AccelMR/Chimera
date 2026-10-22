/************************************************************************/
/**
 * @file chProjectBrowserUI.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Lists the recent projects and makes or opens one.
 */
/************************************************************************/
#include "chProjectBrowserUI.h"

#include "chCommandLine.h"
#include "chConfigFile.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMath.h"
#include "chProjectDescriptor.h"
#include "chSTDThreading.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

#if USING(CH_DISPLAY_SDL3)
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ProjectBrowserLog, All);

namespace {
constexpr const ANSICHAR* kPopupName = ICON_FA_FOLDER_OPEN "  Projects###ProjectBrowser";
constexpr const ANSICHAR* kRecentFile = "Config/RecentProjects.ini";
constexpr const ANSICHAR* kRecentSection = "Recent";
constexpr const ANSICHAR* kRecentKeyPrefix = "Project";
constexpr const ANSICHAR* kNewProjectSection = "NewProject";
constexpr const ANSICHAR* kLocationKey = "Location";
constexpr const ANSICHAR* kDefaultProjectsFolder = "Projects";
constexpr const ANSICHAR* kProjectOption = "project";
constexpr SIZE_T kMaxRecentProjects = 10;
constexpr SIZE_T kPathBufferSize = 1024;
constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);

enum class BrowseTarget : uint8
{
  None,
  ProjectFile,
  Location
};

struct RecentProject
{
  Path file;
  String name;
  bool bExists = false;
};

struct BrowserState
{
  bool bOpenRequested = false;
  // The recent list is read from disk when the browser opens, never per frame.
  bool bLoaded = false;
  Vector<RecentProject> recent;
  SIZE_T selected = 0;
  ANSICHAR newName[128]{};
  ANSICHAR newLocation[kPathBufferSize]{};
  String error;

  Path chosenProject;
  bool bProjectChosen = false;

  // The file dialogs keep pointers to these until they answer.
  String defaultLocation;
#if USING(CH_DISPLAY_SDL3)
  SDL_DialogFileFilter filter{"Chimera project", "chproject"};
#endif // USING(CH_DISPLAY_SDL3)
  BrowseTarget browseTarget = BrowseTarget::None;
  // Written by the dialog's callback, which may run on another thread; chosenPath is only
  // read after bPathChosen is seen true.
  String chosenPath;
  Atomic<bool> bPathChosen{false};
};

BrowserState&
getState()
{
  static BrowserState state;
  return state;
}

Path
getRecentFilePath()
{
  return EnginePaths::getEditorUserDirectory().join(Path(kRecentFile));
}

Vector<Path>
readRecentFiles(const ConfigFile& config)
{
  Vector<Path> files;
  for (SIZE_T i = 0; i < kMaxRecentProjects; ++i) {
    const String key = kRecentKeyPrefix + StringUtils::toString(i + 1);
    if (const String* value = config.getValue(kRecentSection, key)) {
      if (!value->empty()) {
        files.emplace_back(*value);
      }
    }
  }
  return files;
}

void
loadRecent(BrowserState& state)
{
  state.bLoaded = true;
  state.recent.clear();
  state.selected = 0;
  state.error.clear();

  ConfigFile config;
  config.load(getRecentFilePath());
  for (Path& file : readRecentFiles(config)) {
    RecentProject project;
    ProjectDescriptor descriptor;
    project.bExists = FileSystem::isFile(file) && descriptor.load(file);
    project.name = project.bExists ? descriptor.name : file.getFileName(false);
    project.file = std::move(file);
    state.recent.push_back(std::move(project));
  }

  const String* location = config.getValue(kNewProjectSection, kLocationKey);
  const String defaultLocation =
      (location && !location->empty())
          ? *location
          : EnginePaths::getEngineRootDirectory().join(Path(kDefaultProjectsFolder)).toString();
  StringUtils::copyToBuffer(state.newLocation, defaultLocation);
}

void
saveConfig(const ConfigFile& config)
{
  if (!config.save(getRecentFilePath())) {
    CH_LOG_WARNING(ProjectBrowserLog, "Could not write {0}", getRecentFilePath());
  }
}

void
saveRecent(const Vector<Path>& files)
{
  ConfigFile config;
  config.load(getRecentFilePath());
  for (SIZE_T i = 0; i < kMaxRecentProjects; ++i) {
    const String key = kRecentKeyPrefix + StringUtils::toString(i + 1);
    config.setValue(kRecentSection, key, i < files.size() ? files[i].toString() : String());
  }
  saveConfig(config);
}

void
saveNewProjectLocation(StringView location)
{
  ConfigFile config;
  config.load(getRecentFilePath());
  config.setValue(kNewProjectSection, kLocationKey, location);
  saveConfig(config);
}

void
choose(BrowserState& state, const Path& projectFile)
{
  ProjectDescriptor descriptor;
  if (!FileSystem::isFile(projectFile) || !descriptor.load(projectFile)) {
    state.error = "Could not read " + projectFile.toString() + ".";
    return;
  }
  state.chosenProject = projectFile;
  state.bProjectChosen = true;
}

void
removeRecent(BrowserState& state, SIZE_T index)
{
  state.recent.erase(state.recent.begin() + static_cast<int64>(index));
  Vector<Path> files;
  files.reserve(state.recent.size());
  for (const RecentProject& project : state.recent) {
    files.push_back(project.file);
  }
  saveRecent(files);
  state.selected = 0;
}

#if USING(CH_DISPLAY_SDL3)
void SDLCALL
onPathChosen(void* userData, const ANSICHAR* const* fileList, int32 filter)
{
  CH_PARAMETER_UNUSED(userData);
  CH_PARAMETER_UNUSED(filter);
  BrowserState& state = getState();
  state.chosenPath.clear();
  if (!fileList) {
    // SDL keeps the error per thread, so it must be read here.
    CH_LOG_ERROR(ProjectBrowserLog, "Could not open the file dialog: {0}", SDL_GetError());
  }
  else if (fileList[0]) {
    state.chosenPath = fileList[0];
  }
  state.bPathChosen.store(true, std::memory_order_release);
}
#endif // USING(CH_DISPLAY_SDL3)

void
browse(BrowserState& state, BrowseTarget target, const Path& start)
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
  if (target == BrowseTarget::ProjectFile) {
    SDL_ShowOpenFileDialog(onPathChosen, nullptr, window, &state.filter, 1,
                           state.defaultLocation.c_str(), false);
  }
  else {
    SDL_ShowOpenFolderDialog(onPathChosen, nullptr, window, state.defaultLocation.c_str(),
                             false);
  }
#else
  CH_PARAMETER_UNUSED(target);
  CH_PARAMETER_UNUSED(start);
  CH_LOG_ERROR(ProjectBrowserLog, "There is no file dialog without SDL3.");
#endif // USING(CH_DISPLAY_SDL3)
}

void
takeChosenPath(BrowserState& state)
{
  if (state.browseTarget == BrowseTarget::None ||
      !state.bPathChosen.load(std::memory_order_acquire)) {
    return;
  }
  state.bPathChosen.store(false, std::memory_order_relaxed);
  const BrowseTarget target = state.browseTarget;
  state.browseTarget = BrowseTarget::None;
  if (state.chosenPath.empty()) {
    return;
  }

  const Path path(state.chosenPath);
  if (target == BrowseTarget::Location) {
    StringUtils::copyToBuffer(state.newLocation, path.toString());
  }
  else {
    choose(state, path);
  }
}

void
renderRecentList(BrowserState& state, float height)
{
  ImGui::SeparatorText(ICON_FA_HISTORY "  Recent Projects");
  if (!ImGui::BeginChild("##RecentProjects", ImVec2(0.0f, height), ImGuiChildFlags_Borders)) {
    ImGui::EndChild();
    return;
  }

  if (state.recent.empty()) {
    ImGui::TextDisabled("No recent projects. Open one or make a new one below.");
  }

  const ImGuiStyle& style = ImGui::GetStyle();
  const float lineHeight = ImGui::GetTextLineHeight();
  const float rowHeight = lineHeight * 2.0f + style.ItemSpacing.y;
  SIZE_T removeIndex = state.recent.size();
  for (SIZE_T i = 0; i < state.recent.size(); ++i) {
    const RecentProject& project = state.recent[i];
    ImGui::PushID(static_cast<int32>(i));
    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
    if (ImGui::Selectable("##Row", state.selected == i, ImGuiSelectableFlags_AllowDoubleClick,
                          ImVec2(0.0f, rowHeight))) {
      state.selected = i;
      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && project.bExists) {
        choose(state, project.file);
      }
    }
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem(ICON_FA_TIMES "  Remove from List")) {
        removeIndex = i;
      }
      ImGui::EndPopup();
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 nameColor = ImGui::GetColorU32(project.bExists ? ImGuiCol_Text
                                                                : ImGuiCol_TextDisabled);
    drawList->AddText(ImVec2(rowStart.x + style.FramePadding.x, rowStart.y), nameColor,
                      project.name.c_str());
    const String detail =
        project.bExists ? project.file.toString() : project.file.toString() + "  (missing)";
    drawList->AddText(ImVec2(rowStart.x + style.FramePadding.x, rowStart.y + lineHeight),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled), detail.c_str());
    ImGui::PopID();
  }
  ImGui::EndChild();

  if (removeIndex < state.recent.size()) {
    removeRecent(state, removeIndex);
  }

  const bool bCanOpen =
      state.selected < state.recent.size() && state.recent[state.selected].bExists;
  ImGui::BeginDisabled(!bCanOpen);
  if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Open")) {
    choose(state, state.recent[state.selected].file);
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.browseTarget != BrowseTarget::None);
  if (ImGui::Button("Browse...")) {
    browse(state, BrowseTarget::ProjectFile, EnginePaths::getEngineRootDirectory());
  }
  ImGui::EndDisabled();
}

void
renderNewProject(BrowserState& state)
{
  ImGui::SeparatorText(ICON_FA_FOLDER_PLUS "  New Project");
  const float labelWidth = ImGui::CalcTextSize("Location").x + ImGui::GetStyle().ItemSpacing.x;
  const float browseWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 4;

  ImGui::SetNextItemWidth(-labelWidth);
  ImGui::InputText("Name", state.newName, sizeof(state.newName));
  ImGui::SetNextItemWidth(-(labelWidth + browseWidth + ImGui::GetStyle().ItemSpacing.x));
  ImGui::InputText("##Location", state.newLocation, sizeof(state.newLocation));
  ImGui::SameLine();
  ImGui::BeginDisabled(state.browseTarget != BrowseTarget::None);
  if (ImGui::Button("...", ImVec2(browseWidth, 0.0f))) {
    browse(state, BrowseTarget::Location, Path(state.newLocation));
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::TextUnformatted("Location");

  const StringView name(state.newName);
  const bool bValidName = ProjectDescriptor::isValidName(name);
  if (bValidName) {
    ImGui::TextDisabled("Makes %s/%s", state.newLocation, state.newName);
  }
  else {
    ImGui::TextDisabled("Name the project; it becomes its folder and file name.");
  }

  ImGui::BeginDisabled(!bValidName || state.newLocation[0] == '\0');
  if (ImGui::Button(ICON_FA_PLUS "  Create")) {
    Path projectFile;
    if (ProjectDescriptor::create(Path(state.newLocation), name, projectFile, state.error)) {
      CH_LOG_INFO(ProjectBrowserLog, "Created the project {0}", projectFile);
      saveNewProjectLocation(state.newLocation);
      ProjectBrowserUI::addRecentProject(projectFile);
      choose(state, projectFile);
    }
  }
  ImGui::EndDisabled();
}

void
renderContents(BrowserState& state, float listHeight)
{
  if (!state.bLoaded) {
    loadRecent(state);
  }
  takeChosenPath(state);

  renderRecentList(state, listHeight);
  ImGui::Spacing();
  renderNewProject(state);

  if (!state.error.empty()) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
    ImGui::TextWrapped(ICON_FA_EXCLAMATION_TRIANGLE "  %s", state.error.c_str());
    ImGui::PopStyleColor();
  }
}
} // namespace

/*
 */
void
ProjectBrowserUI::open()
{
  BrowserState& state = getState();
  state.bOpenRequested = true;
  state.bLoaded = false;
}

/*
 */
void
ProjectBrowserUI::renderPopup()
{
  BrowserState& state = getState();
  if (state.bOpenRequested) {
    state.bOpenRequested = false;
    ImGui::OpenPopup(kPopupName);
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  const float fontSize = ImGui::GetFontSize();
  ImGui::SetNextWindowSize(ImVec2(fontSize * 40.0f, 0.0f), ImGuiCond_Appearing);
  if (!ImGui::BeginPopupModal(kPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
    return;
  }

  renderContents(state, fontSize * 14.0f);
  ImGui::Separator();
  if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
      state.bProjectChosen) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}

/*
 */
void
ProjectBrowserUI::renderWindow()
{
  BrowserState& state = getState();
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus;
  if (ImGui::Begin("###ProjectBrowserWindow", nullptr, kFlags)) {
    ImGui::Text(ICON_FA_PROJECT_DIAGRAM "  Chimera %s", CH_ENGINE_VERSION_STRING);
    ImGui::TextDisabled("Pick a project to open, or make a new one.");
    ImGui::Spacing();

    // The new project block and the error take about ten lines under the list.
    const float listHeight = Math::max(ImGui::GetFontSize() * 6.0f,
                                       ImGui::GetContentRegionAvail().y -
                                           ImGui::GetFrameHeightWithSpacing() * 10.0f);
    renderContents(state, listHeight);
  }
  ImGui::End();
}

/*
 */
bool
ProjectBrowserUI::takeChosenProject(Path& outProjectFile)
{
  BrowserState& state = getState();
  if (!state.bProjectChosen) {
    return false;
  }
  state.bProjectChosen = false;
  outProjectFile = std::move(state.chosenProject);
  state.chosenProject = Path();
  return true;
}

/*
 */
void
ProjectBrowserUI::addRecentProject(const Path& projectFile)
{
  const Path file = FileSystem::absolutePath(projectFile);
  ConfigFile config;
  config.load(getRecentFilePath());
  Vector<Path> files = readRecentFiles(config);
  for (SIZE_T i = 0; i < files.size(); ++i) {
    if (files[i] == file) {
      files.erase(files.begin() + static_cast<int64>(i));
      break;
    }
  }
  files.insert(files.begin(), file);
  if (files.size() > kMaxRecentProjects) {
    files.resize(kMaxRecentProjects);
  }
  saveRecent(files);
  getState().bLoaded = false;
}

/*
 */
Vector<String>
ProjectBrowserUI::getLaunchArguments(const Path& projectFile)
{
  Vector<String> arguments;
  const int32 argc = CommandLine::getArgc();
  const ANSICHAR* const* argv = CommandLine::getArgv();
  for (int32 i = 1; i < argc; ++i) {
    const StringView argument(argv[i]);
    const SIZE_T nameStart = argument.find_first_not_of('-');
    if (nameStart != 0 && nameStart != StringView::npos) {
      const SIZE_T equals = argument.find('=');
      const SIZE_T nameEnd = equals == StringView::npos ? argument.size() : equals;
      if (StringUtils::equalsIgnoreCase(argument.substr(nameStart, nameEnd - nameStart),
                                        kProjectOption)) {
        continue;
      }
    }
    arguments.emplace_back(argument);
  }
  arguments.push_back("-project=" + FileSystem::absolutePath(projectFile).toString());
  return arguments;
}

} // namespace chEngineSDK
