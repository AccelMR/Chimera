/************************************************************************/
/**
 * @file chVulkanFence.cpp
 * @author AccelMR
 * @date 2025/01/27
 * 
 *  Vulkan Fence implementation.
 *
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanFence.h"

namespace chEngineSDK {
/*
*/
VulkanFence::VulkanFence(VkDevice device)
    : m_device(device), m_fence(nullptr) {
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = 0;
  throwIfFailed(vkCreateFence(m_device, &fenceInfo, nullptr, &m_fence));
}

/*
*/
VulkanFence::~VulkanFence() {
  if (m_fence) {
    vkDestroyFence(m_device, m_fence, nullptr);
  }
}

/*
*/
void
VulkanFence::_internalWait(uint64 timeout) {
  vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout);
}

/*
*/
void
VulkanFence::_internalReset() {
  throwIfFailed(vkResetFences(m_device, 1, &m_fence));
}

/*
*/
bool
VulkanFence::_internalIsSignaled() const {
  return VK_SUCCESS == vkGetFenceStatus(m_device, m_fence);
}


} // namespace chEngineSDK