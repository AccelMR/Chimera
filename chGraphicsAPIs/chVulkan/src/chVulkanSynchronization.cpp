/************************************************************************/
/**
 * @file chVulkanSynchronization.cpp
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Vulkan synchronization implementation.
 * This file contains the implementation of the synchronization
 * primitives for Vulkan.
 */
/************************************************************************/

#include "chVulkanSynchronization.h"

#include "chVulkanAPI.h"

CH_LOG_DECLARE_STATIC(VulkanSynchronization, All);

namespace chEngineSDK {
/*
*/
VulkanSemaphore::VulkanSemaphore(VkDevice device)
    : m_device(device) {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphore));
}

/*
*/
VulkanSemaphore::~VulkanSemaphore() {
  if (m_semaphore != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_device);
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
    m_semaphore = VK_NULL_HANDLE;
  }
}

/*
*/
VulkanFence::VulkanFence(VkDevice device, bool signaled)
    : m_device(device) {
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

  VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_fence));
}

/*
*/
VulkanFence::~VulkanFence() {
  if (m_fence != VK_NULL_HANDLE) {
    vkDestroyFence(m_device, m_fence, nullptr);
    m_fence = VK_NULL_HANDLE;
  }
}

/*
*/
bool
VulkanFence::wait(uint64 timeout) {
  if (m_fence != VK_NULL_HANDLE) {
    return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout) == VK_SUCCESS;
  }
  CH_LOG_ERROR(VulkanSynchronization, "VulkanFence::wait called on an uninitialized fence.");
  return false;
}

/*
*/
void
VulkanFence::reset() {
  if (m_fence != VK_NULL_HANDLE) {
    vkResetFences(m_device, 1, &m_fence);
  }
}

/*
*/
bool
VulkanFence::isSignaled() const {
  if (m_fence != VK_NULL_HANDLE) {
    return vkGetFenceStatus(m_device, m_fence) == VK_SUCCESS;
  }
  return false;
}

} // namespace chEngineSDK
