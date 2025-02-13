/************************************************************************/
/**
 * @file chVulkanIndexBuffer.cpp
 * @author AccelMR
 * @date 2025/01/27
 *   Index buffer implementation for Vulkan.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanIndexBuffer.h"

#include "chVulkanGraphicsModule.h"
#include "chDebug.h"

namespace chEngineSDK {

/*
*/
void
VulkanIndexBuffer::_internalUpdate(const chGPUDesc::IndexBufferDesc& desc, 
                                   const IndexBufferData data) {
}

/*
*/
void
VulkanIndexBuffer::_internalInit(const chGPUDesc::IndexBufferDesc& desc) {
  VkDevice device = g_VulkanGraphicsModule().getDevice();

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = desc.size;
  bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (desc.format == FORMAT::kR16_UINT) {
    m_indexType = VK_INDEX_TYPE_UINT16;
  } 
  else if (desc.format == FORMAT::kR32_UINT) {
    m_indexType = VK_INDEX_TYPE_UINT32;
  } 
  else {
    CH_LOG_ERROR("Invalid index format.");
    CH_ASSERT(false);
    return;
  }

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
    CH_ASSERT(false);
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = g_VulkanGraphicsModule().findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &m_bufferMemory) != VK_SUCCESS) {
    CH_ASSERT(false);
  }

  vkBindBufferMemory(device, m_buffer, m_bufferMemory, 0);
}
} // namespace chEngineSDK