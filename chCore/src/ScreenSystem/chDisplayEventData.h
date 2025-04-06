/************************************************************************/
/**
 * @file chDisplayEventData.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief DisplaySurface Event data to keep all type of data for screen events, platform independent.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chKeyBoard.h"

namespace chEngineSDK{
struct MouseMoveData
{
  MouseMoveData() = delete;
  MouseMoveData(uint32 _x, 
                uint32 _y,
                uint32 _screenx, 
                uint32 _screeny, 
                int32 _deltax, 
                int32 _deltay)
    : x(_x), 
      y(_y), 
      screenx(_screenx), screeny(_screeny), 
      deltax(_deltax), deltay(_deltay) {}

  // Current x position relative to active window
  uint32 x;

  // Current y position relative to active window
  uint32 y;

  // Current global x position
  uint32 screenx;

  // Current in global y position
  uint32 screeny;

  // Change in x relative to previous event, used for FPS motion
  int32 deltax;

  // Change in y relative to previous event, used for FPS motion
  int32 deltay;

};

struct KeyBoardData {
  KeyBoardData() = delete;
  KeyBoardData(chKeyBoard::Key _key, 
                KEYBOARD_STATE _state, 
                uint8_t _modifiers)
    : key(_key), 
      state(_state), 
      modifiers(_modifiers) {}

  chKeyBoard::Key key;
  KEYBOARD_STATE state;
  uint8_t modifiers;

  bool hasModifier(KEY_MODIFIER mod) const {
    return (modifiers & static_cast<uint16_t>(mod) != 0);
  }

};

struct ResizeData {
  uint32 width;
  uint32 height;
};

using DisplayEventData = std::variant<std::monostate,
                                      MouseMoveData,
                                      KeyBoardData,
                                      ResizeData>;

} // namespace chEngineSDK