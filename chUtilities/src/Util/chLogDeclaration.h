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
 * @brief Declare an extern log category to use in a .cpp file
 */
#define CH_LOG_DECLARE_EXTERN(CategoryName) extern chEngineSDK::LogCategory CategoryName