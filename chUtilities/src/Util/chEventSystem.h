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
   * The active connections of one fire. Each one is pinned so it is not deleted while
   * its callback runs, even if a callback disconnects it, and released when this object
   * is destroyed. The caller gives a buffer, usually on the stack, and the heap is only
   * used when there are more active connections than fit in it.
   */
  class CH_UTILITY_EXPORT PinnedConnections
  {
   public:
    PinnedConnections(ConnectionController& controller,
                      BaseConnectionNode** inlineNodes,
                      uint32 inlineCapacity);
    ~PinnedConnections();

    PinnedConnections(const PinnedConnections&) = delete;
    PinnedConnections&
    operator=(const PinnedConnections&) = delete;

    NODISCARD FORCEINLINE BaseConnectionNode* const*
    begin() const noexcept
    {
      return m_nodes;
    }

    NODISCARD FORCEINLINE BaseConnectionNode* const*
    end() const noexcept
    {
      return m_nodes + m_count;
    }

   private:
    ConnectionController& m_controller;
    BaseConnectionNode** m_inlineNodes;
    BaseConnectionNode** m_nodes;
    uint32 m_count = 0;
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
   * deletes it once neither a handle nor a running fire uses it.
   */
  void
  disconnect(BaseConnectionNode* connection);

  /**
   * Removes every connection. Connections that still have a handle are deleted when
   * that handle disconnects.
   */
  void
  clear();

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
template<uint32 InlineCapacity, class ReturnType, class... Args>
class TEvent
{
  static_assert(InlineCapacity > 0, "An event needs room for at least one listener.");

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

  // A copy would share the subscriber list, and destroying it would clear the list of
  // every other copy.
  TEvent(const TEvent&) = delete;
  TEvent&
  operator=(const TEvent&) = delete;

  TEvent(TEvent&& other) noexcept
   : m_connectionController(std::move(other.m_connectionController))
  {}

  TEvent&
  operator=(TEvent&& other) noexcept
  {
    if (this != &other) {
      clear();
      m_connectionController = std::move(other.m_connectionController);
    }
    return *this;
  }

  NODISCARD HEvent
  connect(function<ReturnType(Args...)> func) const
  {
    CH_ASSERT(m_connectionController && "Event used after being moved from.");
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
    CH_ASSERT(m_connectionController && "Event used after being moved from.");
    // Keeps the controller alive in case a callback destroys this event.
    SPtr<ConnectionController> controller = m_connectionController;

    // Callbacks run after the list is unlocked, so a callback can connect or disconnect
    // from this same event.
    BaseConnectionNode* inlineNodes[InlineCapacity];
    ConnectionController::PinnedConnections connections(*controller, inlineNodes,
                                                        InlineCapacity);
    for (BaseConnectionNode* connection : connections) {
      auto& callback = static_cast<BasicConnectionNode*>(connection)->m_function;
      if (callback) {
        // Not forwarded, because a moved argument would reach the next callbacks empty.
        callback(args...);
      }
    }
  }

  void
  clear()
  {
    if (m_connectionController) {
      m_connectionController->clear();
    }
  }

 private:
  SPtr<ConnectionController> m_connectionController;
};

template<typename Signature, uint32 InlineCapacity = 16>
class Event;

/**
 * Lets an event be declared with function syntax, e.g. Event<void(int32)>.
 * InlineCapacity is how many listeners a fire handles without a heap allocation; raise
 * it for events that are expected to have many listeners, e.g. Event<void(int32), 64>.
 */
template<class ReturnType, class... Args, uint32 InlineCapacity>
class Event<ReturnType(Args...), InlineCapacity>
 : public TEvent<InlineCapacity, ReturnType, Args...>
{};

} // namespace chEngineSDK
