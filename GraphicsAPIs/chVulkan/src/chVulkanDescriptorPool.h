/************************************************************************/
/**
 * @file chVulkanDescriptorPool.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan Descriptor pool class. This class is used to manage descriptor sets
 *  and allocate them from the pool.
 * 
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIDescriptorPool.h"

namespace chEngineSDK {
class VulkanDescriptorPool : public IDescriptorPool {
public:
  VulkanDescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& createInfo);
  ~VulkanDescriptorPool() override;

  SPtr<IDescriptorSet> 
  allocateDescriptorSet(const DescriptorSetAllocateInfo& allocInfo) override;

  NODISCARD FORCEINLINE VkDescriptorPool
  getHandle() const { return m_pool; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkDescriptorPool m_pool = VK_NULL_HANDLE;
};
} // namespace chEngineSDK