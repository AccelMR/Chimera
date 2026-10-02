/************************************************************************/
/**
 * @file chEventSystem.h
 * @author AccelMR
 * @date 2022/06/15
 * @brief Events that many listeners can subscribe to.
 *
 * The locking lives in chEventSystem.cpp so this header does not need <mutex>.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {
using std::forward;
using std::function;

/**
 * One subscriber of an event, stored in a double linked list.
 */
class BaseConnectionNode
{
 public:
  BaseConnectionNode() = default;

  virtual ~BaseConnectionNode()
  {
    CH_ASSERT(!m_size && !m_isActive);
  }

  FORCEINLINE void
  deactivate()
  {
    m_isActive = false;
  }

 public:
  BaseConnectionNode* m_prev = nullptr;
  BaseConnectionNode* m_next = nullptr;

  uint32 m_size = 0;
  bool m_isActive = true;
};

/**
 * Owns the subscriber list of one event and guards it with a mutex, so events can be
 * fired and connected from different threads.
 */
class CH_UTILITY_EXPORT ConnectionController
{
 public:
  /**
   * Locks the controller for as long as it lives.
   */
  class ScopedLock
  {
   public:
    explicit ScopedLock(ConnectionController& controller)
     : m_controller(controller)
    {
      m_controller.lock();
    }

    ~ScopedLock()
    {
      m_controller.unlock();
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock&
    operator=(const ScopedLock&) = delete;

   private:
    ConnectionController& m_controller;
  };

  ConnectionController();
  ~ConnectionController();

  ConnectionController(const ConnectionController&) = delete;
  ConnectionController&
  operator=(const ConnectionController&) = delete;

  /**
   * Adds the connection at the end of the list. The controller owns it from now on.
   */
  void
  connect(BaseConnectionNode* connection);

  /**
   * Called when a handle lets go of its connection. Removes it from the list and
   * deletes it once no handle uses it.
   */
  void
  disconnect(BaseConnectionNode* connection);

  /**
   * Removes every connection. Connections that still have a handle are deleted when
   * that handle disconnects.
   */
  void
  clear();

  void
  lock();

  void
  unlock();

  /**
   * First connection of the list. Only walk the list while the controller is locked.
   */
  NODISCARD FORCEINLINE BaseConnectionNode*
  getFirstConnection() const
  {
    return m_connections;
  }

 private:
  void
  removeFromList(BaseConnectionNode* connection);

  struct Impl;

  UniquePtr<Impl> m_impl;
  BaseConnectionNode* m_connections = nullptr;
  BaseConnectionNode* m_lastConnection = nullptr;
};

/**
 * Handle returned when subscribing to an event. The subscription ends when the handle
 * is destroyed or disconnected.
 */
class HEvent
{
 public:
  HEvent() = default;

  HEvent(const HEvent&) = delete;
  HEvent&
  operator=(const HEvent&) = delete;

  HEvent(HEvent&& other) noexcept
   : m_connection(other.m_connection),
     m_controller(std::move(other.m_controller))
  {
    other.m_connection = nullptr;
  }

  HEvent&
  operator=(HEvent&& other) noexcept
  {
    if (this != &other) {
      disconnect();
      m_connection = other.m_connection;
      m_controller = std::move(other.m_controller);
      other.m_connection = nullptr;
    }
    return *this;
  }

  HEvent(SPtr<ConnectionController> controller, BaseConnectionNode* connection)
   : m_connection(connection),
     m_controller(std::move(controller))
  {}

  ~HEvent()
  {
    disconnect();
  }

  FORCEINLINE void
  disconnect()
  {
    if (m_connection && m_controller) {
      m_controller->disconnect(m_connection);
    }
    m_connection = nullptr;
    m_controller = nullptr;
  }

  NODISCARD FORCEINLINE bool
  isValid() const
  {
    return nullptr != m_connection && nullptr != m_controller;
  }

 private:
  BaseConnectionNode* m_connection = nullptr;
  SPtr<ConnectionController> m_controller;
};

/**
 * Event with a list of subscribers that are all called when the event is fired.
 * Use it through Event<ReturnType(Args...)>.
 */
template<class ReturnType, class... Args>
class TEvent
{
 private:
  struct BasicConnectionNode : BaseConnectionNode
  {
    function<ReturnType(Args...)> m_function;
  };

 public:
  TEvent()
  {
    m_connectionController = chMakeShared<ConnectionController>();
  }

  ~TEvent()
  {
    clear();
  }

  NODISCARD HEvent
  connect(function<ReturnType(Args...)> func) const
  {
    auto* connData = new BasicConnectionNode();
    // Set before connecting, because once the node is in the list another thread may
    // fire the event and read it.
    connData->m_function = std::move(func);
    m_connectionController->connect(connData);
    return HEvent(m_connectionController, connData);
  }

  void
  operator()(Args... args) const
  {
    SPtr<ConnectionController> controller = m_connectionController;
    Vector<function<ReturnType(Args...)>> activeCallbacks;

    // Callbacks are copied and called after unlocking, so a callback can connect or
    // disconnect from this same event.
    {
      ConnectionController::ScopedLock lock(*controller);
      auto* connection = static_cast<BasicConnectionNode*>(controller->getFirstConnection());
      while (nullptr != connection) {
        if (connection->m_isActive && connection->m_function) {
          activeCallbacks.push_back(connection->m_function);
        }
        connection = static_cast<BasicConnectionNode*>(connection->m_next);
      }
    }

    for (auto& callback : activeCallbacks) {
      callback(forward<Args>(args)...);
    }
  }

  void
  clear()
  {
    m_connectionController->clear();
  }

 private:
  SPtr<ConnectionController> m_connectionController;
};

template<typename Signature>
class Event;

/**
 * Lets an event be declared with function syntax, e.g. Event<void(int32)>.
 */
template<class ReturnType, class... Args>
class Event<ReturnType(Args...)> : public TEvent<ReturnType, Args...>
{};

} // namespace chEngineSDK
