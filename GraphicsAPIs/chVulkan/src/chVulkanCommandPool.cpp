/************************************************************************/
/**
 * @file chVulkanCommandPool.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan command pool implementation.
 * This file contains the implementation of the command pool interface
 * for Vulkan.
 */
/************************************************************************/

#include "chVulkanCommandPool.h"
#include "chVulkanCommandBuffer.h"

#include "chVulkanAPI.h"

namespace chEngineSDK {

/*
*/
VulkanCommandPool::VulkanCommandPool(VkDevice device, uint32 queueFamilyIndex, bool transient)
    : m_device(device), m_queueFamilyIndex(queueFamilyIndex) {
  VkCommandPoolCreateFlags flags = transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
  flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkCommandPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = flags,
    .queueFamilyIndex = queueFamilyIndex,
  };

  VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool));
}

/*
*/
VulkanCommandPool::~VulkanCommandPool() {
  if (m_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }
}

/*
*/
SPtr<ICommandBuffer>
VulkanCommandPool::allocateCommandBuffer() {
  return chMakeShared<VulkanCommandBuffer>(m_device, m_commandPool);
}

/*
*/
void
VulkanCommandPool::reset() {
  VK_CHECK(vkResetCommandPool(m_device, m_commandPool, 0));
}

} // namespace chEngineSDK