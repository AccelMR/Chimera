/************************************************************************/
/**
 * @file chVulkanTexture.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan texture implementation.
 * This file contains the implementation of the texture
 * interface for Vulkan.
 */
/************************************************************************/
#include "chVulkanTexture.h"

#include "chVulkanTextureView.h"

namespace chEngineSDK {
/*
*/
VulkanTexture::VulkanTexture(VkDevice device, 
                             VkPhysicalDevice physicalDevice, 
                             const TextureCreateInfo& createInfo)
    : m_device(device), 
      m_physicalDevice(physicalDevice),
      m_width(createInfo.width), m_height(createInfo.height),
      m_depth(createInfo.depth), m_mipLevels(createInfo.mipLevels),
      m_arrayLayers(createInfo.arrayLayers), m_format(createInfo.format),
      m_type(createInfo.type),
      m_bShouldDestroy(true) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = static_cast<VkImageType>(createInfo.type);
  imageInfo.format = static_cast<VkFormat>(createInfo.format);
  imageInfo.extent.width = createInfo.width;
  imageInfo.extent.height = createInfo.height;
  imageInfo.extent.depth = createInfo.depth;
  imageInfo.mipLevels = createInfo.mipLevels;
  imageInfo.arrayLayers = createInfo.arrayLayers;
  imageInfo.samples = static_cast<VkSampleCountFlagBits>(createInfo.samples);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = static_cast<VkImageUsageFlags>(createInfo.usage);
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &m_image));

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;

  // Find memory type
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
  
  for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((memRequirements.memoryTypeBits & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      allocInfo.memoryTypeIndex = i;
      break;
    }
  }

  VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory));
  
  vkBindImageMemory(m_device, m_image, m_memory, 0);
}

/*
*/
VulkanTexture::~VulkanTexture() {
  if (!m_bShouldDestroy) {
    return;
  }

  if (m_memory != VK_NULL_HANDLE) {
    vkFreeMemory(m_device, m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
  }
  if (m_image != VK_NULL_HANDLE) {
    vkDestroyImage(m_device, m_image, nullptr);
    m_image = VK_NULL_HANDLE;
  }
} 

/*
*/
NODISCARD SPtr<ITextureView>
VulkanTexture::createView(const TextureViewCreateInfo& createInfo) {
  return chMakeShared<VulkanTextureView>(m_device, this);
}

/*
*/
void
VulkanTexture::uploadData(const void* data, SIZE_T size){
  CH_ASSERT(data != nullptr);
  CH_ASSERT(size > 0);

  CH_LOG_ERROR("VulkanTexture::uploadData not implemented");
}

} // namespace chEngineSDK