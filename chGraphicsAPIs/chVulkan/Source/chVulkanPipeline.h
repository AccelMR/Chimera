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
 * pass), or compute pipeline, on the pipeline layout of the bindless heap, which it does
 * not own.
 */
class VulkanPipeline : public IPipeline
{
 public:
  VulkanPipeline(VkDevice device, VkPipelineLayout layout, const GraphicsPipelineDesc& desc);
  VulkanPipeline(VkDevice device, VkPipelineLayout layout, const ComputePipelineDesc& desc);
  ~VulkanPipeline() override;

  NODISCARD FORCEINLINE VkPipeline
  getHandle() const
  {
    return m_pipeline;
  }

  NODISCARD FORCEINLINE VkPipelineBindPoint
  getBindPoint() const
  {
    return m_bindPoint;
  }

 private:
  VkPipeline m_pipeline = VK_NULL_HANDLE;
  VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

} // namespace chEngineSDK
