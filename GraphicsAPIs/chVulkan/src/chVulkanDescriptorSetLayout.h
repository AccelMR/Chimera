/************************************************************************/
/**
 * @file chVulkanDescriptorSetLayout.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan descriptor set layout class. This class is used to manage the
 *  descriptor set.
 * 
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIDescriptorSetLayout.h"

namespace chEngineSDK {
class VulkanDescriptorSetLayout : public IDescriptorSetLayout {
public:
  VulkanDescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo& createInfo);
  ~VulkanDescriptorSetLayout() override;

  NODISCARD FORCEINLINE VkDescriptorSetLayout
  getHandle() const { return m_layout; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};
} // namespace chEngineSDK