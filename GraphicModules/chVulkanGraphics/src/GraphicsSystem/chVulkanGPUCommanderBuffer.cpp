/************************************************************************/
/**
 * @file chVulkanGPUCommanderBuffer.cpp
 * @author AccelMR
 * @date 2025/01/26
 *   Vulkan specific implementation for GPUCommandBuffer.
 */
 /************************************************************************/

/************************************************************************/
/*
  * Includes
  */
/************************************************************************/
#include "chVulkanGPUCommanderBuffer.h"

#include "chVulkanGraphicsModule.h"
#include "chDebug.h"

namespace chEngineSDK {
/*
*/
VulkanGPUCommandBuffer::~VulkanGPUCommandBuffer() {
  if (m_commandBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_init(chGPUDesc::COMMAND_BUFFER_TYPES commandBufferType,
                              const SPtr<GPUPipelineState>& pipelineState) {
  m_device = g_VulkanGraphicsModule().getDevice();
  m_commandPool = g_VulkanGraphicsModule().commandPool;
}

} // namespace chEngineSDK