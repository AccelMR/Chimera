/************************************************************************/
/**
 * @file chVulkanBuffer.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of IBuffer.
 */
/************************************************************************/
#include "chVulkanBuffer.h"

#include <cstring>

#include <vk_mem_alloc.h>

#include "chVulkanAPI.h"

namespace chEngineSDK {
namespace {
VkBufferUsageFlags
toVkBufferUsage(BufferUsageFlags usage)
{
  VkBufferUsageFlags vkUsage = 0;
  if (usage.isSet(BufferUsage::VertexBuffer)) {
    vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if (usage.isSet(BufferUsage::IndexBuffer)) {
    vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if (usage.isSet(BufferUsage::UniformBuffer)) {
    vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if (usage.isSet(BufferUsage::StorageBuffer)) {
    vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }
  if (usage.isSet(BufferUsage::TransferSrc)) {
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (usage.isSet(BufferUsage::TransferDst)) {
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  return vkUsage;
}

VmaAllocationCreateFlags
toVmaAllocationFlags(MemoryUsage memoryUsage)
{
  switch (memoryUsage) {
  case MemoryUsage::CpuOnly:
  case MemoryUsage::CpuToGpu:
    return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
           VMA_ALLOCATION_CREATE_MAPPED_BIT;
  case MemoryUsage::GpuToCpu:
    return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
  case MemoryUsage::GpuOnly:
  default:
    return 0;
  }
}
} // namespace

/*
 */
VulkanBuffer::VulkanBuffer(VmaAllocator allocator, const BufferCreateInfo& createInfo)
  : m_allocator(allocator),
    m_size(createInfo.size)
{
  const VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .size = m_size,
                                      .usage = toVkBufferUsage(createInfo.usage),
                                      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                      .queueFamilyIndexCount = 0,
                                      .pQueueFamilyIndices = nullptr};

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = createInfo.memoryUsage == MemoryUsage::GpuOnly
                             ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
                             : VMA_MEMORY_USAGE_AUTO;
  allocationInfo.flags = toVmaAllocationFlags(createInfo.memoryUsage);

  VmaAllocationInfo allocationResult{};
  VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocationInfo, &m_buffer,
                           &m_allocation, &allocationResult));
  m_mappedData = allocationResult.pMappedData;

  // A buffer can hold one descriptor type per index; uniform wins when both are asked for.
  const bool isUniform = createInfo.usage.isSet(BufferUsage::UniformBuffer);
  if (isUniform || createInfo.usage.isSet(BufferUsage::StorageBuffer)) {
    VulkanBindlessHeap& bindlessHeap = g_vulkanAPI().getBindlessHeap();
    m_bindlessIndex = bindlessHeap.allocateResourceIndex();
    if (isUniform) {
      bindlessHeap.writeUniformBuffer(m_bindlessIndex, m_buffer, m_size);
    }
    else {
      bindlessHeap.writeStorageBuffer(m_bindlessIndex, m_buffer, m_size);
    }
  }

  if (createInfo.initialData) {
    update(createInfo.initialData, createInfo.initialDataSize);
  }
}

/*
 */
VulkanBuffer::~VulkanBuffer()
{
  VulkanDeletionQueue& deletionQueue = g_vulkanAPI().getDeletionQueue();
  deletionQueue.enqueueBindlessIndex(m_bindlessIndex, false);
  deletionQueue.enqueue(VK_OBJECT_TYPE_BUFFER, m_buffer, m_allocation);
}

/*
 */
void
VulkanBuffer::update(const void* data, SIZE_T size, uint32 offset)
{
  if (m_mappedData == nullptr) {
    CH_LOG_ERROR(Vulkan, "Buffer is not mappable");
    return;
  }

  CH_ASSERT(offset + size <= m_size);
  memcpy(static_cast<uint8*>(m_mappedData) + offset, data, size);
  // Does nothing on host coherent memory, which VMA may not have picked.
  VK_CHECK(vmaFlushAllocation(m_allocator, m_allocation, offset, size));
}

} // namespace chEngineSDK
