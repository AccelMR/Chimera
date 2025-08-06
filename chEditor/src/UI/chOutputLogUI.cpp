/************************************************************************/
/**
 * @file chOutputLogUI.cpp
 * @author AccelMR
 * @date 2025/07/28
 * @brief Implementation of OutputLogUI class
 */
/************************************************************************/
#include "chOutputLogUI.h"

#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(OutputLogUILog, All);

/*
 */
OutputLogUI::OutputLogUI()
 : m_logWrittenEvent(Logger::instance().onLogWritten(
       std::bind(&OutputLogUI::addLogEntry, this, std::placeholders::_1))) {
  CH_LOG_DEBUG(OutputLogUILog, "Creating OutputLogUI instance.");
  // Initialize with all categories enabled by default
  m_filter.enabledCategories.clear();

  appendLogEntries(Logger::instance().getBufferedLogs());
}
/*
 */
OutputLogUI::~OutputLogUI() {
  m_logWrittenEvent.disconnect();
  // Optional: Brief wait to ensure no in-flight calls
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

/*
 */
void
OutputLogUI::renderOutputLogUI() {
  if (!m_isVisible) {
    return;
  }

  if (!ImGui::Begin("Output Log", &m_isVisible)) {
    ImGui::End();
    return;
  }

  renderFilterControls();
  ImGui::Separator();
  renderLogEntries();

  ImGui::End();
}

/*
 */
void
OutputLogUI::renderFilterControls() {
  // Verbosity level filters
  ImGui::Text("Verbosity Levels:");
  ImGui::SameLine();

  bool filterChanged = false;
  bool* levels[] = { &m_filter.showDebug, &m_filter.showInfo, &m_filter.showWarning, &m_filter.showError, &m_filter.showFatal };
  static const ANSICHAR* labels[] = { "Debug", "Info", "Warning", "Error", "Fatal" };

  const SIZE_T numLevels = sizeof(labels) / sizeof(labels[0]);
  for (SIZE_T i = 0; i < numLevels; ++i) {
    if (ImGui::Checkbox(labels[i], levels[i])) {
      filterChanged = true;
    }
    if (i < numLevels - 1) {
      ImGui::SameLine();
    }
  }

  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const float searchWidth = availableWidth  * 0.4f; // 40% of available width

  ImGui::SetNextItemWidth(searchWidth);
  if (ImGui::InputTextWithHint("##search",
                               "Search logs...",
                               m_searchBuffer,
                               sizeof(m_searchBuffer))) {
    m_filter.searchText = String(m_searchBuffer);
    filterChanged = true;
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("Auto-scroll", &m_autoScroll)) {
    if (m_autoScroll) {
      m_needsScrollToBottom = true;
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    clearLog();
  }

  if (filterChanged) {
    m_needsFilterUpdate = true;
  }

  if (m_availableCategories.empty()) {
    ImGui::Text("No categories available.");
    return;
  }

  ImGui::Text("Categories:");
  ImGui::SameLine();

  // Create a combo box for category selection
  String previewText = std::format("({} selected)", m_filter.enabledCategories.size());

  if (ImGui::BeginCombo("##CategoryFilter", previewText.c_str())) {
    // All/None buttons at the top
    if (ImGui::Selectable("Select All")) {
      for (const auto& category : m_availableCategories) {
        m_filter.enabledCategories.insert(category);
      }
      filterChanged = true;
    }

    if (ImGui::Selectable("Select None")) {
      m_filter.enabledCategories.clear();
      filterChanged = true;
    }

    ImGui::Separator();

    // Individual category checkboxes
    for (const auto& category : m_availableCategories) {
      bool isEnabled =
          m_filter.enabledCategories.find(category) != m_filter.enabledCategories.end();

      if (ImGui::Checkbox(category.c_str(), &isEnabled)) {
        if (isEnabled) {
          m_filter.enabledCategories.insert(category);
        }
        else {
          m_filter.enabledCategories.erase(category);
        }
        filterChanged = true;
      }
    }

    m_needsFilterUpdate = filterChanged;

    ImGui::EndCombo();
  }
  ImGui::NewLine();
}

/*
 */
void
OutputLogUI::renderLogEntries() {
  // Update filtered entries if needed
  if (m_needsFilterUpdate) {
    m_filteredEntries.clear();

    for (const auto& entry : m_logEntries) {
      if (m_filter.passesFilter(entry)) {
        m_filteredEntries.push_back(entry);
      }
    }

    m_needsFilterUpdate = false;
    if (m_autoScroll) {
      m_needsScrollToBottom = true;
    }
  }

  // Log entries table
  if (ImGui::BeginTable("LogTable", 5,
                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {

    // Setup columns
    ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    // Render log entries
    for (int32 i = 0; i < static_cast<int32>(m_filteredEntries.size()); ++i) {
      renderLogEntryRow(m_filteredEntries[i], i);
    }

    // Auto-scroll to bottom
    if (m_needsScrollToBottom && m_autoScroll) {
      ImGui::SetScrollHereY(1.0f);
      m_needsScrollToBottom = false;
    }

    ImGui::EndTable();
  }
}

/*
 */
void
OutputLogUI::renderLogEntryRow(const LogBufferEntry& entry, int32) {
  ImGui::TableNextRow();

  // Set row color based on verbosity
  ImVec4 color = getVerbosityColor(entry.verbosity);
  ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(ImVec4(
                                                        color.x, color.y, color.z, 0.3f)));

  // Level column with icon
  ImGui::TableSetColumnIndex(0);
  ImGui::TextColored(color, "%s", getVerbosityIcon(entry.verbosity));

  // Time column
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%s", entry.timestamp.c_str());

  // Category column
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%s", entry.category.c_str());

  // Message column
  ImGui::TableSetColumnIndex(3);
  ImGui::TextWrapped("%s", entry.message.c_str());

  // Source column
  ImGui::TableSetColumnIndex(4);
  if (!entry.sourceFile.empty()) {
    ImGui::Text("%s:%d", entry.sourceFile.c_str(), entry.sourceLine);
  }
}

/*
 */
void
OutputLogUI::addLogEntry(const LogBufferEntry& entry) {
  m_logEntries.push_back(entry);

  // Update available categories
  if (m_availableCategories.find(entry.category) == m_availableCategories.end()) {
    m_availableCategories.insert(entry.category);
    // Auto-enable new categories
    m_filter.enabledCategories.insert(entry.category);
  }

  applySizeLimits();
  m_needsFilterUpdate = true;

  if (m_autoScroll) {
    m_needsScrollToBottom = true;
  }
}

/*
 */
void
OutputLogUI::clearLog() {
  m_logEntries.clear();
  m_filteredEntries.clear();
  m_availableCategories.clear();
  m_filter.enabledCategories.clear();
  m_needsFilterUpdate = true;

  CH_LOG_DEBUG(OutputLogUILog, "Log cleared.");
}

/*
 */
void
OutputLogUI::updateAvailableCategories() {
  m_availableCategories.clear();

  for (const auto& entry : m_logEntries) {
    m_availableCategories.insert(entry.category);
  }
}

/*
 */
ImVec4
OutputLogUI::getVerbosityColor(LogVerbosity verbosity) const {
  switch (verbosity) {
  case LogVerbosity::Fatal:
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
  case LogVerbosity::Error:
    return ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
  case LogVerbosity::Warning:
    return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
  case LogVerbosity::Info:
    return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
  case LogVerbosity::Debug:
    return ImVec4(0.0f, 0.8f, 1.0f, 1.0f); // Light Blue
  default:
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
  }
}

/*
 */
const char*
OutputLogUI::getVerbosityIcon(LogVerbosity verbosity) const {
  switch (verbosity) {
  case LogVerbosity::Debug:
    return "DBG";
  case LogVerbosity::Info:
    return "INF";
  case LogVerbosity::Warning:
    return "WRN";
  case LogVerbosity::Error:
    return "ERR";
  case LogVerbosity::Fatal:
    return "FTL";
  default:
    return "UNK";
  }
}

/*
 */
void
OutputLogUI::applySizeLimits() {
  if (m_logEntries.size() > m_maxLogEntries) {
    uint32 toRemove = static_cast<uint32>(m_logEntries.size()) - m_maxLogEntries;
    m_logEntries.erase(m_logEntries.begin(), m_logEntries.begin() + toRemove);
    m_needsFilterUpdate = true;
  }
}

/*
 */
bool
OutputLogUI::LogFilter::passesFilter(const LogBufferEntry& entry) const {
  // Check verbosity filter
  switch (entry.verbosity) {
  case LogVerbosity::Debug:
    if (!showDebug) {
      return false;
    }
    break;
  case LogVerbosity::Info:
    if (!showInfo) {
      return false;
    }
    break;
  case LogVerbosity::Warning:
    if (!showWarning) {
      return false;
    }
    break;
  case LogVerbosity::Error:
    if (!showError) {
      return false;
    }
    break;
  case LogVerbosity::Fatal:
    if (!showFatal) {
      return false;
    }
    break;
  case LogVerbosity::NoLogging:
    if (!showTrace) {
      return false;
    }
    break;
  }

  if (enabledCategories.empty()) {
    return false; // No filters applied
  }

  // Check category filter
  if (!enabledCategories.empty() &&
      enabledCategories.find(entry.category) == enabledCategories.end()) {
    return false;
  }

  // Check search filter
  if (!searchText.empty()) {
    String searchLower = searchText;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    String messageLower = entry.message;
    std::transform(messageLower.begin(), messageLower.end(), messageLower.begin(), ::tolower);

    String categoryLower = entry.category;
    std::transform(categoryLower.begin(), categoryLower.end(), categoryLower.begin(),
                   ::tolower);

    if (messageLower.find(searchLower) == String::npos &&
        categoryLower.find(searchLower) == String::npos) {
      return false;
    }
  }

  return true;
}

} // namespace chEngineSDK
