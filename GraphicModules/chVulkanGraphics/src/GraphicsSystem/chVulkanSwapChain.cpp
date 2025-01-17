/************************************************************************/
/**
 * @file chVulkanSwapChain.cpp
 * @author AccelMR
 * @date 2022/10/22
 * 
 *  Vulkan specific implementation for chimera engine.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chVulkanSwapChain.h"

#include "chGPUResourceDescriptors.h"
#include "chScreen.h"
#include "chVulkanTranslator.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK{
using namespace chGPUDesc;

/*
*/
VulkanSwapChain::~VulkanSwapChain() {
}

/*
*/
void
VulkanSwapChain::init(const SwapChainDesc& desc) {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  VkSwapchainCreateInfoKHR swapchainInfo = {};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = rhi.getSurface();
  swapchainInfo.minImageCount = desc.frameCount;
  swapchainInfo.imageFormat = VulkanTranslator::get(desc.format);
  swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchainInfo.imageExtent.width = desc.width;
  swapchainInfo.imageExtent.height = desc.height;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = m_swapchain;

  throwIfFailed(vkCreateSwapchainKHR(rhi.getDevice(), &swapchainInfo, nullptr, &m_swapchain));

  createResources();
}

/*
*/
void
VulkanSwapChain::createResources() {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  const VkDevice& vulkanDevice = rhi.getDevice();

  uint32 imageCount = 0;
  vkGetSwapchainImagesKHR(vulkanDevice, m_swapchain, &imageCount, nullptr);
  m_images.resize(imageCount);
  vkGetSwapchainImagesKHR(vulkanDevice, m_swapchain, &imageCount, m_images.data());

  m_imageViews.resize(imageCount);
  for (uint32 i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    throwIfFailed(vkCreateImageView(vulkanDevice, &viewInfo, nullptr, &m_imageViews[i]));
  }

  m_framebuffers.resize(imageCount);
  for (uint32 i = 0; i < imageCount; i++) {
    VkImageView attachments[] = {m_imageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = VK_NULL_HANDLE;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    //framebufferInfo.width = rhi.getSwapchainExtent().width;
    //framebufferInfo.height = rhi.getSwapchainExtent().height;
    framebufferInfo.layers = 1;

    throwIfFailed(vkCreateFramebuffer(vulkanDevice, &framebufferInfo, nullptr, &m_framebuffers[i]));
  }
}

/*
*/
void
VulkanSwapChain::_internalResize(uint32 frameCount, 
                                 uint32 width,
                                 uint32 height,
                                 FORMAT format) {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  VkSwapchainKHR oldSwapchain = m_swapchain;

  VkSwapchainCreateInfoKHR swapchainInfo = {};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = rhi.getSurface();
  swapchainInfo.minImageCount = frameCount;
  swapchainInfo.imageFormat = VulkanTranslator::get(format);
  swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchainInfo.imageExtent.width = width;
  swapchainInfo.imageExtent.height = height;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = oldSwapchain;

  throwIfFailed(vkCreateSwapchainKHR(rhi.getDevice(), &swapchainInfo, nullptr, &m_swapchain));

  vkDestroySwapchainKHR(rhi.getDevice(), oldSwapchain, nullptr);

  createResources();
}

/*
*/
void
VulkanSwapChain::_internalPresent(int32 syncInterval, int32 flags) {

}

/*
*/
uint32
VulkanSwapChain::_internalGetCurrentFrameIndex() const {
  return 0;
}

/*
*/
SPtr<Texture>
VulkanSwapChain::getCurrentRenderTarget() const {
  return nullptr;
}

}


