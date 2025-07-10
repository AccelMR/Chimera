/************************************************************************/
/**
 * @file chResizeDebouncer.h
 * @author AccelMR
 * @date 2025/04/12
 * @brief
 * Resize debouncer class.
 * This class is used to debounce resize events.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class ResizeDebouncer {
 public:
  ResizeDebouncer(uint32 timeoutMs = 250,
                  Function<void(uint32, uint32)> callback = nullptr)
      : m_timeoutMs(timeoutMs),
        m_callback(callback),
        m_active(false) {}

  ~ResizeDebouncer();

  void
  setCallback(Function<void(uint32, uint32)> callback);

  void
  stopDebounce();

  void
  onResize(uint32 width, uint32 height);

 private:

  void
  startDebounce();

 private:
  uint32 m_timeoutMs;
  Function<void(uint32, uint32)> m_callback;

  uint32 m_latestWidth = 0;
  uint32 m_latestHeight = 0;
  std::chrono::steady_clock::time_point m_lastResizeTime;
  Atomic<bool> m_active;

  Mutex m_mutex;
  Optional<Thread> m_debounceThread;
};

} // namespace chEngineSDK
