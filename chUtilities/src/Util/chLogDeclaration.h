/************************************************************************/
/**
 * @file chLogDeclarations.h
 * @author AccelMR
 * @date 2025/04/15
 * @brief Minimal declaration of chLogger.h to avoid circular dependencies
 */
 /************************************************************************/
#pragma once

#include "chPlatformTypes.h"

namespace chEngineSDK {
  // Declaración forward de LogCategory
  class LogCategory;
  
  // Enumeración de niveles de verbosidad (copia mínima de la definición en chLogger.h)
  enum class LogVerbosity : uint8 {
    NoLogging = 0,
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
    All = Debug
  };
}
 
/**
 * Declares a category defined with CH_LOG_DEFINE_CATEGORY_SHARED in another file.
 * ModuleExport is the export macro of the module that defines it (e.g. CH_CORE_EXPORT),
 * so inline code in a public header can log with it from other modules. Leave it empty
 * for a category only used inside its own module.
 */
#define CH_LOG_DECLARE_EXTERN(ModuleExport, CategoryName)                                   \
  extern ModuleExport chEngineSDK::LogCategory CategoryName