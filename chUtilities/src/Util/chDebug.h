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
/*
 * Description: 
 *     Debug global class.
 * You should NEVER call a constructor for this class. Instead you should use 
 * g_debug() so you can get the ONLY Debug object that should exist. 
 *
 * Sample usage:
 *  g_Deug().registerLogger("NewLogger);
 *  g_Debug().logMesg("NewLogger", msg, loggerType);   or
 *  CH_UTIL_DEBUG(msg, loggerType);
 */
class CH_UTILITY_EXPORT Debug
{
 public:
 /*
  *  Default constructor.
  *   DO NOT USE THIS CONSTRUCTOR!!!
  *   To use Debug you'll need to call g_Debug() and get the global debugger.  
  */
  Debug() = default;

 /*
  *   Default destructor
  * NOTE: we do note call delete m_logger[key] because std::map already calls 
  *   a specific deallocator. Once a memory manager has been done probably we need 
  *   to add the deallocator to the map wrapper.
  */
  ~Debug() = default;

  /** 
   *   Logs a message to the given Log type and with its respective level.
   * 
   * @param type
   *  Type of logger to be used. This type needs to be created before calling this
   *  or you'll get an error.
   * 
   * @param msg
   *  The message to be logged.
   * 
   * @param level
   *    Message's level to log this entry. 
   * 
   * @param 
   **/
  void
  logMessage(const String& msg, LOG_LEVEL level);

  /** 
   *   Returns a const pointer to the logger specified.
   * 
   * @param  type
   *    Type of the logger you are looking for.
   * 
   * @return Logger* 
   *  Pointer to the logger.
   **/
  FORCEINLINE Logger&
  getLogger();

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
  logBacktrace(LOG_LEVEL level = LOG_LEVEL::kFATAL);

 private:
  /** 
   *   Map that holds pointer to any created logger.
   **/
  Logger m_log;
};

/** 
 *   Debug reference global for the whole engine. 
 **/
CH_UTILITY_EXPORT Debug&
g_Debug();

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/

FORCEINLINE Logger&
Debug::getLogger() {
  return m_log;
}

/************************************************************************/
/*
 * MACROS!
 */                                                                     
/************************************************************************/
#define LOG_INFO(x) g_Debug().logMessage((x) +                        \
                                         String("\n\t in ") +         \
                                         __PRETTY_FUNCTION__ +        \
                                         " [" + __FILE__ + ":" +      \
                                         std::to_string(__LINE__) +   \
                                         "]\n",                       \
                                         LOG_LEVEL::kINFO);

#if USING(CH_DEBUG_MODE)
#define LOG_DBG(x) g_Debug().logMessage((x) +                         \
                                         String("\n\t in ") +         \
                                         __PRETTY_FUNCTION__ +        \
                                         " [" + __FILE__ + ":" +      \
                                         std::to_string(__LINE__) +   \
                                         "]\n",                       \
                                         LOG_LEVEL::kDEBUG);
#else
#define LOG_DBG(x)
#endif


#define LOG_WARN(x) g_Debug().logMessage((x) +                        \
                                         String("\n\t in ") +         \
                                         __PRETTY_FUNCTION__ +        \
                                         " [" + __FILE__ + ":" +      \
                                         std::to_string(__LINE__) +   \
                                         "]\n",                       \
                                         LOG_LEVEL::kWARN);

#define LOG_ERROR(x) g_Debug().logMessage((x) +                       \
                                         String("\n\t in ") +         \
                                         __PRETTY_FUNCTION__ +        \
                                         " [" + __FILE__ + ":" +      \
                                         std::to_string(__LINE__) +   \
                                         "]\n",                       \
                                         LOG_LEVEL::kERROR);

#define LOG_FATAL(x) g_Debug().logMessage((x) +                       \
                                         String("\n\t in ") +         \
                                         __PRETTY_FUNCTION__ +        \
                                         " [" + __FILE__ + ":" +      \
                                         std::to_string(__LINE__) +   \
                                         "]\n",                       \
                                         LOG_LEVEL::kFATAL);

}