/************************************************************************/
/**
 * @file chVulkanRenderPass.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief 
 * Vulkan render pass implementation.
 * This file contains the implementation of the render pass
 * interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanRenderPass.h"


namespace chEngineSDK {
/*
*/
VulkanRenderPass::VulkanRenderPass(VkDevice device, 
                                   const RenderPassCreateInfo& createInfo)
    : m_device(device) {
}



} // namespace chEngineSDK