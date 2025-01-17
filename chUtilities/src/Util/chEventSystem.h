/************************************************************************/
/**
 * @file chEventSystem.h
 * @author AccelMR
 * @date 2022/06/15
 * @brief This file will define every stuff needed for the event system.
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
using std::function;
using std::forward;
/*
 * Description: 
 *     This class works as a node in a double linked list and will need data needed
 *     to handle the event, meaning a function pointer.
 */
class BaseConnectionNode
{
 public:
 /*
  *   Default constructor
  */
  FORCEINLINE BaseConnectionNode() = default;

 /*
  *   Default destructor
  */
  FORCEINLINE virtual ~BaseConnectionNode() {
    CH_ASSERT(!m_size && !m_isActive);
  }

  FORCEINLINE virtual void
   deactivate() {
    m_isActive = false;
  }

 public:

  BaseConnectionNode* m_prev = nullptr;
  BaseConnectionNode* m_next = nullptr;
  uint32 m_size = 0;
  bool m_isActive = true;
};

/*
 * Description: 
 *     This will act as the main node of the list and controller of it.
 */
class ConnectionController {
 public:
 /** 
  *   Default constructor.
  **/
  FORCEINLINE ConnectionController() = default;

  FORCEINLINE ~ConnectionController() {
    BaseConnectionNode* conn = m_connections;
    while (nullptr != conn) {
      BaseConnectionNode* next = conn->m_next;
      delete conn;
      conn = next;
    }

    conn = m_freeConnections;
    while (nullptr != conn) {
      BaseConnectionNode* next = conn->m_next;
      delete conn;
      conn = next;
    }

    conn = m_newConnection;
    while (nullptr != conn) {
      BaseConnectionNode* next = conn->m_next;
      delete conn;
      conn = next;
    }
  }

  /** 
   *   Adds a new connection to the connection list.
   * 
   * @param _connection
   *   The new connection to be added to the connection list.
   **/
  FORCEINLINE void
  connect(BaseConnectionNode* _connection);

  /** 
   *   Removes a connection from from the connection list.
   * 
   * @param _connection
   *   The connection that will be removed from the connection list.
   **/
  FORCEINLINE void
  disconnect(BaseConnectionNode* _connection);

  /** 
   *   Clears the whole list of connections.
   **/
  FORCEINLINE void
  clear();

  /* 
  * Releases the connection of this controller
  */
  FORCEINLINE void
  freeHandle(BaseConnectionNode* conn);

  FORCEINLINE void
  free(BaseConnectionNode* conn);

  BaseConnectionNode* m_connections = nullptr;
  BaseConnectionNode* m_freeConnections = nullptr;
  BaseConnectionNode* m_lastConnection = nullptr;
  BaseConnectionNode* m_newConnection = nullptr;

  RecursiveMutex m_mutex;
  bool m_isCurrentlyInUse = false;
};

/*
 * Description: 
 *     Handler of event. This class just contains basic stuff to communicate with
 *  the actual data.
 */
class HEvent {
 public:
  HEvent() = default;

  FORCEINLINE HEvent(const HEvent& e) {
    this->operator=(e);
  }

  /** 
   *   Constructor with a node.
   **/
  FORCEINLINE explicit HEvent(SPtr<ConnectionController> _controller, BaseConnectionNode* _node)
    : m_connection(_node),
      m_controller(_controller) {
    ++m_connection->m_size;
  }

  /** 
   *   Default destructor.
   **/
  FORCEINLINE ~HEvent() {
    if (nullptr != m_connection) {
      m_controller->freeHandle(m_connection);
    }
  }
  
  /** 
   *   Disconnects the internal subscriber from event.
   **/
  FORCEINLINE void
  disconnect();

  FORCEINLINE HEvent& operator=(const HEvent& rhs) {
    m_connection = rhs.m_connection;
    m_controller = rhs.m_controller;

    if (nullptr != m_connection) {
      ++m_connection->m_size;
    }
    return *this;
  }

 private:
  BaseConnectionNode* m_connection;
  SPtr<ConnectionController> m_controller;
};

/*
 * Description: 
 *     Actual event that will keep track of the whole list of subscribers and will
 *  call them.
 *
 * Sample usage:
 *  See class Event in chEventSystem.h
 * 
 *
 */
template<class ReturnType, class... Args>
class TEvent {
 private:
   struct BasicConnectionNode : BaseConnectionNode {
    public:
      function<ReturnType(Args...)> m_function;
   };
 public:
 /** 
  *   Base constructor.
  **/
  FORCEINLINE TEvent() {
    m_connectionController =  ch_shared_ptr_new<ConnectionController>();
  }

  /** 
   *   Base destructor.
   **/
  FORCEINLINE  ~TEvent() {
    clear();
  }

  /** 
   *   Connects a new subscriber to this event and returns a handler to that subscriber.
   * 
   * @param func
   *   The template function pointer to be called.
   *  
   * @return HEvent
   *  Handler to the subscriber.
   **/
  FORCEINLINE HEvent
  connect(function<ReturnType(Args...)> func) const{
    auto* connData = new BasicConnectionNode();
    m_connectionController->connect(connData);
    connData->m_function = func;
    return HEvent(m_connectionController, connData);
  }

  /** 
   *   Calls all subscribers connected to this event.
   * 
   * @param args
   *   Any parameter already defined as template.
   **/
  FORCEINLINE void
  operator()(Args... args) const{
    SPtr<ConnectionController> controller = m_connectionController;

    auto* connection = static_cast<BasicConnectionNode*>(m_connectionController->m_connections);
    while (nullptr != connection) {
      auto* next = static_cast<BasicConnectionNode*>(connection->m_next);
      if (nullptr != connection->m_function) {
        connection->m_function(forward<Args>(args)...);
      }
      connection = next;
    }
  }

  /** 
   *   Clears the whole list of subscribed connections. 
   **/
  FORCEINLINE void
  clear() {
    m_connectionController->clear();
  }

 private:
  SPtr<ConnectionController> m_connectionController;
};

/***************************************************************************/
/**                       SPECIALIZATIONS                                  */
/**   SO YOU MAY USE FUNCTION LIKE SYNTAX FOR DECLARING EVENT SIGNATURE    */
/***************************************************************************/

template<typename Signature>
class Event;

template<class ReturnType, class... Args>
class Event<ReturnType(Args...)> : public TEvent<ReturnType, Args...>
  {};


/************************************************************************/
/*
 * Implementation
 */                                                                     
/************************************************************************/

/*
*/
void
ConnectionController::connect(BaseConnectionNode* _connection) {
  _connection->m_prev = m_lastConnection;

  if (nullptr != m_lastConnection) {
    m_lastConnection->m_next = _connection;
  }

  m_lastConnection = _connection;

  //First connection
  if (nullptr == m_connections) {
    m_connections = _connection;
  }
}

/*
*/
void
ConnectionController::disconnect(BaseConnectionNode* _connection) {
  RecursiveLock lock(m_mutex);

  _connection->deactivate();
  --_connection->m_size;

  if (0 == _connection->m_size) {
    delete _connection;
  }
}

/*
*/
void
ConnectionController::clear() {
  RecursiveLock lock(m_mutex);

  BaseConnectionNode* conn = m_connections;
  while (nullptr != conn) {
    BaseConnectionNode* next = conn->m_next;
    conn->deactivate();

    if (0 == conn->m_size) {
      delete conn;
    }

    conn = next;
  }

  m_connections = nullptr;
  m_lastConnection = nullptr;
}

/*
*/
void
ConnectionController::freeHandle(BaseConnectionNode* conn) {
  RecursiveLock lock(m_mutex);

  --conn->m_size;
  if (0 == conn->m_size && !conn->m_isActive) {
    free(conn);
  }
}

/*
*/
void
ConnectionController::free(BaseConnectionNode* conn) {
  if (nullptr != conn->m_prev) {
    conn->m_prev->m_next = conn->m_next;
  }
  else {
    m_connections = conn->m_next;
  }

  if (nullptr != conn->m_next) {
    conn->m_next->m_prev = conn->m_prev;
  }
  else {
    m_lastConnection = conn->m_prev;
  }

  conn->m_prev = nullptr;
  conn->m_next = nullptr;

  if (nullptr != m_freeConnections) {
    conn->m_next = m_freeConnections;
    m_freeConnections->m_prev = conn;
  }

  m_freeConnections = conn;
  m_freeConnections->~BaseConnectionNode();
}

/*
*/
void
HEvent::disconnect() {
  if (nullptr != m_connection) {
    m_controller->disconnect(m_connection);
    m_connection = nullptr;
    m_controller = nullptr;
  }
}
}
