/************************************************************************/
/**
 * @file chDisplayEventHandle.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief
 *  Reads the platform's window and input events once per frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chDisplayEvent.h"
#include "chEventSystem.h"

namespace chEngineSDK {

/**
 * Exists so the rest of the engine never sees the platform layer's events: update() reads
 * them and turns them into DisplayEvents, which EventDispatcherManager hands on. Only the
 * platform implementation (SDL3 today) knows the native events, so another one only needs
 * a new update().
 */
class CH_CORE_EXPORT DisplayEventHandle
{
 public:
  DisplayEventHandle() = default;

  ~DisplayEventHandle() = default;

  /**
   * Replaces the events of the last frame with the ones the platform has now.
   */
  void
  update();

  NODISCARD FORCEINLINE Span<const DisplayEvent>
  getEvents() const noexcept
  {
    return m_events;
  }

  /**
   * The callback receives every native event before the engine turns it into its own.
   * With SDL3 the Any holds a const SDL_Event*, so no event is copied.
   */
  NODISCARD FORCEINLINE HEvent
  addUpdateInjection(Function<bool(const Any&)> updateFunc)
  {
    return m_updateInjection.connect(std::move(updateFunc));
  }

 protected:
  friend class DisplaySurface;

 private:
  // Keeps its memory between frames.
  Vector<DisplayEvent> m_events;

  // Closing this window closes the application. Other windows (ImGui panels dragged out of
  // it) handle their own close requests, and while they exist the platform sends no quit.
  uint32 m_mainWindowId = 0;

  const Event<bool(const Any&)> m_updateInjection;
};

} // namespace chEngineSDK
