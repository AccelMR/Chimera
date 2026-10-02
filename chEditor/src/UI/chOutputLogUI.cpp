/************************************************************************/
/**
 * @file chOutputLogUI.cpp
 * @author AccelMR
 * @date 2025/07/28
 * @brief Implementation of OutputLogUI class
 */
/************************************************************************/
#include "chOutputLogUI.h"

#include <algorithm>

#include "chMath.h"
#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(OutputLogUILog, All);

namespace {

NODISCARD ImVec4
getVerbosityColor(LogVerbosity verbosity) noexcept
{
  switch (verbosity) {
  case LogVerbosity::Fatal:
    return ImVec4(0.953f, 0.545f, 0.659f, 1.0f); // Red #f38ba8
  case LogVerbosity::Error:
    return ImVec4(0.980f, 0.702f, 0.529f, 1.0f); // Peach #fab387
  case LogVerbosity::Warning:
    return ImVec4(0.976f, 0.886f, 0.686f, 1.0f); // Yellow #f9e2af
  case LogVerbosity::Info:
    return ImVec4(0.651f, 0.890f, 0.631f, 1.0f); // Green #a6e3a1
  case LogVerbosity::Debug:
    return ImVec4(0.537f, 0.863f, 0.922f, 1.0f); // Sky #89dceb
  default:
    return ImVec4(0.804f, 0.839f, 0.957f, 1.0f); // Text #cdd6f4
  }
}

NODISCARD const ANSICHAR*
getVerbosityIcon(LogVerbosity verbosity) noexcept
{
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

} // namespace

/*
 */
OutputLogUI::OutputLogUI()
 : m_logWrittenEvent(Logger::instance().onLogWritten(
       [this](const SPtr<const LogBufferEntry>& entry) { addLogEntry(entry); },
       true))
{
  m_entries.reserve(m_maxLogEntries);
  m_filteredSequences.reserve(m_maxLogEntries);
  CH_LOG_DEBUG(OutputLogUILog, "Creating OutputLogUI instance.");
}

/*
 */
OutputLogUI::~OutputLogUI()
{
  if (Logger::isStarted()) {
    Logger::instance().disconnectLogListener(m_logWrittenEvent);
  }
}

/*
 */
void
OutputLogUI::renderOutputLogUI()
{
  // Flushed even while hidden so the queue does not keep growing.
  flushPendingEntries();

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
OutputLogUI::renderFilterControls()
{
  enum class ComboAction
  {
    RenderCheckboxes = -1,
    None = 0,
    All = 1
  };

  bool filterChanged = false;

  auto renderCombo = [&](const ANSICHAR* label,
                         const ANSICHAR* comboId,
                         const ANSICHAR* comboName,
                         auto renderContent) {
    ImGui::Text("%s:", label);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo(comboId, comboName)) {
      if (ImGui::Button("All")) {
        renderContent(ComboAction::All);
        filterChanged = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("None")) {
        renderContent(ComboAction::None);
        filterChanged = true;
      }
      ImGui::Separator();

      renderContent(ComboAction::RenderCheckboxes);
      ImGui::EndCombo();
    }
  };

  const bool bAllCategoriesOn =
      m_filter.enabledCategories.size() == m_availableCategories.size();
  const bool bAllCategoriesOff = m_filter.enabledCategories.empty();
  const ANSICHAR* categoryComboName =
      bAllCategoriesOn ? "All" : (bAllCategoriesOff ? "None" : "Mixed");

  const bool bAllVerbosityOn = m_filter.showDebug && m_filter.showInfo &&
                               m_filter.showWarning && m_filter.showError &&
                               m_filter.showFatal;
  const bool bAllVerbosityOff = !(m_filter.showDebug || m_filter.showInfo ||
                                  m_filter.showWarning || m_filter.showError ||
                                  m_filter.showFatal);
  const ANSICHAR* verbosityComboName =
      bAllVerbosityOn ? "All" : (bAllVerbosityOff ? "None" : "Mixed");

  renderCombo("Categories", "##CategoryCombo", categoryComboName, [&](ComboAction action) {
    if (action == ComboAction::All) {
      for (const auto& category : m_availableCategories) {
        m_filter.enabledCategories.insert(category);
      }
    }
    else if (action == ComboAction::None) {
      m_filter.enabledCategories.clear();
    }
    else {
      for (const auto& category : m_availableCategories) {
        bool isEnabled = m_filter.enabledCategories.count(category) > 0;
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
    }
  });

  ImGui::SameLine();

  renderCombo("Verbosity", "##VerbosityCombo", verbosityComboName, [&](ComboAction action) {
    static const ANSICHAR* labels[] = {"Debug", "Info", "Warning", "Error", "Fatal"};
    bool* levels[] = {&m_filter.showDebug, &m_filter.showInfo, &m_filter.showWarning,
                      &m_filter.showError, &m_filter.showFatal};
    static constexpr size_t numLevels = sizeof(labels) / sizeof(labels[0]);

    if (action == ComboAction::All) {
      for (size_t i = 0; i < numLevels; ++i) {
        *levels[i] = true;
      }
    }
    else if (action == ComboAction::None) {
      for (size_t i = 0; i < numLevels; ++i) {
        *levels[i] = false;
      }
    }
    else {
      for (size_t i = 0; i < numLevels; ++i) {
        if (ImGui::Checkbox(labels[i], levels[i])) {
          filterChanged = true;
        }
      }
    }
  });

  if (ImGui::InputTextWithHint("##search", "Search logs...", m_searchBuffer,
                               sizeof(m_searchBuffer))) {
    m_filter.searchText = m_searchBuffer;
    filterChanged = true;
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("Auto-scroll", &m_autoScroll) && m_autoScroll) {
    m_needsScrollToBottom = true;
  }

  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    clearLog();
  }

  if (filterChanged) {
    m_needsFilterUpdate = true;
  }
}

/*
 */
void
OutputLogUI::renderLogEntries()
{
  if (m_needsFilterUpdate) {
    rebuildFilteredEntries();
  }

  const bool hasSelection = NO_SELECTION != m_selectedSequence;
  const float detailHeight =
      hasSelection ? ImGui::GetTextLineHeightWithSpacing() * 8.0f : 0.0f;

  if (ImGui::BeginTable("LogTable", 5,
                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable,
                        ImVec2(0.0f, -detailHeight))) {
    ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    // Only the visible rows are drawn. The clipper needs every row to have the same
    // height, which is why each row shows the first line of its message.
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int32>(m_filteredSequences.size()));
    while (clipper.Step()) {
      for (int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
        renderLogEntryRow(m_filteredSequences[row]);
      }
    }

    if (m_needsScrollToBottom && m_autoScroll) {
      ImGui::SetScrollHereY(1.0f);
      m_needsScrollToBottom = false;
    }

    ImGui::EndTable();
  }

  if (hasSelection) {
    renderSelectedEntry();
  }
}

/*
 */
void
OutputLogUI::renderLogEntryRow(uint64 sequence)
{
  const LogBufferEntry& entry = getEntry(sequence);

  ImGui::TableNextRow();

  const ImVec4 color = getVerbosityColor(entry.verbosity);
  ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                         ImGui::ColorConvertFloat4ToU32(
                             ImVec4(color.x, color.y, color.z, 0.3f)));

  ImGui::TableSetColumnIndex(0);
  ImGui::PushID(static_cast<int32>(sequence));
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  const bool isSelected = sequence == m_selectedSequence;
  if (ImGui::Selectable(getVerbosityIcon(entry.verbosity), isSelected,
                        ImGuiSelectableFlags_SpanAllColumns)) {
    m_selectedSequence = isSelected ? NO_SELECTION : sequence;
  }
  ImGui::PopStyleColor();
  const bool isRowHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
  ImGui::PopID();

  ImGui::TableSetColumnIndex(1);
  ImGui::TextUnformatted(entry.timestamp);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted(entry.category.data(),
                         entry.category.data() + entry.category.size());

  ImGui::TableSetColumnIndex(3);
  const StringView message = entry.message;
  const size_t lineEnd = message.find('\n');
  const StringView firstLine = message.substr(0, lineEnd);
  const bool isCut = StringView::npos != lineEnd ||
                     ImGui::CalcTextSize(firstLine.data(),
                                         firstLine.data() + firstLine.size()).x >
                         ImGui::GetContentRegionAvail().x;
  ImGui::TextUnformatted(firstLine.data(), firstLine.data() + firstLine.size());

  ImGui::TableSetColumnIndex(4);
  if (!entry.sourceFile.empty()) {
    ImGui::Text("%s:%d", entry.sourceFile.c_str(), entry.sourceLine);
  }

  if (isRowHovered && isCut && ImGui::BeginTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 60.0f);
    ImGui::TextUnformatted(message.data(), message.data() + message.size());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

/*
 */
void
OutputLogUI::renderSelectedEntry()
{
  const LogBufferEntry& entry = getEntry(m_selectedSequence);

  ImGui::TextColored(getVerbosityColor(entry.verbosity), "%s",
                     getVerbosityIcon(entry.verbosity));
  ImGui::SameLine();
  ImGui::TextUnformatted(entry.timestamp);
  ImGui::SameLine();
  ImGui::TextUnformatted(entry.category.data(),
                         entry.category.data() + entry.category.size());
  if (!entry.sourceFile.empty()) {
    ImGui::SameLine();
    ImGui::Text("%s:%d", entry.sourceFile.c_str(), entry.sourceLine);
  }

  ImGui::SameLine();
  if (ImGui::SmallButton("Copy")) {
    ImGui::SetClipboardText(entry.message.c_str());
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Close")) {
    m_selectedSequence = NO_SELECTION;
  }

  if (ImGui::BeginChild("SelectedLogEntry", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
    if (!entry.sourceFunctionName.empty()) {
      ImGui::TextDisabled("%s", entry.sourceFunctionName.c_str());
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(entry.message.data(), entry.message.data() + entry.message.size());
    ImGui::PopTextWrapPos();
  }
  ImGui::EndChild();
}

/*
 */
void
OutputLogUI::addLogEntry(const SPtr<const LogBufferEntry>& entry)
{
  LockGuard<Mutex> lock(m_pendingMutex);
  m_pendingEntries.push_back(entry);
}

/*
 */
void
OutputLogUI::flushPendingEntries()
{
  // Swapping keeps the lock short and lets both vectors reuse their memory.
  {
    LockGuard<Mutex> lock(m_pendingMutex);
    m_pendingEntries.swap(m_flushEntries);
  }

  if (m_flushEntries.empty()) {
    return;
  }

  for (SPtr<const LogBufferEntry>& entry : m_flushEntries) {
    // New categories start enabled.
    if (m_availableCategories.insert(entry->category).second) {
      m_filter.enabledCategories.insert(entry->category);
    }

    const uint64 sequence = m_nextSequence;
    pushEntry(std::move(entry));

    // Only the new entry is checked; a full rebuild happens when the filter changes.
    if (!m_needsFilterUpdate && m_filter.passesFilter(getEntry(sequence))) {
      m_filteredSequences.push_back(sequence);
    }
  }
  m_flushEntries.clear();

  dropRemovedEntries();

  if (m_autoScroll) {
    m_needsScrollToBottom = true;
  }
}

/*
 */
void
OutputLogUI::pushEntry(SPtr<const LogBufferEntry> entry)
{
  if (m_entries.size() < m_maxLogEntries) {
    m_entries.push_back(std::move(entry));
  }
  else {
    m_entries[m_oldestIndex] = std::move(entry);
    ++m_oldestIndex;
    if (m_oldestIndex == m_entries.size()) {
      m_oldestIndex = 0;
    }
    ++m_oldestSequence;
  }
  ++m_nextSequence;
}

/*
 */
const LogBufferEntry&
OutputLogUI::getEntry(uint64 sequence) const
{
  CH_ASSERT(sequence >= m_oldestSequence && sequence < m_nextSequence);
  size_t index = m_oldestIndex + static_cast<size_t>(sequence - m_oldestSequence);
  if (index >= m_entries.size()) {
    index -= m_entries.size();
  }
  return *m_entries[index];
}

/*
 */
void
OutputLogUI::rebuildFilteredEntries()
{
  m_filteredSequences.clear();
  for (uint64 sequence = m_oldestSequence; sequence < m_nextSequence; ++sequence) {
    if (m_filter.passesFilter(getEntry(sequence))) {
      m_filteredSequences.push_back(sequence);
    }
  }

  m_needsFilterUpdate = false;
  if (m_autoScroll) {
    m_needsScrollToBottom = true;
  }
}

/*
 */
void
OutputLogUI::dropRemovedEntries()
{
  const auto firstKept = std::lower_bound(m_filteredSequences.begin(),
                                          m_filteredSequences.end(), m_oldestSequence);
  m_filteredSequences.erase(m_filteredSequences.begin(), firstKept);

  if (NO_SELECTION != m_selectedSequence && m_selectedSequence < m_oldestSequence) {
    m_selectedSequence = NO_SELECTION;
  }
}

/*
 */
void
OutputLogUI::clearLog()
{
  CH_LOG_DEBUG(OutputLogUILog, "Log cleared.");

  m_entries.clear();
  m_oldestIndex = 0;
  m_oldestSequence = m_nextSequence;
  m_filteredSequences.clear();
  m_selectedSequence = NO_SELECTION;
  m_availableCategories.clear();
  m_filter.enabledCategories.clear();
  m_needsFilterUpdate = true;
}

/*
 */
void
OutputLogUI::setMaxLogEntries(uint32 maxEntries)
{
  maxEntries = Math::max<uint32>(maxEntries, 1);

  // Puts the entries back in order, so the ring starts at index 0 again.
  std::rotate(m_entries.begin(), m_entries.begin() + m_oldestIndex, m_entries.end());
  m_oldestIndex = 0;

  if (m_entries.size() > maxEntries) {
    const size_t toRemove = m_entries.size() - maxEntries;
    m_entries.erase(m_entries.begin(), m_entries.begin() + toRemove);
    m_oldestSequence += toRemove;
  }

  m_maxLogEntries = maxEntries;
  m_entries.reserve(maxEntries);
  m_filteredSequences.reserve(maxEntries);
  dropRemovedEntries();
}

/*
 */
void
OutputLogUI::updateAvailableCategories()
{
  m_availableCategories.clear();
  for (const SPtr<const LogBufferEntry>& entry : m_entries) {
    m_availableCategories.insert(entry->category);
  }

  m_filter.enabledCategories = m_availableCategories;
  m_needsFilterUpdate = true;
}

/*
 */
bool
OutputLogUI::LogFilter::passesFilter(const LogBufferEntry& entry) const
{
  bool isLevelShown = true;
  switch (entry.verbosity) {
  case LogVerbosity::Debug:
    isLevelShown = showDebug;
    break;
  case LogVerbosity::Info:
    isLevelShown = showInfo;
    break;
  case LogVerbosity::Warning:
    isLevelShown = showWarning;
    break;
  case LogVerbosity::Error:
    isLevelShown = showError;
    break;
  case LogVerbosity::Fatal:
    isLevelShown = showFatal;
    break;
  case LogVerbosity::NoLogging:
    isLevelShown = showTrace;
    break;
  }

  if (!isLevelShown || enabledCategories.find(entry.category) == enabledCategories.end()) {
    return false;
  }

  return chString::containsIgnoreCase(entry.message, searchText) ||
         chString::containsIgnoreCase(entry.category, searchText);
}

} // namespace chEngineSDK
