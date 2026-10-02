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
#include "chSTDThreading.h"

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
   * @brief Queue a log entry to be shown on the next render. Safe to call from any thread.
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
   * @brief Update the list of available categories from log entries
   */
  void
  updateAvailableCategories();

 private:
  /**
   * @brief Move the queued entries into the displayed log. Main thread only.
   */
  void
  flushPendingEntries();

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

  // Logs can come from any thread, so they wait here until the main thread renders.
  Mutex m_pendingMutex;
  Vector<LogBufferEntry> m_pendingEntries;
  Vector<LogBufferEntry> m_flushEntries;

  HEvent m_logWrittenEvent;
};

} // namespace chEngineSDK
