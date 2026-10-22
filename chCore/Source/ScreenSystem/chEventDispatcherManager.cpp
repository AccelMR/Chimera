/************************************************************************/
/**
 * @file chEventDispatcherManager.cpp
 * @author AccelMR
 * @date 2022/09/13
 * @brief
 *  Sends the frame's platform events where they belong: window events to its listeners,
 *  input to InputManager.
 */
/************************************************************************/
#include "chEventDispatcherManager.h"

#include "chDisplayEventHandle.h"
#include "chInputManager.h"

namespace chEngineSDK {

/*
 */
void
EventDispatcherManager::dispatchEvents(const DisplayEventHandle& eventHandler)
{
  InputManager* input = InputManager::isStarted() ? &InputManager::instance() : nullptr;
  for (const DisplayEvent& event : eventHandler.getEvents()) {
    if (const ResizeEvent* resize = std::get_if<ResizeEvent>(&event)) {
      OnResize(resize->width, resize->height);
    }
    else if (std::holds_alternative<CloseEvent>(event)) {
      OnClose();
    }
    else {
      // Focus is both: the window listeners care, and the input state must release what
      // the window stops hearing about.
      if (const FocusEvent* focus = std::get_if<FocusEvent>(&event)) {
        OnFocusChanged(focus->bFocused);
      }
      if (input) {
        input->processEvent(event);
      }
    }
  }
}

} // namespace chEngineSDK
