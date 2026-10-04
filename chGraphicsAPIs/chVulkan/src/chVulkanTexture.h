/************************************************************************/
/**
 * @file chVulkanTexture.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan implementation of ITexture.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chITexture.h"

namespace chEngineSDK {

/**
 * Vulkan image whose memory comes from VMA, freed through the deletion queue. Swap chain
 * images are wrapped without ownership, because the swap chain creates and destroys them.
 */
class VulkanTexture : public ITexture
{
 public:
  VulkanTexture(VkDevice device, VmaAllocator allocator, const TextureCreateInfo& createInfo);

  VulkanTexture(VkDevice device,
                VkImage image,
                VkFormat format,
                uint32 width,
                uint32 height,
                uint32 depth,
                uint32 mipLevels,
                uint32 arrayLayers = 1)
    : m_device(device),
      m_image(image),
      m_width(width),
      m_height(height),
      m_depth(depth),
      m_mipLevels(mipLevels),
      m_arrayLayers(arrayLayers),
      m_format(vkFormatToChFormat(format)),
      m_type(TextureType::Texture2D),
      m_ownsTexture(false)
  {}

  ~VulkanTexture() override;

  NODISCARD TextureType
  getType() const override
  {
    return m_type;
  }

  NODISCARD Format
  getFormat() const override
  {
    return m_format;
  }

  NODISCARD uint32
  getWidth() const override
  {
    return m_width;
  }

  NODISCARD uint32
  getHeight() const override
  {
    return m_height;
  }

  NODISCARD uint32
  getDepth() const override
  {
    return m_depth;
  }

  NODISCARD uint32
  getMipLevels() const override
  {
    return m_mipLevels;
  }

  NODISCARD uint32
  getArrayLayers() const override
  {
    return m_arrayLayers;
  }

  NODISCARD SPtr<ITextureView>
  createView(const TextureViewCreateInfo& createInfo = {}) override;

  void
  uploadData(const void* data, SIZE_T size) override;

  NODISCARD FORCEINLINE VkImage
  getHandle() const
  {
    return m_image;
  }

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VmaAllocator m_allocator = nullptr;
  VmaAllocation m_allocation = nullptr;
  VkImage m_image = VK_NULL_HANDLE;
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_depth = 0;
  uint32 m_mipLevels = 0;
  uint32 m_arrayLayers = 0;
  Format m_format = Format::Unknown;
  TextureType m_type = TextureType::Texture2D;
  bool m_ownsTexture = true;
};

} // namespace chEngineSDK
