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
#include "chVulkanFence.h"
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

  // // Select the surface format
  // VkSurfaceFormatKHR surfaceFormat{};
  // {
  //   uint32 formatCount;
  //   vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
  //   if (formatCount == 0) {
  //     CH_EXCEPT(RunTimeException, "No available surface formats.");
  //   }

  //   Vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  //   vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, surfaceFormats.data());

  //   surfaceFormat
  //    = VulkanTranslator::get(desc.format);
  // }

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
  submitInfo.pCommandBuffers = nullptr;
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
VulkanSwapChain::_internalGetCurrentFrame() const {
  return  std::static_pointer_cast<Texture>(
            chMakeShared<VulkanTexture>(m_swapChainImages[m_currentFrame], 
                                        g_VulkanGraphicsModule().getDevice()));
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

} // namespace chEngineSDK























// using namespace chGPUDesc;

// namespace chVulkanSwapChainUtils {
// /*
// */
// VkRenderPass
// createBasicRenderPass(VkFormat colorFormat, VkDevice& vulkanDevice) {
//   VkAttachmentDescription colorAttachment = {};
//   colorAttachment.format = colorFormat;
//   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//   VkAttachmentReference colorAttachmentRef = {};
//   colorAttachmentRef.attachment = 0;
//   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

//   VkSubpassDescription subpass = {};
//   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//   subpass.colorAttachmentCount = 1;
//   subpass.pColorAttachments = &colorAttachmentRef;

//   VkSubpassDependency dependency = {};
//   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//   dependency.dstSubpass = 0;
//   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//   dependency.srcAccessMask = 0;
//   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

//   // 5. Crear el render pass
//   VkRenderPassCreateInfo renderPassInfo = {};
//   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//   renderPassInfo.attachmentCount = 1;
//   renderPassInfo.pAttachments = &colorAttachment;
//   renderPassInfo.subpassCount = 1;
//   renderPassInfo.pSubpasses = &subpass;
//   renderPassInfo.dependencyCount = 1;
//   renderPassInfo.pDependencies = &dependency;

//   VkRenderPass renderPass;
//   if (vkCreateRenderPass(vulkanDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
//     CH_EXCEPT(InternalErrorException, "Failed to create render pass.");
//   }

//   CH_LOG_INFO("Basic render pass created successfully.");
//   return renderPass;
// }
// }

// /*
// */
// VulkanSwapChain::~VulkanSwapChain() {
// }

// /*
// */
// void
// VulkanSwapChain::init(const SwapChainDesc& desc) {
//     GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
//     VkDevice device = rhi.getDevice();
//     VkSurfaceKHR surface = rhi.getSurface();
//     VkPhysicalDevice physicalDevice = rhi.getPhysicalDevice();

//     VkSurfaceCapabilitiesKHR surfaceCapabilities;
//     vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

//     m_surfaceFormat.format = VulkanTranslator::get(desc.format);
//     m_swapChainExtent = { desc.width, desc.height };

//     VkPresentModeKHR effectMode = VulkanTranslator::get(desc.effect);

//     VkSwapchainCreateInfoKHR createInfo{};
//     createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//     createInfo.surface = surface;
//     createInfo.minImageCount = desc.frameCount;
//     createInfo.imageFormat = m_surfaceFormat.format;
//     createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
//     createInfo.imageExtent = m_swapChainExtent;
//     createInfo.imageArrayLayers = 1;
//     createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

//     uint32 graphicsQueueFamilyIndex = rhi.getGraphicsQueueFamilyIndex();
//     uint32 presentQueueFamilyIndex = rhi.getPresentQueueFamilyIndex();
//     uint32 queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};

//     if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
//         createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//         createInfo.queueFamilyIndexCount = 2;
//         createInfo.pQueueFamilyIndices = queueFamilyIndices;
//     }
//     else {
//         createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     }

//     createInfo.preTransform = surfaceCapabilities.currentTransform;
//     createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//     createInfo.presentMode = effectMode;
//     createInfo.clipped = VK_TRUE;
//     createInfo.oldSwapchain = VK_NULL_HANDLE;

//     throwIfFailed(vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapChain));

//     createResources();

//     m_frameFences.reserve(desc.frameCount);
//     for (uint32 i = 0; i < desc.frameCount; ++i) {
//         m_frameFences.push_back(rhi.createFence());
//     }

//     CH_LOG_INFO("Vulkan swapchain successfully initialized.");
// }

// /*
//  */
// void 
// VulkanSwapChain::createResources()
// {
//   VkDevice device = g_VulkanGraphicsModule().getDevice();
//   uint32 swapChainImageCount;
//   vkGetSwapchainImagesKHR(device, m_swapChain, &swapChainImageCount, nullptr);
//   m_swapChainImages.resize(swapChainImageCount);
//   vkGetSwapchainImagesKHR(device,
//                           m_swapChain,
//                           &swapChainImageCount,
//                           m_swapChainImages.data());

//   CH_LOG_INFO(StringUtils::format("Created {0} swapchain images.", swapChainImageCount));
// }

// /*
// */
// void
// VulkanSwapChain::_internalResize(uint32 frameCount, 
//                                  uint32 width,
//                                  uint32 height,
//                                  FORMAT format) {
//   GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
//   VkSwapchainKHR oldSwapchain = m_swapChain;

//   VkSwapchainCreateInfoKHR swapchainInfo = {};
//   swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//   swapchainInfo.surface = rhi.getSurface();
//   swapchainInfo.minImageCount = frameCount;
//   swapchainInfo.imageFormat = VulkanTranslator::get(format);
//   swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
//   swapchainInfo.imageExtent.width = width;
//   swapchainInfo.imageExtent.height = height;
//   swapchainInfo.imageArrayLayers = 1;
//   swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//   swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//   swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
//   swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//   swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
//   swapchainInfo.clipped = VK_TRUE;
//   swapchainInfo.oldSwapchain = oldSwapchain;

//   throwIfFailed(vkCreateSwapchainKHR(rhi.getDevice(), 
//                                      &swapchainInfo, 
//                                      nullptr, 
//                                      &m_swapChain));

//   vkDestroySwapchainKHR(rhi.getDevice(), oldSwapchain, nullptr);

//   createResources();
// }

// /*
// */
// void
// VulkanSwapChain::_internalPresent(int32 syncInterval, int32 flags) {

// }

// /*
// */
// uint32
// VulkanSwapChain::_internalGetCurrentFrameIndex() const {
//   return 0;
// }

// /*
// */
// SPtr<Texture>
// VulkanSwapChain::getCurrentRenderTarget() const {
//   return nullptr;
// }

// }


