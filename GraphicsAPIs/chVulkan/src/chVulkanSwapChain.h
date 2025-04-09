/************************************************************************/
/**
 * @file chVulkanSwapChain.h
 * @author AccelMR
 * @date 2025/04/07
 * @details 
 * SwapChain implementation for Vulkan.
 * This class is used to create and manage the swap chain.
 * It is used to create the swap chain, and to present the swap chain.
 * It is used by the graphics API to create the swap chain.
 */
/************************************************************************/
#pragma once
#include "chVulkanPrerequisites.h"

#include "chISwapChain.h"

namespace chEngineSDK {
class VulkanSwapChain : public ISwapChain {
 public:
  VulkanSwapChain(VkDevice device, 
                  VkPhysicalDevice physicalDevice, 
                  VkSurfaceKHR surface,
                  VkFormat colorFormat,
                  VkColorSpaceKHR colorSpace,
                  uint32 graphicsFamilyQueueIndex,
                  uint32 presentFamilyQueueIndex);

  ~VulkanSwapChain() override;

  void
  create(uint32 width, uint32 height, bool vsync = false) override;

  void
  present() override;

  void
  cleanUp() override;

  void
  resize(uint32 width, uint32 height) override;

  void*
  getNativeHandle() const override;

  void 
  cleanUpSwapChain();

  void
  createImageViews();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  uint32 m_graphicsFamilyQueueIndex = UINT32_MAX;
  uint32 m_presentFamilyQueueIndex = UINT32_MAX;
  VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
 
  VkFormat m_colorFormat;
  VkColorSpaceKHR m_colorSpace;
 
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_imageCount = 0;
  uint32 m_currentImageIndex = 0;
 
  Vector<VkImage> m_images;
  Vector<VkImageView> m_imageViews;
 
  VkPresentInfoKHR m_presentInfo = {};
};
} // namespace chEngineSDKs