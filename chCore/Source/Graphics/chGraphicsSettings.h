/************************************************************************/
/**
 * @file chGraphicsSettings.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief Console variables of the graphics API, section [Graphics] of Engine.ini.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chConsoleVariable.h"

namespace chEngineSDK {

// Plugin loaded at start up (-GraphicsAPI=<name>).
extern CH_CORE_EXPORT ConsoleVariable<String> g_cvarGraphicsAPI;

// Every swap chain, also those of other windows, is made with this choice.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarVSync;

extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarValidationLayer;

} // namespace chEngineSDK
