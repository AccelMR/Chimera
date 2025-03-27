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

#include "chDebug.h"
#include "chGPUResourceDescriptors.h"
#include "chScreen.h"
#include "chStringUtils.h"
#include "chMath.h"
#include "chVulkanGPUCommandBuffer.h"
#include "chVulkanFence.h"
#include "chVulkanGPUPipelineState.h"
#include "chVulkanRenderPass.h"
#include "chVulkanTexture.h"
#include "chVulkanTranslator.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK{
/*
*/
VulkanSwapChain::~VulkanSwapChain() {
  cleanup();
}

/*
*/
void
VulkanSwapChain::_internalInit(const chGPUDesc::SwapChainDesc& desc) {
    // Cleanup any existing resources before reinitializing
  _internalCleanup();

  m_format = desc.format;

  // Get the surface capabilities
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

  // Determine swap extent
  VkExtent2D swapExtent{};
  swapExtent.width = Math::clamp(desc.width, 
                                surfaceCapabilities.minImageExtent.width, 
                                surfaceCapabilities.maxImageExtent.width);
  swapExtent.height = Math::clamp(desc.height, 
                                 surfaceCapabilities.minImageExtent.height, 
                                 surfaceCapabilities.maxImageExtent.height);
  m_swapChainExtent = swapExtent;

  // Select the number of images in the swap chain
  uint32 imageCount = Math::clamp(desc.frameCount, 
                                 surfaceCapabilities.minImageCount, 
                                 (surfaceCapabilities.maxImageCount > 0) ? surfaceCapabilities.maxImageCount : UINT32_MAX);

  // Create the swap chain
  VkSwapchainCreateInfoKHR swapChainCreateInfo{};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = m_surface;
  swapChainCreateInfo.minImageCount = imageCount;
  swapChainCreateInfo.imageFormat = VulkanTranslator::get(m_format);
  swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // TODO: make this configurable
  swapChainCreateInfo.imageExtent = swapExtent;
  swapChainCreateInfo.imageArrayLayers = desc.stereo ? 2 : 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32 m_graphicsQueueFamilyIndex = g_VulkanGraphicsModule().getGraphicsQueueFamilyIndex();
  uint32 m_presentQueueFamilyIndex = g_VulkanGraphicsModule().getPresentQueueFamilyIndex();

  // Determine sharing mode
  uint32 queueFamilyIndices[] = { m_graphicsQueueFamilyIndex, m_presentQueueFamilyIndex };
  if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex) {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  }

  swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.presentMode = VulkanTranslator::get(desc.effect);
  swapChainCreateInfo.clipped = VK_TRUE;
  swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create Vulkan swap chain.");
  }

  // Retrieve the swap chain images
  vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
  m_swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

  // Create image views
  m_swapChainImageViews.resize(imageCount);
  for (size_t i = 0; i < m_swapChainImages.size(); i++) {
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = m_swapChainImages[i];
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = VulkanTranslator::get(m_format);
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
      CH_EXCEPT(InternalErrorException, "Failed to create image views for swap chain.");
    }
  }

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  // Initialize fences for synchronization
  m_frameFences.resize(imageCount);
  m_imageAvailableSemaphores.resize(imageCount);
  m_renderFinishedSemaphores.resize(imageCount);
  for (size_t i = 0; i < desc.frameCount; i++) {
    throwIfFailed(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
    throwIfFailed(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
    m_frameFences[i] = chMakeShared<chEngineSDK::VulkanFence>(m_device);
  }

  CH_LOG_INFO("Vulkan swap chain initialized successfully.");
}

/*
*/
void
VulkanSwapChain::_internalCleanup() {
  for (auto imageView : m_swapChainImageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

/*
*/
void
VulkanSwapChain::_internalResize(uint32 width, uint32 height) {
  // Cleanup any existing resources before reinitializing
  uint32 frameCount = m_frameFences.size();
  cleanup();
  init({ width, height, m_format, false, frameCount });
}

/*
*/
bool
VulkanSwapChain::present(uint32 syncInterval, 
                         uint32 flags,
                         const VulkanGPUCommandBuffer* commandBuffer) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame];
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer vkCommandBuffer = commandBuffer->getCommandBuffer();
  submitInfo.pCommandBuffers = &vkCommandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

  VkQueue queue = g_VulkanGraphicsModule().getPresentQueue();
  throwIfFailed(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapChain;
  presentInfo.pImageIndices = &m_currentFrame;

  VkResult result = vkQueuePresentKHR(queue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return false;
  }

  return result == VK_SUCCESS;
}

/*
*/
bool
VulkanSwapChain::_internalAcquireNextFrame() {

  m_frameFences[m_currentFrame]->wait(2);
  m_frameFences[m_currentFrame]->reset();
  
  VkResult result = vkAcquireNextImageKHR(m_device,
                                          m_swapChain,
                                          UINT64_MAX,
                                          m_imageAvailableSemaphores[m_currentFrame],
                                          VK_NULL_HANDLE, 
                                          &m_currentFrame);
  return result == VK_SUCCESS;
}

/*
*/
uint32
VulkanSwapChain::_internalGetCurrentFrameIndex() const {
  return m_currentFrame;
}

/*
*/
FORMAT
VulkanSwapChain::_internalGetFormat() const {
  return m_format;
}

/*
*/
SPtr<Texture>
VulkanSwapChain::_internalGetCurrentFrame(const SPtr<GPUPipelineState>& pipeline) const {
  VkDevice device = g_VulkanGraphicsModule().getDevice();
  auto vulkanTexture = chMakeShared<VulkanTexture>(m_swapChainImages[m_currentFrame],
                                                   m_swapChainImageViews[m_currentFrame],
                                                   VK_NULL_HANDLE,
                                                   m_swapChainExtent,
                                                   device);
  return  std::reinterpret_pointer_cast<Texture>(vulkanTexture);
}

/*
*/
void
VulkanSwapChain::_internalSetVSyncEnabled(bool enabled) {
  // Not supported
  CH_LOG_WARNING("VSync is not supported in Vulkan.");
}

/*
*/
void
VulkanSwapChain::_internalWaitForGPU() {
  vkDeviceWaitIdle(m_device);
}

/*
*/
void
VulkanSwapChain::recreateFramebuffers(VkRenderPass renderPass) const {
  for (auto framebuffer : m_frameBuffers) {
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  }
  m_frameBuffers.clear();

  for (SIZE_T i = 0; i < m_swapChainImageViews.size(); ++i) {
    VkImageView attachments[] = { m_swapChainImageViews[i] };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &m_swapChainImageViews[i];
    framebufferInfo.width = m_swapChainExtent.width;
    framebufferInfo.height = m_swapChainExtent.height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    throwIfFailed(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer));

    m_frameBuffers.push_back(framebuffer);
  }
}

} // namespace chEngineSDK