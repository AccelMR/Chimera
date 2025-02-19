/************************************************************************/
/**
 * @file chVulkanTexutre.h
 * @author AccelMR
 * @date 2025/01/28
 *   Vulkan Texture implementation.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chTexture.h"

namespace chEngineSDK {
class VulkanTexture final : public Texture
{
 public:

  VulkanTexture() = default;

  VulkanTexture(VkDevice device) 
    : m_device(device), m_image(nullptr) {}

  VulkanTexture(VkImage image, 
                VkImageView imageView, 
                VkFramebuffer frameBuffer, 
                VkExtent2D extent,
                VkDevice device)
    : m_image(image),
      m_imageView(imageView),
      m_frameBuffer(frameBuffer),
      m_extent(extent),
      m_device(device) {}

  ~VulkanTexture() override;
  
  void
  _internalInit(const chGPUDesc::TextureDesc& desc) override;

  void 
  createImageView();

  FORCEINLINE VkImage
  getImage() const {
    return m_image;
  }

  FORCEINLINE VkImageView
  getImageView() const {
    return m_imageView;
  }

  FORCEINLINE VkFramebuffer
  getFrameBuffer() const {
    return m_frameBuffer;
  }

  FORCEINLINE VkFormat
  getFormat() const {
    return m_format;
  }

  FORCEINLINE VkExtent2D
  getExtent() const {
    return m_extent;
  }

 private:
  VkImage m_image;
  VkImageView m_imageView;
  VkFramebuffer m_frameBuffer;
  VkDevice m_device;
  VkFormat m_format;
  VkExtent2D m_extent;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
} // namespace chEngineSDK

