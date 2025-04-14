/************************************************************************/
/**
 * @file chDebug.h
 * @author AccelMR
 * @date 2022/06/15
 * @brief Singleton Debug that handles all engine outputs.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chLog.h"

namespace chEngineSDK{
class CH_UTILITY_EXPORT Debug
{
 public:

 /*
  *   Default destructor
  * NOTE: we do note call delete m_logger[key] because std::map already calls 
  *   a specific deallocator. Once a memory manager has been done probably we need 
  *   to add the deallocator to the map wrapper.
  */
  ~Debug() = default;

  NODISCARD const LogLevel* 
  registerLogLevel(const String& name, 
                   const String& displayName, 
                   const String& htmlStyle,
                   uint32 severity);

  void
  logMessage(String&& msg, const LogLevel* level);

  void
  logMessage(const char* msg, const LogLevel* level);

  // void
  // logMessage(const String& msg, const LogLevel* level);

  /** 
   *   Returns a const pointer to the logger specified.
   * 
   * @param  type
   *    Type of the logger you are looking for.
   * 
   * @return Logger* 
   *  Pointer to the logger.
   **/
  NODISCARD FORCEINLINE Logger& 
  getLogger() { return m_log; }

  /**
   *     Saves a log about the current state of the application to the
   *        specified location.
   * 
   * @param path  
   *    Absolute path to the log filename.
   */
  void
  saveLog(const Path& path) const;

  /**
   *     Saves a log about the current state of the application to the
   *        specified location in HTML format.
   * 
   * @param path  
   *    Absolute path to the log filename.
   */
  void 
  saveLogAsHtml(const Path& path) const;

  /*
   *   Logs a backtrace of the current state of the application.
  */
  void 
  logBacktrace(const LogLevel* level = LogLevel::FATAL);

  static Debug& 
  getInstance();

 private:
  Debug();

  Logger m_log;
  UnorderedMap<String, std::unique_ptr<LogLevel>> m_logLevels;
};

/** 
 *   Debug reference global for the whole engine. 
 **/
CH_UTILITY_EXPORT Debug&
g_Debug();

/************************************************************************/
/*
 * MACROS!
 */                                                                     
/************************************************************************/
// Macro base optimizada con movimiento
#define CH_LOG_BASE(level, x) do { \
  auto&& _log_msg = String(x) + String("\n\t in ") + \
                   __PRETTY_FUNCTION__ + \
                   " [" + __FILE__ + ":" + \
                   std::to_string(__LINE__) + "]\n"; \
  chEngineSDK::g_Debug().logMessage(std::move(_log_msg), level); \
} while(0)

// Macros específicas por nivel
#define CH_LOG_FATAL(x)   CH_LOG_BASE(chEngineSDK::LogLevel::FATAL, x)
#define CH_LOG_ERROR(x)   CH_LOG_BASE(chEngineSDK::LogLevel::ERROR, x)
#define CH_LOG_WARNING(x) CH_LOG_BASE(chEngineSDK::LogLevel::WARN, x)
#define CH_LOG_INFO(x)    CH_LOG_BASE(chEngineSDK::LogLevel::INFO, x)

// Debug solo en modo debug
#if USING(CH_DEBUG_MODE)
#define CH_LOG_DEBUG(x) CH_LOG_BASE(chEngineSDK::LogLevel::DEBUG, x)
#else
#define CH_LOG_DEBUG(x) 
#endif

// Macro para niveles personalizados
#define CH_LOG_CUSTOM(level, x) CH_LOG_BASE(level, x)

// Macro para registrar nuevos niveles
#define CH_REGISTER_LOG_LEVEL(name, displayName, cssStyle, severity) \
    static const chEngineSDK::LogLevel* name = \
        []() -> const chEngineSDK::LogLevel* { \
            static const auto _level = chEngineSDK::g_Debug().registerLogLevel( \
                #name, displayName, cssStyle, severity); \
            return _level; \
        }()
} // namespace chEngineSDK