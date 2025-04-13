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

#include "chVulkanCommandBuffer.h"
#include "chVulkanCommandPool.h"
#include "chVulkanSynchronization.h"

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
VulkanCommandQueue::submit(const SubmitInfo& submitInfo, const SPtr<IFence>& fence) {
  Vector<VkCommandBuffer> vkCommandBuffers;
  Vector<VkSemaphore> vkWaitSemaphores;
  Vector<VkSemaphore> vkSignalSemaphores;
  Vector<VkPipelineStageFlags> vkWaitStages;
  
  for (const auto& cmdBuffer : submitInfo.commandBuffers) {
      auto vulkanCmdBuffer = std::static_pointer_cast<VulkanCommandBuffer>(cmdBuffer);
      vkCommandBuffers.push_back(vulkanCmdBuffer->getHandle());
  }
  
  for (const auto& sem : submitInfo.waitSemaphores) {
      auto vulkanSem = std::static_pointer_cast<VulkanSemaphore>(sem);
      vkWaitSemaphores.push_back(vulkanSem->getHandle());
  }
  
  for (const auto& sem : submitInfo.signalSemaphores) {
      auto vulkanSem = std::static_pointer_cast<VulkanSemaphore>(sem);
      vkSignalSemaphores.push_back(vulkanSem->getHandle());
  }
  
  for (const auto& stage : submitInfo.waitStages) {
      vkWaitStages.push_back(pipelineStageToVkPipelineStage(stage));
  }
  
  VkSubmitInfo vkSubmitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32>(vkWaitSemaphores.size()),
      .pWaitSemaphores = vkWaitSemaphores.empty() ? nullptr : vkWaitSemaphores.data(),
      .pWaitDstStageMask = vkWaitStages.empty() ? nullptr : vkWaitStages.data(),
      .commandBufferCount = static_cast<uint32>(vkCommandBuffers.size()),
      .pCommandBuffers = vkCommandBuffers.data(),
      .signalSemaphoreCount = static_cast<uint32>(vkSignalSemaphores.size()),
      .pSignalSemaphores = vkSignalSemaphores.empty() ? nullptr : vkSignalSemaphores.data()
  };
  
  VkFence vkFence = VK_NULL_HANDLE;
  if (fence) {
      auto vulkanFence = std::static_pointer_cast<VulkanFence>(fence);
      vkFence = vulkanFence->getHandle();
  }
  
  VK_CHECK(vkQueueSubmit(m_queue, 1, &vkSubmitInfo, vkFence));
}

/*
*/
void
VulkanCommandQueue::waitIdle() {
  if (m_queue == VK_NULL_HANDLE) {
      CH_LOG_ERROR("VulkanCommandQueue::waitIdle: Queue is null.");
      return;
  }
  VK_CHECK(vkQueueWaitIdle(m_queue));
}
} // namespace chEngineSDK
