/************************************************************************/
/**
 * @file chVulkanSampler.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan sampler class. This class is used to manage the Vulkan sampler
 * and its properties. It is used to create samplers and allocate them.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chISampler.h"

namespace chEngineSDK {
class VulkanSampler : public ISampler {
 public:
  VulkanSampler(VkDevice device, const SamplerCreateInfo& createInfo);
  ~VulkanSampler() override;

  NODISCARD FORCEINLINE VkSampler
  getHandle() const { return m_sampler; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkSampler m_sampler = VK_NULL_HANDLE;
};
} // namespace chEngineSDK