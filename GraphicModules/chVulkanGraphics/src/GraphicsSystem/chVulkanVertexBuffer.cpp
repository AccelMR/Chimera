/************************************************************************/
/**
 * @file chNulkanVertexBuffer.cpp
 * @author AccelMR
 * @date 2025/01/27
 *   Vertex buffer implementation for Vulkan.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanVertexBuffer.h"

#include "chVulkanGraphicsModule.h"

namespace chEngineSDK {
/*
*/
void
VulkanVertexBuffer::init(const chGPUDesc::VertexBufferDesc& desc) {
  VkDevice device = g_VulkanGraphicsModule().getDevice();

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = desc.size;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  throwIfFailed(vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer));

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = g_VulkanGraphicsModule().findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  throwIfFailed(vkAllocateMemory(device, &allocInfo, nullptr, &m_bufferMemory));
  vkBindBufferMemory(device, m_buffer, m_bufferMemory, 0);
}

/*
*/
void
VulkanVertexBuffer::_internalUpdate(const chGPUDesc::VertexBufferDesc& desc, 
                                    const VertexBufferData data) {
  VkDevice device = g_VulkanGraphicsModule().getDevice();

  // Copy the triangle data to the vertex buffer.
  void* pVertexDataBegin;
  throwIfFailed(vkMapMemory(device, m_bufferMemory, 0, desc.size, 0, &pVertexDataBegin));
  memcpy(pVertexDataBegin, data, desc.size);
  vkUnmapMemory(device, m_bufferMemory);
}

} // namespace chEngineSDK