/************************************************************************/
/**
 * @file chVulkanFrameBuffer.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief 
 * Vulkan frame buffer implementation.
 * This file contains the implementation of the frame buffer
 * interface for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIFrameBuffer.h"

namespace chEngineSDK {
class VulkanFrameBuffer : public IFrameBuffer {
 public:
  VulkanFrameBuffer(VkDevice device, const FrameBufferCreateInfo& createInfo);
  ~VulkanFrameBuffer() override;

  NODISCARD FORCEINLINE uint32 
  getWidth() const override { return m_width; }
  
  NODISCARD FORCEINLINE uint32
  getHeight() const override { return m_height; }

  NODISCARD FORCEINLINE uint32
  getLayers() const override { return 1; } // TODO: Support layers

  NODISCARD FORCEINLINE VkFramebuffer
  getHandle() const { return m_framebuffer; }

  NODISCARD FORCEINLINE SPtr<IRenderPass>
  getRenderPass() const override { return m_renderPass; }

  NODISCARD FORCEINLINE const Vector<SPtr<ITextureView>>&
  getAttachments() const override { return m_attachments; }

  // NODISCARD FORCEINLINE const Vector<SPtr<ITextureView>>&
  // getDepthStencilAttachments() const override { return m_depthStencilAttachments; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

  SPtr<IRenderPass> m_renderPass = nullptr;
  Vector<SPtr<ITextureView>> m_attachments;
  Vector<SPtr<ITextureView>> m_colorAttachments;


  uint32 m_width = 0;
  uint32 m_height = 0;
};
} // namespace chEngineSDK