/************************************************************************/
/**
 * @file chVulkanRenderPass.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief 
 * Vulkan render pass implementation.
 * This file contains the implementation of the render pass
 * interface for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chIRenderPass.h"

namespace chEngineSDK {
class VulkanRenderPass : public IRenderPass {
 public:
  VulkanRenderPass(VkDevice device, const RenderPassCreateInfo& createInfo);
  ~VulkanRenderPass() override;

  NODISCARD VkRenderPass
  getHandle() const { return m_renderPass; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
};
} // namespace chEngineSDK