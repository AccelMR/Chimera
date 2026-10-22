/************************************************************************/
/**
 * @file chScriptDefinitionsUI.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Writes the script API for code editors when the project opens.
 */
/************************************************************************/
#include "chScriptDefinitionsUI.h"

#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chScriptApi.h"
#include "chScriptManager.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

#include <cfloat>
#include <cstring>

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ScriptDefinitionsLog, All);

namespace {
// Relative to the project folder, as .luarc.json names it.
constexpr const ANSICHAR* kDefinitionFolder = "Saved/ScriptDefinitions";
constexpr const ANSICHAR* kDefinitionFile = "/Saved/ScriptDefinitions/chimera.d.lua";
constexpr const ANSICHAR* kLuaLsConfigFile = ".luarc.json";
constexpr double kResultSeconds = 4.0;
constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);
constexpr ImVec4 kSuccessColor(0.5f, 0.9f, 0.5f, 1.0f);

enum class Step : uint8 {
  Idle,
  // The window shows its bar for one frame before the files are written.
  Pending,
  Writing,
  Done
};

struct DefinitionsState
{
  Step step = Step::Idle;
  bool bDefinitionsChanged = false;
  bool bConfigWritten = false;
  String error;
  double resultTime = -1.0;
};

DefinitionsState&
getState()
{
  static DefinitionsState state;
  return state;
}

/*
 * Writes text into path unless the file already holds it, so the date only changes with the
 * API and code editors do not read it again for nothing.
 */
bool
writeIfChanged(const Path& path, const String& text, bool& outChanged)
{
  outChanged = false;
  if (FileSystem::exists(path)) {
    const Vector<uint8> current = FileSystem::fastRead(path);
    if (current.size() == text.size() &&
        std::memcmp(current.data(), text.data(), text.size()) == 0) {
      return true;
    }
  }
  const SPtr<DataStream> stream = FileSystem::createAndOpenFile(path);
  if (!stream || stream->write(text.data(), text.size()) != text.size()) {
    return false;
  }
  outChanged = true;
  return true;
}

void
writeFiles(DefinitionsState& state)
{
  const ScriptApiDescription* description =
      ScriptManager::isStarted() ? ScriptManager::instance().getApiDescription() : nullptr;
  if (!description) {
    state.error = "The script runtime is not running.";
    return;
  }
  const Path definitionFile(kDefinitionFile);
  if (!writeIfChanged(definitionFile, description->writeDefinitions(),
                      state.bDefinitionsChanged)) {
    state.error = "Could not write " + definitionFile.toString() + ".";
    CH_LOG_ERROR(ScriptDefinitionsLog, "{0}", state.error);
    return;
  }
  if (state.bDefinitionsChanged) {
    CH_LOG_INFO(ScriptDefinitionsLog, "Script definitions written to {0}.", definitionFile);
  }

  // The project's own configuration wins: it is only written when missing.
  const Path configFile = EnginePaths::getProjectDirectory().join(Path(kLuaLsConfigFile));
  if (!FileSystem::exists(configFile)) {
    bool bChanged = false;
    if (!writeIfChanged(configFile, ScriptApiDescription::writeLuaLsConfig(kDefinitionFolder),
                        bChanged)) {
      state.error = "Could not write " + configFile.toString() + ".";
      CH_LOG_ERROR(ScriptDefinitionsLog, "{0}", state.error);
      return;
    }
    state.bConfigWritten = true;
    CH_LOG_INFO(ScriptDefinitionsLog, "Wrote {0} for code editors.", configFile);
  }
}
} // namespace

/*
 */
void
ScriptDefinitionsUI::requestUpdate()
{
  DefinitionsState& state = getState();
  state = DefinitionsState();
  if (EnginePaths::hasProject()) {
    state.step = Step::Pending;
  }
}

/*
 */
void
ScriptDefinitionsUI::render()
{
  DefinitionsState& state = getState();
  if (state.step == Step::Idle) {
    return;
  }
  if (state.step == Step::Writing) {
    writeFiles(state);
    state.step = Step::Done;
    state.resultTime = ImGui::GetTime();
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float margin = ImGui::GetFontSize();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - margin,
                                 viewport->WorkPos.y + viewport->WorkSize.y - margin),
                          ImGuiCond_Always, ImVec2(1.0f, 1.0f));
  const float width = ImGui::GetFontSize() * 22.0f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, FLT_MAX));
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoDocking |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoFocusOnAppearing |
                                      ImGuiWindowFlags_NoMove;
  bool bClose = false;
  if (ImGui::Begin(ICON_FA_CODE "  Script Definitions###ScriptDefinitions", nullptr, kFlags)) {
    // Opens with the project, before the dock space exists, which would cover it; kept in
    // front without taking the keyboard from the panels.
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    if (state.step == Step::Pending) {
      ImGui::TextUnformatted("Writing the script API for code editors...");
      ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), ImVec2(-FLT_MIN, 0.0f), "");
      state.step = Step::Writing;
    }
    else if (!state.error.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
      ImGui::TextWrapped(ICON_FA_TIMES_CIRCLE "  %s", state.error.c_str());
      ImGui::PopStyleColor();
      bClose = ImGui::Button("Close");
    }
    else {
      ImGui::PushStyleColor(ImGuiCol_Text, kSuccessColor);
      ImGui::TextWrapped(ICON_FA_CHECK_CIRCLE "  %s",
                         state.bDefinitionsChanged ? "Script API written."
                                                   : "Script API is up to date.");
      ImGui::PopStyleColor();
      ImGui::TextDisabled("%s", kDefinitionFolder);
      if (state.bConfigWritten) {
        ImGui::TextDisabled("%s created.", kLuaLsConfigFile);
      }
      ImGui::TextWrapped("Open the project folder in a code editor with the Lua language "
                         "server (LuaLS) to complete the engine API.");

      // Stays while the mouse is over it, then goes away on its own.
      if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        state.resultTime = ImGui::GetTime();
      }
      bClose = ImGui::GetTime() - state.resultTime > kResultSeconds;
    }
  }
  ImGui::End();
  if (bClose) {
    state.step = Step::Idle;
  }
}

} // namespace chEngineSDK
