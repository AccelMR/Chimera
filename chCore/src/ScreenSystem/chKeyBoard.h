/************************************************************************/
/**
 * @file chKeys.h
 * @author AccelMR
 * @date 2022/09/12
 * @brief Keeps a bing enum of different keys from driver that can be pressed.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

namespace chEngineSDK{
enum class KeyBoardState : uint32
{
  UP = 0,
  DOWN,
  PRESSED,
  RELEASED,
  
  NONE
};

enum class KeyBoardModifier : uint16
{
  NONE = 0,
  LSHIFT    = 1 << 0,
  RSHIFT    = 1 << 1,
  LCTRL     = 1 << 2,
  RCTRL     = 1 << 3,
  LALT      = 1 << 4,
  RALT      = 1 << 5,
  LMETA     = 1 << 6,
  RMETA     = 1 << 7,
  NUMLOCK   = 1 << 8,
  CAPSLOCK  = 1 << 9,
  META      = 1 << 10,
};

namespace chKeyBoard {
enum class Key : uint32
{
  Escape = 0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Num0,
  Minus,
  Equals,
  Back,
  Tab,
  Q,
  W,
  E,
  R,
  T,
  Y,
  U,
  I,
  O,
  P,
  LBracket,
  RBracket,
  Enter,
  LControl,
  A,
  S,
  D,
  F,
  G,
  H,
  J,
  K,
  L,
  Semicolon,
  Colon,
  Apostrophe,
  Quotation,
  Grave,
  LShift,
  Backslash,
  Z,
  X,
  C,
  V,
  B,
  N,
  M,
  Comma,
  Period,
  Slash,
  RShift,
  Multiply,
  LAlt,
  Space,
  Capital,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  Numlock,
  Scroll,
  Numpad7,
  Numpad8,
  Numpad9,
  Subtract,
  Numpad4,
  Numpad5,
  Numpad6,
  Add,
  Numpad1,
  Numpad2,
  Numpad3,
  Numpad0,
  Decimal,
  F11,
  F12,
  Numpadenter,
  RControl,
  Divide,
  sysrq,
  RAlt,
  Pause,
  Home,
  Up,
  PgUp,
  Left,
  Right,
  End,
  Down,
  PgDn,
  Insert,
  Del,
  LMod,
  RMod,
  Apps,

  KeysMax
};
} // namespace chKeyBoard

enum class MouseButton : uint16 {
  Left = 0,
  Right,
  Middle,
  MouseButton4,
  MouseButton5,
  MouseButton6,
  MouseButton7,
  MouseButton8,

  MouseButtonsMax
};

enum class MouseState : uint16 {
  Up = 0,
  Down,
  Pressed,
  Released,

  NoneMouse
};
} // namespace chEngineSDK

