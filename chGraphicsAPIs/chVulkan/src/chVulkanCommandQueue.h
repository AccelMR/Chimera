/************************************************************************/
/**
 * @file chVulkanCommandQueue.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan command queue implementation.
 * This file contains the implementation of the command queue
 * interface for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chICommandQueue.h"

namespace chEngineSDK {
class VulkanCommandQueue : public ICommandQueue {
 public:
  VulkanCommandQueue(VkDevice device, uint32 graphicsQueueIndex, QueueType queueType);
  ~VulkanCommandQueue() override;

  NODISCARD QueueType
  getQueueType() const override { return m_queueType; }

  void 
  submit(const SubmitInfo& submitInfo, const SPtr<IFence>& fence) override;

  void 
  waitIdle() override;

  NODISCARD VkQueue
  getHandle() const { return m_queue; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueue m_queue = VK_NULL_HANDLE;
  QueueType m_queueType = QueueType::Graphics;
  uint32 m_graphicsQueueFamilyIndex = 0;
};
} // namespace chEngineSDK