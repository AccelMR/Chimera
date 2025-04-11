/************************************************************************/
/**
 * @file chVulkanCommandQueue.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan command queue implementation.
 * This file contains the implementation of the command queue
 * interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanCommandQueue.h"

namespace chEngineSDK {
/*
*/
VulkanCommandQueue::VulkanCommandQueue(VkDevice device, 
                                       uint32 graphicsQueueIndex, 
                                       QueueType queueType)
    : m_device(device), 
      m_graphicsQueueFamilyIndex(graphicsQueueIndex),
      m_queueType(queueType) {
  vkGetDeviceQueue(m_device, static_cast<uint32>(queueType), 0, &m_queue);
}

/*
*/
VulkanCommandQueue::~VulkanCommandQueue() {
  // No need to destroy the queue, it is managed by the device.
}

/*
*/
void
VulkanCommandQueue::submit(const Vector<SPtr<ICommandBuffer>>& commandBuffers, 
                             const SPtr<IFence>& fence) {
}

/*
*/
void
VulkanCommandQueue::waitIdle() {
  VK_CHECK(vkQueueWaitIdle(m_queue));
}
} // namespace chEngineSDK
