/************************************************************************/
/**
 * @file chScreenEventData.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief Screen Event data to keep all type of data for screen events, platform independent.
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

  MouseMoveData() = default;
};

struct KeyBoardData {
  chKeyBoard::Key key;
};

struct ResizeData {
  uint32 width;
  uint32 height;
};

/** 
 *   Union to handle all types of data.
 *
 * SDL does something similar:
 * <https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlevent.html>
 **/
union ScreenEventData {
  MouseMoveData mouseMoveData;
  KeyBoardData keyBoardData;
  ResizeData resizeData;

  ScreenEventData() = default;
  ~ScreenEventData() = default;
};

}