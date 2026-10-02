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

namespace chEngineSDK {

/**
 * Editor window that shows the engine log, filtered by verbosity, category and text.
 * The last entries are kept in a ring buffer and only the visible rows are drawn, so
 * the cost per frame does not grow with the size of the log.
 */
class OutputLogUI
{
 public:
  struct LogFilter
  {
    bool showDebug = true;
    bool showInfo = true;
    bool showWarning = true;
    bool showError = true;
    bool showFatal = true;
    bool showTrace = true;

    Set<String> enabledCategories;

    // Kept in lower case, so each entry is compared without building new strings.
    String searchTextLower;

    NODISCARD bool
    passesFilter(const LogBufferEntry& entry) const;
  };

 public:
  OutputLogUI();
  ~OutputLogUI();

  void
  renderOutputLogUI();

  /**
   * Queues an entry to be shown on the next render. Safe to call from any thread.
   */
  void
  addLogEntry(const LogBufferEntry& entry);

  void
  clearLog();

  void
  setMaxLogEntries(uint32 maxEntries);

  FORCEINLINE void
  setAutoScroll(bool autoScroll) noexcept
  {
    m_autoScroll = autoScroll;
  }

  FORCEINLINE void
  setVisible(bool visible) noexcept
  {
    m_isVisible = visible;
  }

  NODISCARD FORCEINLINE bool
  isVisible() const noexcept
  {
    return m_isVisible;
  }

  /**
   * Rebuilds the category list from the kept entries and enables all of them.
   */
  void
  updateAvailableCategories();

 private:
  /**
   * Moves the queued entries into the log. Main thread only.
   */
  void
  flushPendingEntries();

  /**
   * Adds an entry to the ring buffer, replacing the oldest one when it is full.
   */
  void
  pushEntry(LogBufferEntry&& entry);

  /**
   * Entry with the given sequence number. It must still be in the ring buffer.
   */
  NODISCARD const LogBufferEntry&
  getEntry(uint64 sequence) const;

  void
  rebuildFilteredEntries();

  /**
   * Drops the filtered entries and the selection that left the ring buffer.
   */
  void
  dropRemovedEntries();

  void
  renderFilterControls();

  void
  renderLogEntries();

  void
  renderLogEntryRow(uint64 sequence);

  void
  renderSelectedEntry();

 private:
  static constexpr uint64 NO_SELECTION = ~0ull;

  bool m_isVisible = true;
  bool m_autoScroll = true;
  uint32 m_maxLogEntries = 1000;

  // Ring buffer. Every entry gets a sequence number that never changes, so the
  // filtered list stays valid when the oldest entries are replaced.
  Vector<LogBufferEntry> m_entries;
  uint32 m_oldestIndex = 0;
  uint64 m_oldestSequence = 0;
  uint64 m_nextSequence = 0;

  // Sequence numbers of the entries that pass the filter, oldest first.
  Vector<uint64> m_filteredSequences;
  uint64 m_selectedSequence = NO_SELECTION;

  Set<String> m_availableCategories;
  LogFilter m_filter;
  char m_searchBuffer[256] = {0};

  bool m_needsScrollToBottom = false;
  bool m_needsFilterUpdate = true;

  // Logs can come from any thread, so they wait here until the main thread renders.
  Mutex m_pendingMutex;
  Vector<LogBufferEntry> m_pendingEntries;
  Vector<LogBufferEntry> m_flushEntries;

  HEvent m_logWrittenEvent;
};

} // namespace chEngineSDK
