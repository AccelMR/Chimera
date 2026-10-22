/************************************************************************/
/**
 * @file chEventDispatcherManager.h
 * @author AccelMR
 * @date 2022/09/13
 * @brief
 *  Sends the frame's platform events where they belong: window events to its listeners,
 *  input to InputManager.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chDisplayEvent.h"
#include "chEventSystem.h"
#include "chModule.h"

namespace chEngineSDK {

/**
 * Exists because the platform hands out window and input events in one queue, read once
 * per frame by DisplayEventHandle, while the engine wants them apart: this sends the input
 * ones to InputManager (state, actions, key events) and fires the window ones (close,
 * resize, focus) here, so neither side sees the other's events.
 */
class CH_CORE_EXPORT EventDispatcherManager : public Module<EventDispatcherManager>
{
 public:
  EventDispatcherManager() = default;

  ~EventDispatcherManager() override = default;

  /**
   * Sends every event the handle read this frame, in order.
   */
  void
  dispatchEvents(const DisplayEventHandle& eventHandler);

  const Event<void()> OnClose;
  const Event<void(uint32, uint32)> OnResize;
  const Event<void(bool)> OnFocusChanged;
};

} // namespace chEngineSDK
