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
#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"

namespace chEngineSDK {
/*
*/
VulkanTexture::~VulkanTexture() {
  if (m_image) {
    vkDestroyImage(m_device, m_image, nullptr);
  }

  if (m_memory) {
    vkFreeMemory(m_device, m_memory, nullptr);
  }

  if (m_imageView) {
    vkDestroyImageView(m_device, m_imageView, nullptr);
  }
}

/*
*/
void
VulkanTexture::_internalInit(const chGPUDesc::TextureDesc& desc) {
  CH_ASSERT(m_device != nullptr && "Vulkan device is not set!");
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

  if (desc.dimensions.z > 1) {
    imageInfo.imageType = VK_IMAGE_TYPE_3D;
  }
  else if (desc.dimensions.y > 1) {
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
  }
  else {
    imageInfo.imageType = VK_IMAGE_TYPE_1D;
  }

  imageInfo.extent.width = static_cast<uint32>(desc.dimensions.x);
  imageInfo.extent.height = static_cast<uint32>(desc.dimensions.y);
  imageInfo.extent.depth = static_cast<uint32>(desc.dimensions.z);

  imageInfo.mipLevels = desc.mips;
  imageInfo.arrayLayers = desc.flags.isSet(TEXTURE_FLAGS::kARRAY) ? desc.arrayLayers : 1;
  imageInfo.format = VulkanTranslator::get(desc.format);

  imageInfo.tiling = desc.flags.isSet(TEXTURE_FLAGS::kLINEAR_TILING) ? 
                                      VK_IMAGE_TILING_LINEAR : 
                                      VK_IMAGE_TILING_OPTIMAL;

  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  imageInfo.usage = VulkanTranslator::get(desc.usage);

  imageInfo.sharingMode = desc.flags.isSet(TEXTURE_FLAGS::kCONCURRENT_SHARING) ? 
                                      VK_SHARING_MODE_CONCURRENT : 
                                      VK_SHARING_MODE_EXCLUSIVE;

  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = VulkanTranslator::get(desc.flags);

  m_mipLevels = desc.mips;
  m_width = desc.dimensions.x;
  m_height = desc.dimensions.y;
  m_format = imageInfo.format;

  throwIfFailed(vkCreateImage(m_device, &imageInfo, nullptr, &m_image));

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = g_VulkanGraphicsModule().findMemoryType(
    memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  throwIfFailed(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory));

  throwIfFailed(vkBindImageMemory(m_device, m_image, m_memory, 0));

  createImageView();
}

/*
*/
void 
VulkanTexture::createImageView() {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = m_image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = m_format;
  
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = m_mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  throwIfFailed(vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView));
}
} // namespace chEngineSDK  