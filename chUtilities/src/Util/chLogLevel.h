/************************************************************************/
/**
 * @file chCustomLogLevel.h
 * @author AccelMR
 * @date 2025/04/13
 * @brief 
 * Custom log level definitions for the Chimera Engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"\

#include "chLog.h"

namespace chEngineSDK {
class CH_UTILITY_EXPORT LogLevel {
 public:
  LogLevel(const String& name, 
    const String& displayName, 
    const String& cssStyle,
    uint32 severity) :
    m_name(name), 
    m_displayName(displayName), 
    m_cssStyle(cssStyle),
    m_severity(severity) {} 

  NODISCARD FORCEINLINE const String& 
  getName() const { return m_name; }

  NODISCARD FORCEINLINE const String&
  getDisplayName() const { return m_displayName; }

  NODISCARD FORCEINLINE const String&
  getCssStyle() const { return m_cssStyle; }

  NODISCARD FORCEINLINE uint32
  getSeverity() const { return m_severity; }

  NODISCARD FORCEINLINE bool
  operator==(const LogLevel& other) const { return this == &other; }

 public:
  static const LogLevel* FATAL;
  static const LogLevel* ERROR;
  static const LogLevel* WARN;
  static const LogLevel* INFO;
  static const LogLevel* DEBUG;

 private:
  String m_name;
  String m_displayName;
  String m_cssStyle;
  uint32 m_severity;
};

} // namespace chEngineSDK