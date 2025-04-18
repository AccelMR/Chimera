/************************************************************************/
/**
 * @file chVulkanDescriptorSet.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan descriptor set class. This class is used to manage the
 *  descriptor set.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIDescriptorSet.h"

namespace chEngineSDK {
class VulkanDescriptorSet : public IDescriptorSet {
public:
  VulkanDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet);
  ~VulkanDescriptorSet() override;

  NODISCARD FORCEINLINE VkDescriptorSet
  getHandle() const { return m_descriptorSet; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkDescriptorPool m_pool = VK_NULL_HANDLE;
  VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};
} // namespace chEngineSDK