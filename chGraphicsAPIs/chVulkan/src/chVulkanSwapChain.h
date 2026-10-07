/************************************************************************/
/**
 * @file chVulkanSwapChain.h
 * @author AccelMR
 * @date 2025/04/07
 * @details
 * SwapChain implementation for Vulkan.
 */
/************************************************************************/
#pragma once
#include "chVulkanPrerequisites.h"

#include "chISwapChain.h"
#include "chVulkanTexture.h"
#include "chVulkanTextureView.h"

namespace chEngineSDK {

/**
 * Vulkan swap chain of one window. It owns the surface made on the window and the
 * semaphores that tie its images to the frame: one per frame in flight that the acquire
 * signals and the frame submit waits on, and one per image that the submit signals and the
 * present waits on.
 */
class VulkanSwapChain : public ISwapChain
{
 public:
  VulkanSwapChain(VkInstance instance,
                  VkDevice device,
                  VkPhysicalDevice physicalDevice,
                  uint32 graphicsFamilyQueueIndex,
                  uint32 presentFamilyQueueIndex,
                  const SwapChainDesc& desc);

  ~VulkanSwapChain() override;

  VulkanSwapChain(const VulkanSwapChain&) = delete;
  VulkanSwapChain&
  operator=(const VulkanSwapChain&) = delete;

  NODISCARD SwapChainStatus
  acquireNextImage() override;

  NODISCARD SwapChainStatus
  present() override;

  void
  resize(uint32 width, uint32 height) override;

  NODISCARD const ITexture&
  getCurrentTexture() const override
  {
    return *m_textures[m_currentImageIndex];
  }

  NODISCARD const ITextureView&
  getCurrentTextureView() const override
  {
    return *m_textureViews[m_currentImageIndex];
  }

  NODISCARD uint32
  getTextureCount() const override
  {
    return m_imageCount;
  }

  NODISCARD Format
  getFormat() const override
  {
    return vkFormatToChFormat(m_colorFormat);
  }

  NODISCARD uint32
  getWidth() const override
  {
    return m_width;
  }

  NODISCARD uint32
  getHeight() const override
  {
    return m_height;
  }

  NODISCARD FORCEINLINE VkSemaphore
  getAcquireSemaphore() const
  {
    return m_acquireSemaphores[m_acquireSlot];
  }

  NODISCARD FORCEINLINE VkSemaphore
  getPresentSemaphore() const
  {
    return m_presentSemaphores[m_currentImageIndex];
  }

 private:
  void
  create(uint32 width, uint32 height, bool vsync);

  void
  createSurface(PlatformDisplay window);

  void
  cleanUpSwapChain();

  void
  createImageViews();

  NODISCARD VkSemaphore
  createSemaphore(const ANSICHAR* name) const;

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  uint32 m_graphicsFamilyQueueIndex = UINT32_MAX;
  uint32 m_presentFamilyQueueIndex = UINT32_MAX;
  VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
  bool m_vsync = false;
  VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_imageCount = 0;
  uint32 m_currentImageIndex = 0;
  // Frame slot of the last acquire, which picks its semaphore.
  uint32 m_acquireSlot = 0;
  String m_debugName;

  Vector<VkImage> m_images;
  Vector<VkImageView> m_imageViews;
  // Made once per image, so a frame hands them out by reference without allocating.
  Vector<UniquePtr<VulkanTexture>> m_textures;
  Vector<UniquePtr<VulkanTextureView>> m_textureViews;
  Vector<VkSemaphore> m_presentSemaphores;
  Array<VkSemaphore, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_acquireSemaphores{};
};
} // namespace chEngineSDK
