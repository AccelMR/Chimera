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
namespace chKeyBoard {
enum class Key: int32{
Escape               ,
Num0                 ,
Num1                 ,
Num2                 ,
Num3                 ,
Num4                 ,
Num5                 ,
Num6                 ,
Num7                 ,
Num8                 ,
Num9                 ,
A                    ,
B                    ,
C                    ,
D                    ,
E                    ,
F                    ,
G                    ,
H                    ,
I                    ,
J                    ,
K                    ,
L                    ,
M                    ,
N                    ,
O                    ,
P                    ,
Q                    ,
R                    ,
S                    ,
T                    ,
U                    ,
V                    ,
W                    ,
X                    ,
Y                    ,
Z                    ,
Minus                ,
Add                  ,
Multiply             ,
Divide               ,
Back                 ,
Enter                ,
Del                  ,
Tab                  ,
Numpad0              ,
Numpad1              ,
Numpad2              ,
Numpad3              ,
Numpad4              ,
Numpad5              ,
Numpad6              ,
Numpad7              ,
Numpad8              ,
Numpad9              ,
Up                   ,
Left                 ,
Down                 ,
Right                ,
Space                ,
F1                   ,
F2                   ,
F3                   ,
F4                   ,
F5                   ,
F6                   ,
F7                   ,
F8                   ,
F9                   ,
F10                  ,
F11                  ,
F12                  ,
LShift               ,
RShift               ,
LControl             ,
RControl             ,
LAlt                 ,
RAlt                 ,
Period               ,
Comma                ,
Semicolon            ,
Backslash            ,
Grave                ,
LBracket             ,
RBracket             ,
Apostrophe           ,
CapsLock             ,
Insert               ,
Delete               ,
Home                 ,
End                  ,
PageUp               ,
PageDown             ,
NumLock              ,
ScrollLock           ,
KeysMax
};

}
}

