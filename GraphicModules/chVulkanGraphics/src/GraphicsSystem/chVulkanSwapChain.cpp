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
#include "chVulkanTranslator.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK{
using namespace chGPUDesc;

namespace chVulkanSwapChainUtils {
/*
*/
VkRenderPass
createBasicRenderPass(VkFormat colorFormat, VkDevice& vulkanDevice) {
  // 1. Describir el attachment de color (swapchain)
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = colorFormat; // Formato de las imágenes del swapchain
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No se usa multisampling
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Borrar la imagen al comenzar el render pass
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Guardar el resultado al finalizar
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No usamos stencil
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Sin layout inicial específico
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout para presentar al swapchain

  // 2. Configurar la referencia del attachment en la subpasada
  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0; // Índice del attachment (único en este caso)
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // 3. Configurar la subpasada
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Subpasada gráfica
  subpass.colorAttachmentCount = 1; // Un attachment de color
  subpass.pColorAttachments = &colorAttachmentRef;

  // 4. Configurar dependencias (para sincronización implícita)
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Depende de operaciones externas
  dependency.dstSubpass = 0; // La primera (y única) subpasada
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Espera a que se liberen las imágenes
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Escribe en el attachment de color
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Asegura acceso para escribir

  // 5. Crear el render pass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1; // Un attachment (color)
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1; // Una subpasada
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1; // Una dependencia
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
  VkSwapchainCreateInfoKHR swapChainCreateInfo{};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = rhi.getSurface();
  swapChainCreateInfo.minImageCount = desc.frameCount;
  swapChainCreateInfo.imageFormat = VulkanTranslator::get(desc.format);
  swapChainCreateInfo.imageExtent = { desc.width, desc.height };
  swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

  VkDevice device = rhi.getDevice();
  if (vkCreateSwapchainKHR(device, 
                           &swapChainCreateInfo, 
                           nullptr, 
                           &m_swapchain) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create Vulkan swapchain.");
  }

  LOG_INFO("Vulkan Swapchain created.");
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
    viewInfo.format = rhi.getDepthFormat();
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
    framebufferInfo.renderPass = 
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    throwIfFailed(vkCreateFramebuffer(vulkanDevice, &framebufferInfo, nullptr, &m_framebuffers[i]));
  }

  LOG_INFO("Swapchain framebuffers created successfully.");
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

  throwIfFailed(vkCreateSwapchainKHR(rhi.getDevice(), 
                                     &swapchainInfo, 
                                     nullptr, 
                                     &m_swapchain));

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


