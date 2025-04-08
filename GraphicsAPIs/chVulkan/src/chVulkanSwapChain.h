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
  VulkanSwapChain() = default;
  ~VulkanSwapChain() override;

  void
  createSwapChain(uint32 width, uint32 height, bool vsync = false) override;

  void
  present() override;

  void
  cleanUp() override;

  void
  resize(uint32 width, uint32 height) override;

  void*
  getNativeHandle() const override;

  void
  createSurface(VkPhysicalDevice physicalDevice, WeakPtr<DisplaySurface> display);

 private:
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  VkFormat m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR m_colorSpace;

  uint32 m_queueNodeIndex = 0;
};
} // namespace chEngineSDK