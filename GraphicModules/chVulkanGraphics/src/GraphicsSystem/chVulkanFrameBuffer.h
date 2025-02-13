/************************************************************************/
/**
 * @file chVulkanFramebuffer.h
 * @author AccelMR
 * @date 2025/02/01
 *   Vulkan-specific implementation of a GPU Framebuffer.
 */
/************************************************************************/
#pragma once

#include "chGPUFramebuffer.h"
#include "chVulkanTexture.h"

namespace chEngineSDK {
class VulkanFramebuffer : public Framebuffer {
 public:
  VulkanFramebuffer(const SPtr<RenderPass>& renderPass, 
                    const Vector<SPtr<Texture>>& attachments);
  ~VulkanFramebuffer();

  void
  _internalBind() override {}

  uint32
  _internalGetWidth() const override { return m_width; }



  uint32 
  _internalGetHeight() const override { return m_height; }
  
  const Vector<SPtr<Texture>>& 
  _internalGetAttachments() const override { return m_attachments; }

  FORCEINLINE VkFramebuffer 
  getFramebuffer() const { return m_framebuffer; }

  FORCEINLINE VkRect2D
  getExtent() const { return m_extent; }

 private:
  VkFramebuffer m_framebuffer;
  VkRect2D m_extent;
};

} // namespace chEngineSDK
