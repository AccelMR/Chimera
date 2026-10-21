/************************************************************************/
/**
 * @file chEventSystem.cpp
 * @author AccelMR
 * @date 2022/06/15
 * @brief Locking and list handling of the event system.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chEventSystem.h"

#include "chSTDThreading.h"

namespace chEngineSDK {

// Recursive because a callback fired while a caller holds the lock can touch the same
// event again on that thread.
struct ConnectionController::Impl
{
  RecursiveMutex mutex;
};

/*
 */
ConnectionController::ConnectionController()
 : m_impl(chMakeUnique<Impl>())
{}

/*
 */
ConnectionController::~ConnectionController()
{
  clear();
}

/*
 */
void
ConnectionController::connect(BaseConnectionNode* connection)
{
  CH_ASSERT(nullptr != connection && "Connection must not be null.");
  RecursiveLock lock(m_impl->mutex);

  // The handle created for this connection is its only user.
  connection->m_size = 1;
  connection->m_prev = m_lastConnection;
  connection->m_next = nullptr;

  if (nullptr != m_lastConnection) {
    m_lastConnection->m_next = connection;
  }
  else {
    m_connections = connection;
  }

  m_lastConnection = connection;
}

/*
 */
void
ConnectionController::disconnect(BaseConnectionNode* connection)
{
  if (nullptr == connection) {
    return;
  }

  RecursiveLock lock(m_impl->mutex);

  // clear() already took inactive connections out of the list.
  if (connection->m_isActive) {
    connection->deactivate();
    removeFromList(connection);
  }

  CH_ASSERT(connection->m_size > 0);
  --connection->m_size;
  if (0 == connection->m_size) {
    delete connection;
  }
}

/*
 */
void
ConnectionController::clear()
{
  RecursiveLock lock(m_impl->mutex);

  BaseConnectionNode* connection = m_connections;
  while (nullptr != connection) {
    BaseConnectionNode* next = connection->m_next;
    connection->m_prev = nullptr;
    connection->m_next = nullptr;
    connection->deactivate();

    if (0 == connection->m_size) {
      delete connection;
    }

    connection = next;
  }

  m_connections = nullptr;
  m_lastConnection = nullptr;
}

/*
 */
ConnectionController::PinnedConnections::PinnedConnections(ConnectionController& controller,
                                                           BaseConnectionNode** inlineNodes,
                                                           uint32 inlineCapacity)
 : m_controller(controller),
   m_inlineNodes(inlineNodes),
   m_nodes(inlineNodes)
{
  RecursiveLock lock(controller.m_impl->mutex);

  uint32 activeCount = 0;
  for (BaseConnectionNode* node = controller.m_connections; nullptr != node;
       node = node->m_next) {
    if (node->m_isActive) {
      ++activeCount;
    }
  }

  if (activeCount > inlineCapacity) {
    m_nodes = new BaseConnectionNode*[activeCount];
  }

  for (BaseConnectionNode* node = controller.m_connections; nullptr != node;
       node = node->m_next) {
    if (node->m_isActive) {
      ++node->m_size;
      m_nodes[m_count++] = node;
    }
  }
}

/*
 */
ConnectionController::PinnedConnections::~PinnedConnections()
{
  // Events with no listeners are common (one event per key), so skip the lock.
  if (0 < m_count) {
    RecursiveLock lock(m_controller.m_impl->mutex);
    for (uint32 i = 0; i < m_count; ++i) {
      BaseConnectionNode* node = m_nodes[i];
      CH_ASSERT(node->m_size > 0);
      --node->m_size;
      if (0 == node->m_size) {
        delete node;
      }
    }
  }

  if (m_nodes != m_inlineNodes) {
    delete[] m_nodes;
  }
}

/*
 */
void
ConnectionController::removeFromList(BaseConnectionNode* connection)
{
  if (connection->m_prev) {
    connection->m_prev->m_next = connection->m_next;
  }
  else {
    m_connections = connection->m_next;
  }

  if (connection->m_next) {
    connection->m_next->m_prev = connection->m_prev;
  }
  else {
    m_lastConnection = connection->m_prev;
  }

  connection->m_prev = nullptr;
  connection->m_next = nullptr;
}

} // namespace chEngineSDK
