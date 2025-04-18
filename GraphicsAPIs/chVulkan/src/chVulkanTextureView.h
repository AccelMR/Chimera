/************************************************************************/
/**
 * @file chVulkanTextureView.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan texture view implementation.
 * This file contains the implementation of the texture
 * view interface for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chITextureView.h"

namespace chEngineSDK {
class VulkanTextureView : public ITextureView {
 public:
  VulkanTextureView(VkDevice device, 
                    ITexture* texture,
                    const TextureViewCreateInfo& createInfo);

  VulkanTextureView(VkDevice device, 
                    VkImageView imageView,
                    VkFormat format,
                    uint32 baseMipLevel, uint32 mipLevelCount,
                    uint32 baseArrayLayer, uint32 arrayLayerCount,
                    TextureViewType viewType = TextureViewType::View2D)
  : m_device(device),
    m_imageView(imageView),
    m_format(format),
    m_baseMipLevel(baseMipLevel),
    m_mipLevelCount(mipLevelCount),
    m_baseArrayLayer(baseArrayLayer),
    m_arrayLayerCount(arrayLayerCount),
    m_viewType(chTextureViewTypeToVkTextureViewType(viewType)),
    m_ownsTextureView(false) {
    CH_ASSERT(m_device != VK_NULL_HANDLE);
    CH_ASSERT(m_imageView != VK_NULL_HANDLE);
  }

  ~VulkanTextureView() override;

  NODISCARD VkImageView
  getHandle() const { return m_imageView; }

  NODISCARD FORCEINLINE virtual Format 
  getFormat() const override { return vkFormatToChFormat(m_format); }

  NODISCARD FORCEINLINE virtual TextureViewType 
  getViewType() const override { return vkTextureViewTypeToChTextureViewType(m_viewType); }

  NODISCARD FORCEINLINE virtual uint32 
  getBaseMipLevel() const override { return m_baseMipLevel; }

  NODISCARD FORCEINLINE virtual uint32 
  getMipLevelCount() const override { return m_mipLevelCount; }

  NODISCARD FORCEINLINE virtual uint32 
  getBaseArrayLayer() const override { return m_baseArrayLayer; }

  NODISCARD FORCEINLINE virtual uint32 
  getArrayLayerCount() const override { return m_arrayLayerCount; }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkImageView m_imageView = VK_NULL_HANDLE;
  VkImageViewType m_viewType = VK_IMAGE_VIEW_TYPE_2D;
  uint32 m_baseMipLevel = 0;
  uint32 m_mipLevelCount = 1;
  uint32 m_baseArrayLayer = 0;
  uint32 m_arrayLayerCount = 1;
  VkFormat m_format = VK_FORMAT_UNDEFINED;
  bool m_ownsTextureView = true;

  // TODO: this should be a weak pointer
  ITexture* m_texture;
};
} // namespace chEngineSDK