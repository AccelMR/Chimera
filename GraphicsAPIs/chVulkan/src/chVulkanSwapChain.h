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

  virtual bool
  acquireNextImage(SPtr<ISemaphore> waitSemaphore, 
                   SPtr<IFence> fence = nullptr) override;

  virtual void
  present(const Vector<SPtr<ISemaphore>>& waitSemaphores) override;
 
  virtual void
  resize(uint32 width, uint32 height) override;

  NODISCARD FORCEINLINE virtual uint32
  getCurrentImageIndex() const override { return m_currentImageIndex; }

  NODISCARD virtual SPtr<ITexture>
  getTexture(uint32 index) const override;

  NODISCARD virtual SPtr<ITextureView>
  getTextureView(uint32 index) const override;

  NODISCARD FORCEINLINE virtual SPtr<IRenderPass>
  getRenderPass() const override { return m_renderPass; }

  NODISCARD FORCEINLINE virtual SPtr<IFrameBuffer>
  getFramebuffer(uint32 index) const override { 
    CH_ASSERT(index < m_framebuffers.size());
    return m_framebuffers[index]; 
  }

  NODISCARD FORCEINLINE virtual uint32
  getTextureCount() const override { return m_imageCount; }

  NODISCARD FORCEINLINE virtual Format
  getFormat() const override { return vkFormatToChFormat(m_colorFormat); }

  NODISCARD FORCEINLINE virtual uint32
  getWidth() const override { return m_width; }

  NODISCARD FORCEINLINE virtual uint32
  getHeight() const override { return m_height; }

  NODISCARD FORCEINLINE virtual VkSwapchainKHR
  getHandle() const { return m_swapChain; }

 public:
  void
  create(uint32 width, uint32 height, bool vsync = false);

  void 
  cleanUpSwapChain();

  void
  createImageViews();

  void
  cleanUp();

  void
  createRenderPass();

  void
  createFramebuffers();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  uint32 m_graphicsFamilyQueueIndex = UINT32_MAX;
  uint32 m_presentFamilyQueueIndex = UINT32_MAX;
  VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;

  Vector<SPtr<IFrameBuffer>> m_framebuffers;
  SPtr<IRenderPass> m_renderPass;
 
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