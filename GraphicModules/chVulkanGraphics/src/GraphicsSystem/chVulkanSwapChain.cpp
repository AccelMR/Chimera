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
#include "chVulkanTranslator.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK{
using namespace chGPUDesc;

namespace chVulkanSwapChainUtils {
/*
*/
VkRenderPass
createBasicRenderPass(VkFormat colorFormat, VkDevice& vulkanDevice) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = colorFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // 5. Crear el render pass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkRenderPass renderPass;
  if (vkCreateRenderPass(vulkanDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create render pass.");
  }

  LOG_INFO("Basic render pass created successfully.");
  return renderPass;
}
}

/*
*/
VulkanSwapChain::~VulkanSwapChain() {
}

/*
*/
void
VulkanSwapChain::init(const SwapChainDesc& desc) {
    GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
    VkDevice device = rhi.getDevice();
    VkSurfaceKHR surface = rhi.getSurface();
    VkPhysicalDevice physicalDevice = rhi.getPhysicalDevice();

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    m_surfaceFormat.format = VulkanTranslator::get(desc.format);
    m_swapChainExtent = { desc.width, desc.height };

    VkPresentModeKHR effectMode = VulkanTranslator::get(desc.effect);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = desc.frameCount;
    createInfo.imageFormat = m_surfaceFormat.format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32 graphicsQueueFamilyIndex = rhi.getGraphicsQueueFamilyIndex();
    uint32 presentQueueFamilyIndex = rhi.getPresentQueueFamilyIndex();
    uint32 queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = effectMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    throwIfFailed(vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapChain));

    createResources();

    LOG_INFO("Vulkan swapchain successfully initialized.");
}

/*
 */
void 
VulkanSwapChain::createResources()
{
  VkDevice device = g_VulkanGraphicsModule().getDevice();
  uint32 swapChainImageCount;
  vkGetSwapchainImagesKHR(device, m_swapChain, &swapChainImageCount, nullptr);
  m_swapChainImages.resize(swapChainImageCount);
  vkGetSwapchainImagesKHR(device,
                          m_swapChain,
                          &swapChainImageCount,
                          m_swapChainImages.data());

  LOG_INFO(StringUtils::format("Created {0} swapchain images.", swapChainImageCount));
}

/*
*/
void
VulkanSwapChain::_internalResize(uint32 frameCount, 
                                 uint32 width,
                                 uint32 height,
                                 FORMAT format) {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  VkSwapchainKHR oldSwapchain = m_swapChain;

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

  throwIfFailed(vkCreateSwapchainKHR(rhi.getDevice(), 
                                     &swapchainInfo, 
                                     nullptr, 
                                     &m_swapChain));

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


