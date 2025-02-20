/************************************************************************/
/**
 * @file chVulkanGPUBuffer.cpp
 * @author AccelMR
 * @date 2024/12/11
 *   Vulkan implementation of a GPU buffer.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanGPUBuffer.h"

#include "chDebug.h"
#include "chVulkanGraphicsModule.h"

namespace chEngineSDK {
/*
*/
uint32
findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {

  // Obtener las propiedades de memoria del dispositivo físico
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  // Iterar sobre los tipos de memoria disponibles
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i; // Encontró un tipo de memoria compatible
      }
  }

  CH_EXCEPT(InternalErrorException, "Failed to find suitable memory type!");
}


/*
*/
VulkanGPUBuffer::~VulkanGPUBuffer() {
  const VkDevice& device = g_VulkanGraphicsModule().getDevice();

  if (m_buffer) {
    vkDestroyBuffer(device, m_buffer, nullptr);
    m_buffer = VK_NULL_HANDLE;
  }

  if (m_memory) {
    vkFreeMemory(device, m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanGPUBuffer::_init(const SIZE_T& size) {
  GraphicsModuleVulkan& vulkanGraphicsModule = g_VulkanGraphicsModule();
  const VkDevice& device = vulkanGraphicsModule.getDevice();
  const VkPhysicalDevice& physicalDevice = vulkanGraphicsModule.getPhysicalDevice();

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
    CH_LOG_ERROR("Failed to create buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = vulkanGraphicsModule.findMemoryType(memRequirements.memoryTypeBits,
                                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
    CH_LOG_ERROR("Failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, m_buffer, m_memory, 0);
}

/*
*/
void
VulkanGPUBuffer::_internalUpdate(const SIZE_T& size, const GPUBufferData data) {
  const VkDevice& device = g_VulkanGraphicsModule().getDevice();

  void* mappedData;
  vkMapMemory(device, m_memory, 0, size, 0, &mappedData);
  memcpy(mappedData, data, size);
  vkUnmapMemory(device, m_memory);
}

} // namespace chEngineSDK
