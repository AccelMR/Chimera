/************************************************************************/
/**
 * @file chVulkanFramebuffer.h
 * @author AccelMR
 * @date 2025/02/01
 *   Vulkan-specific implementation of a GPU Framebuffer.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanFrameBuffer.h"

#include "chVulkanRenderPass.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK {
VulkanFramebuffer::VulkanFramebuffer(const SPtr<RenderPass>& renderPass, 
                                     const Vector<SPtr<Texture>>& attachments) {
  m_attachments = attachments;

  if (!attachments.empty()) {
    m_width = attachments[0]->getWidth();
    m_height = attachments[0]->getHeight();
    
    for (const auto& texture : attachments) {
      if (texture->getWidth() != m_width || texture->getHeight() != m_height) {
        CH_ASSERT(false && "All attachments must have the same dimensions.");
      }
    }
  } 
  else {
    CH_ASSERT(false && "Framebuffer must have at least one attachment.");
  }

  Vector<VkImageView> attachmentViews;
  for (const auto& texture : attachments) {
    auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(texture);
    attachmentViews.push_back(vulkanTexture->getImageView());
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = std::reinterpret_pointer_cast<VulkanRenderPass>(renderPass)->getRenderPass();
  framebufferInfo.attachmentCount = static_cast<uint32>(attachmentViews.size());
  framebufferInfo.pAttachments = attachmentViews.data();
  framebufferInfo.width = m_width;
  framebufferInfo.height = m_height;
  framebufferInfo.layers = 1;

  VkDevice device = g_VulkanGraphicsModule().getDevice();
  throwIfFailed(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffer));
}

VulkanFramebuffer::~VulkanFramebuffer() {
  VkDevice device = g_VulkanGraphicsModule().getDevice();
  vkDestroyFramebuffer(device, m_framebuffer, nullptr);
}

} // namespace chEngineSDK
