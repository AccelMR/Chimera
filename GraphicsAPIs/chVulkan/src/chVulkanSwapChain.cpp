/************************************************************************/
/**
 * @file chVulkanSwapChain.cpp
 * @author AccelMR
 * @date 2025/04/07
 * @details 
 * SwapChain implementation for Vulkan.
 * This class is used to create and manage the swap chain.
 * It is used to create the swap chain, and to present the swap chain.
 * It is used by the graphics API to create the swap chain.
 */
/************************************************************************/
#include "chVulkanSwapChain.h"

#include "chDebug.h"
#include "chMath.h"
#include "chVulkanAPI.h"

namespace chEngineSDK {
/*
*/
VulkanSwapChain::VulkanSwapChain(VkDevice device, 
                                   VkPhysicalDevice physicalDevice, 
                                   VkSurfaceKHR surface,
                                   VkFormat colorFormat,
                                   VkColorSpaceKHR colorSpace,
                                   uint32 graphicsFamilyQueueIndex,
                                   uint32 presentFamilyQueueIndex)
  : m_device(device),
    m_physicalDevice(physicalDevice),
    m_surface(surface),
    m_colorFormat(colorFormat),
    m_colorSpace(colorSpace),
    m_graphicsFamilyQueueIndex(graphicsFamilyQueueIndex),
    m_presentFamilyQueueIndex(presentFamilyQueueIndex) {
  CH_ASSERT(m_device != VK_NULL_HANDLE);
  CH_ASSERT(m_physicalDevice != VK_NULL_HANDLE);
  CH_ASSERT(m_surface != VK_NULL_HANDLE);
}

/*
*/
VulkanSwapChain::~VulkanSwapChain() {
  cleanUp();
}

/*
*/
void
VulkanSwapChain::create(uint32 width, uint32 height, bool vsync) {
  cleanUpSwapChain();

  VkSurfaceCapabilitiesKHR capabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, 
                                                     m_surface, 
                                                     &capabilities));

  VkExtent2D swapChainExtent = {};
  if (capabilities.currentExtent.width == UINT32_MAX) {
    swapChainExtent.width = Math::clamp(width, 
                                        capabilities.minImageExtent.width, 
                                        capabilities.maxImageExtent.width);
    swapChainExtent.height = Math::clamp(height, 
                                         capabilities.minImageExtent.height, 
                                         capabilities.maxImageExtent.height);
  } 
  else {
    swapChainExtent = capabilities.currentExtent;
  }

  m_width = swapChainExtent.width;
  m_height = swapChainExtent.height;

  uint32 presentModeCount = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, 
                                                     m_surface, 
                                                     &presentModeCount, 
                                                     nullptr));

  Vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, 
                                                     m_surface, 
                                                     &presentModeCount, 
                                                     presentModes.data()));

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  if (!vsync) {
    // Prefer MAILBOX (triple buffering) if it is available
    auto it = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
    if (it != presentModes.end()) {
      presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    } 
    else {
      // Fallback to IMMEDIATE (no VSync)
      it = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR);
      if (it != presentModes.end()) {
          presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  m_presentMode = presentMode;

  uint32 imageCount = 3; // Triple buffering
  imageCount = Math::clamp(imageCount, capabilities.minImageCount,
                           capabilities.maxImageCount > 0 ? 
                            capabilities.maxImageCount : imageCount); 

  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = m_surface,
    .minImageCount = imageCount,
    .imageFormat = m_colorFormat,
    .imageColorSpace = m_colorSpace,
    .imageExtent = swapChainExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE
  };

  if (m_graphicsFamilyQueueIndex != m_presentFamilyQueueIndex) {
    uint32 queueFamilyIndices[] = { m_graphicsFamilyQueueIndex, m_presentFamilyQueueIndex };
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } 
  else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain));
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount, nullptr));
  m_images.resize(m_imageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount, m_images.data()));
  m_imageViews.resize(m_imageCount);

  createImageViews();

  CH_LOG_DEBUG(StringUtils::format("SweapChain created ({0}x{1}, {2} images, mode {3})", 
                                    m_width, m_height, m_imageCount, presentMode));
}

/*
*/
void
VulkanSwapChain::present() {
  VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .swapchainCount = 1,
    .pSwapchains = &m_swapChain,
    .pImageIndices = &m_currentImageIndex,
    .pResults = nullptr
  };

  VkQueue presentQueue =  g_vulkanAPI().getGraphicsQueue();
  VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    // Swap chain is no longer valid, recreate it
    create(m_width, m_height, (m_presentMode != VK_PRESENT_MODE_FIFO_KHR));
  } 
  else if (result != VK_SUCCESS) {
    CH_LOG_ERROR("Failed to present swap chain image");
  }
} 

/*
*/
void
VulkanSwapChain::cleanUp() {
  VkDevice device = g_vulkanAPI().getDevice();
  VkInstance instance = g_vulkanAPI().getInstance();

  if (m_swapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
  }

  if (m_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanSwapChain::resize(uint32 width, uint32 height) {
  if (width == 0 || height == 0) {
    CH_LOG_WARNING("Intento de resize a 0x0 - ignorado");
    return;
  }
  create(width, height, (m_presentMode != VK_PRESENT_MODE_FIFO_KHR));
}

/*
*/
void*
VulkanSwapChain::getNativeHandle() const {
  return reinterpret_cast<void*>(m_swapChain);
}

/*
*/
void
VulkanSwapChain::cleanUpSwapChain() {
  vkDeviceWaitIdle(m_device);
  for (auto& imageView : m_imageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }
  m_imageViews.clear();
  if (m_swapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanSwapChain::createImageViews() {
  m_imageViews.resize(m_imageCount);
  for (uint32 i = 0; i < m_imageCount; i++) {
    VkImageViewCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = m_images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = m_colorFormat,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };
    VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]));
  }
}

} // namespace chEngineSDK