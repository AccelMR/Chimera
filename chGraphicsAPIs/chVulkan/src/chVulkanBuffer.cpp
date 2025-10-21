/************************************************************************/
/**
 * @file chIBuffer.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Interface for the buffer. This is the base class for all buffers.
 * It is used to create buffers and allocate them.
 * It is also used to reset buffers and free them.
 */
/************************************************************************/

#include "chVulkanBuffer.h"

#include "chICommandQueue.h"
#include "chVulkanAPI.h"

namespace chEngineSDK {
namespace chVulkanBufferUtils {
uint32
findMemoryType(VkPhysicalDevice physicalDevice,
               uint32 typeFilter,
               VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    CH_EXCEPT(VulkanErrorException, "Failed to find suitable memory type");
    return 0;
}
} // namespace chVulkanBufferUtils
using namespace chVulkanBufferUtils;

/*
*/
VulkanBuffer::VulkanBuffer(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           const BufferCreateInfo& createInfo)
    : m_device(device), m_size(createInfo.size) {
  VkBufferUsageFlags usage = 0;
  if (createInfo.usage.isSet(BufferUsage::VertexBuffer)) {
    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if (createInfo.usage.isSet(BufferUsage::IndexBuffer)) {
    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if (createInfo.usage.isSet(BufferUsage::UniformBuffer)) {
    usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if (createInfo.usage.isSet(BufferUsage::StorageBuffer)) {
    usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }
  if (createInfo.usage.isSet(BufferUsage::TransferSrc)) {
    usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (createInfo.usage.isSet(BufferUsage::TransferDst)) {
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  VkBufferCreateInfo bufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = m_size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr
  };

  VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer));

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

  VkMemoryPropertyFlags memoryFlags = 0;
  switch (createInfo.memoryUsage) {
  case MemoryUsage::GpuOnly:
    memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_mappable = false;
    break;
  case MemoryUsage::CpuOnly:
    memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_mappable = true;
    break;
  case MemoryUsage::CpuToGpu:
    memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_mappable = true;
    break;
  case MemoryUsage::GpuToCpu:
    memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    m_mappable = true;
    break;
  }

  uint32 memoryTypeIndex = findMemoryType(physicalDevice,
                                          memRequirements.memoryTypeBits,
                                          memoryFlags);

  VkMemoryAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = memRequirements.size,
    .memoryTypeIndex = memoryTypeIndex
  };
  VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &m_memory));
  VK_CHECK(vkBindBufferMemory(device, m_buffer, m_memory, 0));

  if (m_mappable){
    VK_CHECK(vkMapMemory(device, m_memory, 0, m_size, 0, (void**)&m_mappedData));

    if (createInfo.initialData) {
      update(createInfo.initialData, createInfo.initialDataSize);
    }
  }
}

/*
*/
VulkanBuffer::~VulkanBuffer() {
  if (m_device == VK_NULL_HANDLE) {
    return;
  }
  VkResult result = vkDeviceWaitIdle(m_device);
  if (result != VK_SUCCESS) {
    CH_LOG_ERROR(Vulkan, "VulkanBuffer::Destructor: Failed to wait for device idle.");
  }

  if (m_buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(m_device, m_buffer, nullptr);
    m_buffer = VK_NULL_HANDLE;
  }

  if (m_mappedData && m_mappable) {
    vkUnmapMemory(m_device, m_memory);
    m_mappedData = nullptr;
  }

  if (m_memory != VK_NULL_HANDLE) {
    vkFreeMemory(m_device, m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanBuffer::update(const void* data, SIZE_T size, uint32 offset) {
  if (m_mappable && m_mappedData) {
    memcpy(static_cast<uint8*>(m_mappedData) + offset, data, size);
  }
  else if (m_mappable) {
    void* mappedData = nullptr;
    VK_CHECK(vkMapMemory(m_device, m_memory, offset, size, 0, &mappedData));
    memcpy(static_cast<uint8*>(mappedData), data, size);
    vkUnmapMemory(m_device, m_memory);
  }
  else {
    CH_LOG_ERROR(Vulkan, "Buffer is not mappable");
  }
}
} // namespace chEngineSDK
