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
                                     ITexture* texture,
                                     const TextureViewCreateInfo& createInfo)
    : m_device(device),
      m_texture(texture),
      m_ownsTextureView(true) {
  CH_ASSERT(m_device != VK_NULL_HANDLE);
  CH_ASSERT(m_texture != nullptr);

  VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(m_texture);

  VkImageViewCreateInfo viewInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = vulkanTexture->getHandle(),
  };

  viewInfo.viewType = chTextureViewTypeToVkTextureViewType(createInfo.viewType);

  //If unknown, use the texture format
  Format format = (createInfo.format != Format::Unknown) ? 
                   createInfo.format : vulkanTexture->getFormat();
  viewInfo.format = chFormatToVkFormat(format);

  viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  viewInfo.subresourceRange.baseMipLevel = createInfo.baseMipLevel;
  viewInfo.subresourceRange.levelCount = createInfo.mipLevelCount == ~0u ?
                                          vulkanTexture->getMipLevels() :
                                          createInfo.mipLevelCount;

  viewInfo.subresourceRange.baseArrayLayer = createInfo.baseArrayLayer;
  viewInfo.subresourceRange.layerCount = createInfo.arrayLayerCount == ~0u ?
                                          vulkanTexture->getArrayLayers() :
                                          createInfo.arrayLayerCount;

  viewInfo.subresourceRange.baseArrayLayer = createInfo.baseArrayLayer;
  viewInfo.subresourceRange.layerCount = createInfo.arrayLayerCount == ~0u ?
                                          vulkanTexture->getArrayLayers() :
                                          createInfo.arrayLayerCount;

  VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView));

  m_format = viewInfo.format;
  m_viewType = viewInfo.viewType;
  m_baseMipLevel = viewInfo.subresourceRange.baseMipLevel;
  m_mipLevelCount = viewInfo.subresourceRange.levelCount;
  m_baseArrayLayer = viewInfo.subresourceRange.baseArrayLayer;
  m_arrayLayerCount = viewInfo.subresourceRange.layerCount;
}

/*
*/
VulkanTextureView::~VulkanTextureView() {
  if (m_ownsTextureView && m_imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device, m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }
}

} // namespace chEngineSDK