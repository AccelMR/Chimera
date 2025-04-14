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

#include "chLogLevel.h"

namespace chEngineSDK{
class LogEntry {
 public:
  LogEntry() = delete;

  /** 
   *   Base constructor for this log entry.
   **/
  LogEntry(String&& _msg, const LogLevel* level) 
    : m_msg(std::move(_msg)), 
      m_level(level) {}

  LogEntry(const char* _msg, const LogLevel* level) 
    : m_msg(_msg), 
      m_level(level) {}

  //disable copy constructor & disable copy assignment
  LogEntry(const LogEntry&) = delete;
  LogEntry& operator=(const LogEntry&) = delete;

  // allow move constructor & move assignment
  LogEntry(LogEntry&&) = default;
  LogEntry& operator=(LogEntry&&) = default;

  NODISCARD FORCEINLINE const LogLevel* 
  getLevel() const { return m_level; }

  NODISCARD FORCEINLINE const String&
  getLogMessage() const { return m_msg; }

 private:
  String m_msg;
  const LogLevel* m_level;

};

class Logger
{
 public:
  
 /*
  * @brief Default destructor
  */
  ~Logger() = default;
  
  FORCEINLINE void 
  logMsg(String&& msg, const LogLevel* level) {
    RecursiveLock lock(m_mutex);
    m_logs.emplace_back(std::move(msg), level);
  }

  FORCEINLINE void 
  logMsg(const char* msg, const LogLevel* level) {
      RecursiveLock lock(m_mutex);
      m_logs.emplace_back(msg, level);
  }

  FORCEINLINE const Vector<LogEntry>&
  getAllEntries() const {
    RecursiveLock lock(m_mutex);
    return m_logs;
  }

 private:
  friend class Debug;

  Logger() = default;

 protected:
   Vector<LogEntry> m_logs;
   mutable RecursiveMutex m_mutex;
};
}