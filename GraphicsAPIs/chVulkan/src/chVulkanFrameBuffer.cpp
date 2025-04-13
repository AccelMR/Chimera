/************************************************************************/
/**
 * @file chVulkanFrameBuffer.cpp
 * @author AccelMR
 * @date 2025/04/12
 * @brief 
 * Vulkan frame buffer implementation.
 * This file contains the implementation of the frame buffer
 * interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanFrameBuffer.h"

#include "chVulkanAPI.h"
#include "chVulkanRenderPass.h"
#include "chVulkanTextureView.h"

namespace chEngineSDK {
/*
*/
VulkanFrameBuffer::VulkanFrameBuffer(VkDevice device, 
                                     const FrameBufferCreateInfo& createInfo)
    : m_device(device),
      m_renderPass(createInfo.renderPass),
      m_width(createInfo.width),
      m_height(createInfo.height) {
  CH_ASSERT(m_device != VK_NULL_HANDLE);
  CH_ASSERT(m_renderPass != nullptr);

  m_attachments = createInfo.attachments;

  VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = static_cast<VulkanRenderPass*>(m_renderPass.get())->getHandle(),
      .attachmentCount = static_cast<uint32>(m_attachments.size()),
      .pAttachments = nullptr,
      .width = m_width,
      .height = m_height,
      .layers = createInfo.layers
  };

  Vector<VkImageView> attachmentHandles;
  attachmentHandles.reserve(m_attachments.size());
  for (const auto& attachment : m_attachments) {
    attachmentHandles.push_back(static_cast<VulkanTextureView*>(attachment.get())->getHandle());
  }
  framebufferInfo.pAttachments = attachmentHandles.data();

  VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffer));
}

/*
*/
VulkanFrameBuffer::~VulkanFrameBuffer() {
  if (m_framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
    m_framebuffer = VK_NULL_HANDLE;
  }
  m_attachments.clear();
  m_colorAttachments.clear();
}
} // namespace chEngineSDK