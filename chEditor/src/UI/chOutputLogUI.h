/************************************************************************/
/**
 * @file chOutputLogUI.h
 * @author AccelMR
 * @date 2025/07/28
 * @brief Output log window with filtering, search and categorization
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chLogger.h"

#include "chMultiStageRenderer.h"

struct ImVec4;
namespace chEngineSDK {
// Forward declarations
/**
 * @brief UI component for displaying and filtering engine log output
 */
class OutputLogUI
{
 public:

  /**
   * @brief Filter settings for log display
   */
  struct LogFilter {
    bool showDebug = true;
    bool showInfo = true;
    bool showWarning = true;
    bool showError = true;
    bool showFatal = true;
    bool showTrace = true;

    Set<String> enabledCategories;
    String searchText;

    // Check if an entry passes current filters
    bool
    passesFilter(const LogBufferEntry& entry) const;
  };

 public:
  OutputLogUI();
  ~OutputLogUI();
  /**
   * @brief Main rendering function for the output log window
   */
  void
  renderOutputLogUI();

  /**
   * @brief Add a new log entry to the display
   */
  void
  addLogEntry(const LogBufferEntry& entry);

  /**
   * @brief Clear all log entries
   */
  void
  clearLog();

  /**
   * @brief Set maximum number of log entries to keep in memory
   */
  void
  setMaxLogEntries(uint32 maxEntries) {
    m_maxLogEntries = maxEntries;
  }

  /**
   * @brief Enable/disable auto-scroll to bottom
   */
  void
  setAutoScroll(bool autoScroll) {
    m_autoScroll = autoScroll;
  }

  /**
   * @brief Show/hide the output log window
   */
  void
  setVisible(bool visible) {
    m_isVisible = visible;
  }
  bool
  isVisible() const {
    return m_isVisible;
  }

  /**
   * @brief Append multiple log entries at once
   */
  void
  appendLogEntries(const Vector<LogBufferEntry>& entries) {
    m_logEntries.insert(m_logEntries.end(), entries.begin(), entries.end());
    m_needsFilterUpdate = true;
    applySizeLimits();
  }

  /**
   * @brief Update the list of available categories from log entries
   */
  void
  updateAvailableCategories();

 private:
  /**
   * @brief Render the filter controls (verbosity, categories, search)
   */
  void
  renderFilterControls();

  /**
   * @brief Render the log entries table
   */
  void
  renderLogEntries();

  /**
   * @brief Render a single log entry row
   */
  void
  renderLogEntryRow(const LogBufferEntry& entry, int32 index);

  /**
   * @brief Get color for log verbosity level
   */
  ImVec4
  getVerbosityColor(LogVerbosity verbosity) const;

  /**
   * @brief Get icon for log verbosity level
   */
  const char*
  getVerbosityIcon(LogVerbosity verbosity) const;

  /**
   * @brief Apply size limits to log entries buffer
   */
  void
  applySizeLimits();

 private:
  // UI state
  bool m_isVisible = true;
  bool m_autoScroll = true;
  uint32 m_maxLogEntries = 1000;

  // Log data
  Vector<LogBufferEntry> m_logEntries;
  Vector<LogBufferEntry> m_filteredEntries;
  Set<String> m_availableCategories;

  // Filtering
  LogFilter m_filter;
  char m_searchBuffer[256] = {0};

  // UI state
  bool m_needsScrollToBottom = false;
  bool m_needsFilterUpdate = true;

  HEvent m_logWrittenEvent;
};

} // namespace chEngineSDK
