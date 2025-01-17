/************************************************************************/
/**
 * @file chLog.h
 * @author AccelMR
 * @date 2022/06/15
 * @brief Log class that holds all stuff needed for a log and a logger to work.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Log Level enumerator.
 * 
 * This will describe which type of log entry is.
 *
 * Sample usage:
 *  g_Debug.logMsessage("UtilitiesLogger" ,"Message" ,LOG_LEVEL::kFATAL);
 * 
 * But prefer using a macro so you can actually get the whole information.
 * 
 * See UTIL_LOG("Test message", LOG_LEVEL::kINFO);
 *
 */
enum class LOG_LEVEL
{
  kFATAL,
  kERROR,
  kWARN,
  kINFO,
  kDEBUG,

  kCOUNT
};

/*
 * Description: 
 *     Log Entry that describes what a new log line should describe.
 *
 * Sample usage:
 *    LogEntry logEntry("Test message", LOG_LEVEL::kINFO);
 *
 */
class LogEntry {
 public:
  LogEntry() = delete;

  /** 
   *   Base constructor for this log entry.
   **/
  LogEntry(String _msg, LOG_LEVEL _level) : m_level(_level), m_msg(_msg) {}

  LOG_LEVEL
  getLevel() const {
    return m_level;
  }

  const String&
  getLogMessage() const {
    return m_msg;
  }

 private:
  String m_msg;
  LOG_LEVEL m_level;

};

/*
 * Description: 
 *   Logger that describes a container for new entries of one single type.
 *   That type must be sent on the creation so you can actually call a logMsg()
 *  through the Debug class.
 *
 * Sample usage:
 * g_Debug().registerLogger("newLogger");
 * g_Debug().logMessage("newLogger", "test Message", LOG_LEVEL::kINFO);
 * 
 *
 */
class Logger
{
 public:
  
 /*
  * @brief Default destructor
  */
  ~Logger() = default;
  
  /**
   *    Logs a new message.
   * 
   * @param message	
   *    The message describing the log entry.
   * 
   * @param channel 
   *    Channel in which to store the log entry.
   * 
   */
  FORCEINLINE void 
  logMsg(const String& msg, LOG_LEVEL level);

  /**
   *    Returns all log entries, including those marked as unread.
   */
  FORCEINLINE Vector<LogEntry>
  getAllEntries() const;

 private:
  friend class Debug;

  /*
  * @brief Default constructor
  */
  Logger() = default;

 protected:
   Vector<LogEntry> m_logs;
   mutable RecursiveMutex m_mutex;
};

/************************************************************************/
/*
 * Implementations
 */                                                                     
/************************************************************************/
/*
*/
FORCEINLINE void
Logger::logMsg(const String& msg, LOG_LEVEL level) {
  RecursiveLock lock(m_mutex);
  m_logs.emplace_back(msg, level);
}

/*
*/
FORCEINLINE Vector<LogEntry>
Logger::getAllEntries() const {
  RecursiveLock lock( m_mutex );
  return m_logs;
}

}