/************************************************************************/
/**
 * @file chVulkanSynchronization.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Vulkan synchronization implementation.
 * This file contains the implementation of the synchronization
 * primitives for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chISynchronization.h"

namespace chEngineSDK {

class VulkanSemaphore : public ISemaphore {
 public:
  VulkanSemaphore(VkDevice device);
  ~VulkanSemaphore() override;

  NODISCARD FORCEINLINE VkSemaphore
  getHandle() const { return m_semaphore; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkSemaphore m_semaphore = VK_NULL_HANDLE;
};

class VulkanFence : public IFence {
 public:
  VulkanFence(VkDevice device, bool signaled = false);
  ~VulkanFence() override;

  bool
  wait(uint64 timeout = UINT64_MAX) override;

  void
  reset() override;

  NODISCARD bool
  isSignaled() const override;

  NODISCARD FORCEINLINE VkFence
  getHandle() const { return m_fence; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkFence m_fence = VK_NULL_HANDLE;
};

} // namespace chEngineSDK
