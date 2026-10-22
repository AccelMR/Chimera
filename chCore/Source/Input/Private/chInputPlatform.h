/************************************************************************/
/**
 * @file chInputPlatform.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  What the input system asks of the platform layer.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chInputManager.h"

namespace chEngineSDK {

/**
 * Exists so InputManager never names the platform layer: the few things it must ask of it
 * (cursor, rumble, key names of the current layout) are these functions, written once per
 * platform layer next to its events (Platform/SDL today). Changing the layer means a new
 * implementation of this and of DisplayEventHandle::update, nothing else.
 */
class InputPlatform
{
 public:
  static void
  setCursorMode(DisplaySurface& window, CursorMode mode);

  static bool
  rumbleGamepad(uint32 slot, float lowFrequency, float highFrequency, uint32 milliseconds);

  /**
   * The key's name in the current keyboard layout; empty when the layer has none.
   */
  NODISCARD static String
  getKeyboardKeyName(InputKey key);

  /**
   * Closes every gamepad the platform layer opened.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
