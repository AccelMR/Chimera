/************************************************************************/
/**
 * @file chVulkanTexture.h
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan texture implementation.
 * This file contains the implementation of the texture
 * interface for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chITexture.h"

namespace chEngineSDK {
class VulkanTexture : public ITexture {
 public:
  VulkanTexture(VkDevice device, 
                VkPhysicalDevice physicalDevice,
                const TextureCreateInfo& createInfo);

  VulkanTexture(VkDevice device, 
                VkPhysicalDevice physicalDevice, 
                VkImage image,
                VkFormat format,
                uint32 width, uint32 height,
                uint32 depth, uint32 mipLevels,
                uint32 arrayLayers = 1)
  : m_device(device), 
    m_physicalDevice(physicalDevice),
    m_width(width), m_height(height),
    m_depth(depth), m_mipLevels(mipLevels),
    m_arrayLayers(arrayLayers), m_format(vkFormatToChFormat(format)),
    m_type(TextureType::Texture2D), m_image(image), m_memory(VK_NULL_HANDLE),
    m_bShouldDestroy(false) {}

  ~VulkanTexture() override;

  NODISCARD TextureType
  getType() const override { return m_type; }

  NODISCARD Format
  getFormat() const override { return m_format; }

  NODISCARD uint32
  getWidth() const override { return m_width; }

  NODISCARD uint32
  getHeight() const override { return m_height; }

  NODISCARD uint32
  getDepth() const override { return m_depth; }

  NODISCARD uint32
  getMipLevels() const override { return m_mipLevels; }

  NODISCARD uint32
  getArrayLayers() const override { return m_arrayLayers; }

  NODISCARD SPtr<ITextureView>
  createView(const TextureViewCreateInfo& createInfo = {}) override;

  void
  uploadData(const void* data, size_t size) override;

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
  VkImage m_image = VK_NULL_HANDLE;

  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_depth = 0;
  uint32 m_mipLevels = 0;
  uint32 m_arrayLayers = 0;
  Format m_format = Format::Unknown;
  TextureType m_type = TextureType::Texture2D;
  bool m_bShouldDestroy = true;
};
} // namespace chEngineSDK