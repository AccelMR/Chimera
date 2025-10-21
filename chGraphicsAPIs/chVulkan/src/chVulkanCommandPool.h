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
#pragma once

#include "chVulkanPrerequisites.h"
#include "chICommandPool.h"

namespace chEngineSDK {
class VulkanCommandPool : public ICommandPool {
 public:
  VulkanCommandPool(VkDevice device,
                    uint32 queueFamilyIndex,
                    bool transient = false);

  ~VulkanCommandPool() override;

  NODISCARD SPtr<ICommandBuffer> 
  allocateCommandBuffer() override;

  void 
  reset() override;

  NODISCARD FORCEINLINE VkCommandPool
  getHandle() const { return m_commandPool; }

 private:
 VkDevice m_device;
 uint32 m_queueFamilyIndex = 0;
 VkCommandPool m_commandPool;
};
} // namespace chEngineSDK