/************************************************************************/
/**
 * @file chVulkanPipeline.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan pipeline implementation.
 * This file contains the implementation of the pipeline interface
 * for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chIPipeline.h"

namespace chEngineSDK {
class VulkanPipeline : public IPipeline {
 public:
  VulkanPipeline(VkDevice device, const PipelineCreateInfo& createInfo);
  ~VulkanPipeline() override;

  NODISCARD FORCEINLINE virtual VkPipeline
  getHandle() const {
    return m_pipeline;
  }

  NODISCARD FORCEINLINE virtual VkPipelineLayout
  getLayout() const {
    return m_pipelineLayout;
  }

 private:
  VkPipeline m_pipeline = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
};
} // namespace chEngineSDK