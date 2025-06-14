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

#include "chMath.h"
#include "chVulkanAPI.h"
#include "chVulkanSynchronization.h"
#include "chVulkanCommandQueue.h"
#include "chVulkanTexture.h"
#include "chVulkanTextureView.h"
#include "chVulkanRenderPass.h"

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
    m_swapChain(VK_NULL_HANDLE),
    m_surface(surface),
    m_graphicsFamilyQueueIndex(graphicsFamilyQueueIndex),
    m_presentFamilyQueueIndex(presentFamilyQueueIndex),
    m_presentMode(VK_PRESENT_MODE_FIFO_KHR),
    m_renderPass(nullptr),
    m_colorFormat(colorFormat),
    m_colorSpace(colorSpace),
    m_width(0),
    m_height(0),
    m_imageCount(0),
    m_currentImageIndex(0) {
  CH_ASSERT(m_device != VK_NULL_HANDLE);
  CH_ASSERT(m_physicalDevice != VK_NULL_HANDLE);
  CH_ASSERT(m_surface != VK_NULL_HANDLE);
  m_imageViews.clear();
  m_images.clear();
  m_framebuffers.clear();
}

/*
*/
VulkanSwapChain::~VulkanSwapChain() {
  cleanUp();
}

/*
*/
bool
VulkanSwapChain::acquireNextImage(SPtr<ISemaphore> signalSemaphore, SPtr<IFence> fence) {
  auto vulkanSemaphore = std::static_pointer_cast<VulkanSemaphore>(signalSemaphore);

  VkFence vkFence = VK_NULL_HANDLE;
  if (fence) {
      auto vulkanFence = std::static_pointer_cast<VulkanFence>(fence);
      vkFence = vulkanFence->getHandle();
  }

  VkResult result = vkAcquireNextImageKHR(m_device,
                                          m_swapChain,
                                          UINT64_MAX,
                                          vulkanSemaphore->getHandle(),
                                          vkFence,
                                          &m_currentImageIndex);

  if (result != VK_SUCCESS) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      return false;
    }
    CH_LOG_WARNING(Vulkan, "Failed to acquire next image from swap chain");
  }
  return true;
}

/*
*/
void
VulkanSwapChain::create(uint32 width, uint32 height, bool vsync) {
  //VkSwapchainKHR oldSwapChain = m_swapChain;
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

  constexpr uint32 imageDefaultCount = 3; // Triple buffering
  if (capabilities.maxImageCount == 0) {
    m_imageCount = Math::max(imageDefaultCount, capabilities.minImageCount);
  }
  else {
    m_imageCount = Math::clamp(imageDefaultCount,
                               capabilities.minImageCount,
                               capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .surface = m_surface,
    .minImageCount = m_imageCount,
    .imageFormat = m_colorFormat,
    .imageColorSpace = m_colorSpace,
    .imageExtent = swapChainExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
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

  createRenderPass();
  createFramebuffers();
}

/*
*/
void
VulkanSwapChain::present(const Vector<SPtr<ISemaphore>>& waitSemaphores) {
  Vector<VkSemaphore> vkSemaphores;
  for (const auto& sem : waitSemaphores) {
    auto vulkanSem = std::static_pointer_cast<VulkanSemaphore>(sem);
    vkSemaphores.push_back(vulkanSem->getHandle());
  }

  VkPresentInfoKHR presentInfo{
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,
    .waitSemaphoreCount = static_cast<uint32>(vkSemaphores.size()),
    .pWaitSemaphores = vkSemaphores.data(),
    .swapchainCount = 1,
    .pSwapchains = &m_swapChain,
    .pImageIndices = &m_currentImageIndex,
    .pResults = nullptr
  };

  auto vulkanCommandQueue =
    std::static_pointer_cast<VulkanCommandQueue>(g_vulkanAPI().getQueue(QueueType::Graphics));
  CH_ASSERT(vulkanCommandQueue != nullptr);
  VkQueue presentQueue = vulkanCommandQueue->getHandle();
  VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    resize(m_width, m_height);
  }
  else if (result != VK_SUCCESS) {
    CH_LOG_ERROR(Vulkan, "Failed to present swap chain image");
  }
}

/*
*/
NODISCARD SPtr<ITexture>
VulkanSwapChain::getTexture(uint32 index) const {
  CH_ASSERT(index < m_imageCount);
  return chMakeShared<VulkanTexture>(m_device,
                                     m_physicalDevice,
                                     m_images[index],
                                     m_colorFormat,
                                     m_width, m_height,
                                     1, 1, 1);
}

/*
*/
NODISCARD SPtr<ITextureView>
VulkanSwapChain::getTextureView(uint32 index) const {
  CH_ASSERT(index < m_imageCount);
  return chMakeShared<VulkanTextureView>(m_device,
                                         m_imageViews[index],
                                         m_colorFormat,
                                         0, 1, 0, 1,
                                         TextureViewType::View2D);
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
    CH_LOG_WARNING(Vulkan, "Intento de resize a 0x0 - ignorado");
    return;
  }

  vkDeviceWaitIdle(m_device);

  create(width, height, (m_presentMode != VK_PRESENT_MODE_FIFO_KHR));
}


/*
*/
void
VulkanSwapChain::cleanUpSwapChain() {
  vkDeviceWaitIdle(m_device);

  m_framebuffers.clear();

  if (m_renderPass) {
    m_renderPass.reset();
  }

  for (auto& imageView : m_imageViews) {
    if (imageView != VK_NULL_HANDLE){
      vkDestroyImageView(m_device, imageView, nullptr);
      imageView = VK_NULL_HANDLE;
    }
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
      .pNext = nullptr,
      .flags = 0,
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

/*
*/
void
VulkanSwapChain::createRenderPass() {
  AttachmentDescription colorAttachment{
      .format = vkFormatToChFormat(m_colorFormat),
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .initialLayout = TextureLayout::Undefined,
      .finalLayout = TextureLayout::PresentSrc
  };

  AttachmentReference colorRef{
      .attachment = 0,
      .layout = TextureLayout::ColorAttachment
  };

  SubpassDescription subpass{
      .pipelineBindPoint = PipelineBindPoint::Graphics,
      .colorAttachments = {colorRef}
  };

  SubpassDependency dependency{
      .srcSubpass = SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = PipelineStage::ColorAttachmentOutput,
      .dstStageMask = PipelineStage::ColorAttachmentOutput,
      .srcAccessMask = Access::NoAccess,
      .dstAccessMask = Access::ColorAttachmentWrite
  };

  RenderPassCreateInfo renderPassInfo{
      .attachments = {colorAttachment},
      .subpasses = {subpass},
      .dependencies = {dependency}
  };

  m_renderPass = g_vulkanAPI().createRenderPass(renderPassInfo);
}

/*
*/
void
VulkanSwapChain::createFramebuffers() {
  m_framebuffers.resize(m_imageCount);

  for (uint32 i = 0; i < m_imageCount; i++) {
    auto textureView = getTextureView(i);

    FrameBufferCreateInfo framebufferInfo{
      .renderPass = m_renderPass,
      .attachments = {textureView},
      .width = m_width,
      .height = m_height,
      .layers = 1
    };

    m_framebuffers[i] = g_vulkanAPI().createFrameBuffer(framebufferInfo);
  }
}
} // namespace chEngineSDK
