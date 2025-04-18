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
      screenX(_screenx), screenY(_screeny), 
      deltaX(_deltax), deltaY(_deltay) {}

  // Current x position relative to active window
  uint32 x;

  // Current y position relative to active window
  uint32 y;

  // Current global x position
  uint32 screenX;

  // Current in global y position
  uint32 screenY;

  // Change in x relative to previous event, used for FPS motion
  int32 deltaX;

  // Change in y relative to previous event, used for FPS motion
  int32 deltaY;

};

struct KeyBoardData {
  KeyBoardData() = default;
  KeyBoardData(chKeyBoard::Key _key, 
                KeyBoardState _state, 
                uint16 _modifiers)
    : key(_key), 
      state(_state), 
      modifiers(_modifiers) {}

  chKeyBoard::Key key;
  KeyBoardState state;
  uint16 modifiers;

  bool hasModifier(KeyBoardModifier mod) const {
    return (modifiers & static_cast<uint16>(mod) != 0);
  }

};

struct MouseButtonData{
  MouseButtonData() = delete;
  MouseButtonData(MouseButton _button,
                  MouseState _state,
                  uint32 _x, uint32 _y)
    : button(_button), state(_state), x(_x), y(_y) {}

  MouseButton button;
  MouseState state;
  uint32 x;
  uint32 y;
};

struct ResizeData {
  uint32 width;
  uint32 height;
};

using DisplayEventData = std::variant<std::monostate,
                                      MouseMoveData,
                                      KeyBoardData,
                                      ResizeData,
                                      MouseButtonData>;

} // namespace chEngineSDK