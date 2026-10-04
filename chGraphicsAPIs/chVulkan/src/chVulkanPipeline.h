/************************************************************************/
/**
 * @file chVulkanPipeline.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of IPipeline.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIPipeline.h"

namespace chEngineSDK {

/**
 * Graphics pipeline built for dynamic rendering (attachment formats instead of a render
 * pass) on the pipeline layout of the bindless heap, which it does not own.
 */
class VulkanPipeline : public IPipeline
{
 public:
  VulkanPipeline(VkDevice device, VkPipelineLayout layout, const GraphicsPipelineDesc& desc);
  ~VulkanPipeline() override;

  NODISCARD FORCEINLINE VkPipeline
  getHandle() const
  {
    return m_pipeline;
  }

 private:
  VkPipeline m_pipeline = VK_NULL_HANDLE;
};

} // namespace chEngineSDK
