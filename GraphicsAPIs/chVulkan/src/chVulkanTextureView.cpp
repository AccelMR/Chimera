/************************************************************************/
/**
 * @file chVulkanTextureView.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan texture view implementation.
 * This file contains the implementation of the texture
 * view interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanTextureView.h"

#include "chVulkanTexture.h"
#include "chVulkanAPI.h"


namespace chEngineSDK {
/*
*/
VulkanTextureView::VulkanTextureView(VkDevice device,
                                       ITexture* texture)
    : m_device(device),
      m_texture(texture) {
  CH_ASSERT(m_texture != nullptr);
}

/*
*/
VulkanTextureView::~VulkanTextureView() {
  if (m_imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device, m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }
}

} // namespace chEngineSDK