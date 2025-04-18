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
      m_ownsTexture(true) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = chTextureTypeToVkImageType(createInfo.type);
  imageInfo.format = chFormatToVkFormat(createInfo.format);
  imageInfo.extent.width = createInfo.width;
  imageInfo.extent.height = createInfo.height;
  imageInfo.extent.depth = createInfo.depth;
  imageInfo.mipLevels = createInfo.mipLevels;
  imageInfo.arrayLayers = createInfo.arrayLayers;
  imageInfo.samples = chSampleCountToVkSampleCount(createInfo.samples);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = chTextureUsageToVkImageUsage(createInfo.usage);
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


  if (createInfo.initialData && createInfo.initialDataSize > 0) {
    uploadData(createInfo.initialData, createInfo.initialDataSize);
  }
}

/*
*/
VulkanTexture::~VulkanTexture() {
  if (!m_ownsTexture) {
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
  return chMakeShared<VulkanTextureView>(m_device, this, createInfo);
}

/*
*/
void
VulkanTexture::uploadData(const void* data, SIZE_T size) {
  CH_ASSERT(data != nullptr);
  CH_ASSERT(size > 0);
  
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer));
  
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);
  
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
  
  allocInfo.memoryTypeIndex = UINT32_MAX;
  for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((memRequirements.memoryTypeBits & (1 << i)) && 
        (memProperties.memoryTypes[i].propertyFlags & 
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
      allocInfo.memoryTypeIndex = i;
      break;
    }
  }
  
  VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory));
  VK_CHECK(vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0));
  
  void* mapped;
  VK_CHECK(vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &mapped));
  memcpy(mapped, data, size);
  vkUnmapMemory(m_device, stagingBufferMemory);
  
  VkCommandPool commandPool;
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  poolInfo.queueFamilyIndex = 0; // Asumiendo que 0 es el índice de la cola de gráficos
  
  VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool));
  
  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo commandBufferAllocInfo{};
  commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocInfo.commandPool = commandPool;
  commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocInfo.commandBufferCount = 1;
  
  VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &commandBuffer));
  
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
  
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = m_image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = m_mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = m_arrayLayers;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);
  
  VkBufferImageCopy region{
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .imageOffset = {0, 0, 0},
    .imageExtent = {m_width,m_height, 1},
  };
    
  vkCmdCopyBufferToImage(commandBuffer,
                         stagingBuffer,
                         m_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);
  
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  
  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);
  
  VK_CHECK(vkEndCommandBuffer(commandBuffer));
  
  VkSubmitInfo submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
  };
  
  VkQueue graphicsQueue;
  vkGetDeviceQueue(m_device, 0, 0, &graphicsQueue); // Asumiendo 0 como índice de cola de gráficos
  
  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
  VK_CHECK(vkQueueWaitIdle(graphicsQueue));
  
  // Limpieza
  vkFreeCommandBuffers(m_device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(m_device, commandPool, nullptr);
  
  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}
} // namespace chEngineSDK