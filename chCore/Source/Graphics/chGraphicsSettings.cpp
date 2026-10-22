/************************************************************************/
/**
 * @file chGraphicsSettings.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief Console variables of the graphics API, section [Graphics] of Engine.ini.
 */
/************************************************************************/
#include "chGraphicsSettings.h"

namespace chEngineSDK {

ConsoleVariable<String> g_cvarGraphicsAPI("Graphics.API",
                                          CH_DEFAULT_GRAPHICS_API,
                                          "Graphics API plugin loaded at start up.",
                                          "GraphicsAPI");

ConsoleVariable<bool> g_cvarVSync("Graphics.VSync",
                                  false,
                                  "Waits for the display refresh before showing a frame.",
                                  "VSync");

// On only in debug builds: the checks cost frame time, and a player without the graphics SDK
// has no validation layer to load.
ConsoleVariable<bool> g_cvarValidationLayer("Graphics.ValidationLayer",
                                            USING(CH_DEBUG_MODE),
                                            "Turns on the graphics API debug checks. Default: on "
                                            "in debug builds only.",
                                            "ValidationLayer");

} // namespace chEngineSDK
