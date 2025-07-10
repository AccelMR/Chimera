/************************************************************************/
/**
 * @file chResizeDebouncer.cpp
 * @author AccelMR
 * @date 2025/04/12
 * @brief
 * Resize debouncer class.
 * This class is used to debounce resize events.
 */
/************************************************************************/

#include "chResizeDebouncer.h"

namespace chEngineSDK {
/*
*/
ResizeDebouncer::~ResizeDebouncer() {
  stopDebounce();
}

/*
*/
void
ResizeDebouncer::setCallback(Function<void(uint32, uint32)> callback) {
  LockGuard<Mutex> lock(m_mutex);
  m_callback = std::move(callback);
}

/*
*/
void
ResizeDebouncer::stopDebounce() {
  {
    LockGuard<Mutex> lock(m_mutex);
    m_active = false;
  }

  if (m_debounceThread.has_value() && m_debounceThread->joinable()) {
    m_debounceThread->join();
    m_debounceThread.reset();
  }
}

/*
*/
void
ResizeDebouncer::onResize(uint32 width, uint32 height) {
  m_latestWidth = width;
  m_latestHeight = height;

  auto now = std::chrono::steady_clock::now();

  {
    LockGuard<Mutex> lock(m_mutex);
    m_lastResizeTime = now;
    if (!m_active){
      startDebounce();
    }
  }
}

/*
*/
void
ResizeDebouncer::startDebounce() {
  m_active = true;

  if (m_debounceThread.has_value() && m_debounceThread->joinable()) {
    m_debounceThread->join();
    m_debounceThread.reset();
  }

  m_debounceThread.emplace([this](){
    using namespace std::chrono;

    while(true){
      steady_clock::time_point lastTime;
      bool bShouldRun = false;
      Function<void(uint32, uint32)> callback;

      {
        LockGuard<Mutex> lock(m_mutex);
        lastTime = m_lastResizeTime;
        bShouldRun = m_active;
        callback = m_callback;
      }

      if (!bShouldRun) {
        break;
      }

      auto now = steady_clock::now();
      auto duration = duration_cast<milliseconds>(now - lastTime).count();

      if (duration >= m_timeoutMs) {
        if (callback) {
          callback(m_latestWidth, m_latestHeight);
        }

        {
          LockGuard<Mutex> lock(m_mutex);
          m_active = false;
        }

        break;
      }
      else {
        std::this_thread::sleep_for(milliseconds(
          std::min<uint64>(50, m_timeoutMs - duration)
        ));
      }
    }
  });
}
} // namespace chEngineSDK
