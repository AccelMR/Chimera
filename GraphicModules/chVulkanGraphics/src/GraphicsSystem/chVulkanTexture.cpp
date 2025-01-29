/************************************************************************/
/**
 * @file chVulkanTexutre.cpp
 * @author AccelMR
 * @date 2025/01/28
 *   Vulkan Texture implementation.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanTexture.h"

#include "chDebug.h"
#include "chGPUResourceDescriptors.h"

namespace chEngineSDK {
/*
*/
VulkanTexture::~VulkanTexture() {
  if (m_image) {
    vkDestroyImage(m_device, m_image, nullptr);
  }
}

/*
*/
void
VulkanTexture::_internalInit(const chGPUDesc::TextureDesc& desc) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32>(desc.dimensions.x);
  imageInfo.extent.height = static_cast<uint32>(desc.dimensions.y);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional

  throwIfFailed(vkCreateImage(m_device, &imageInfo, nullptr, &m_image));
}
} // namespace chEngineSDK  