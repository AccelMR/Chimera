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
#include "chVulkanGPUCommandBuffer.h"

#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"
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
  pipelineState;

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = (commandBufferType == COMMAND_BUFFER_TYPES::kDIRECT) ? 
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY : 
                    VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;
  throwIfFailed(vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer));

  VkCommandBufferBeginInfo beginInfo = {}; 
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  throwIfFailed(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

/*
 * TODO: Implement flags
 */
void
VulkanGPUCommandBuffer::_internalReset(const SPtr<GPUPipelineState>& pipelineState) {
  pipelineState;
  VkCommandBufferResetFlagBits flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
  vkResetCommandBuffer(m_commandBuffer, flags);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalclose() {
  vkEndCommandBuffer(m_commandBuffer);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) {
  pipelineState;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) {
  buff;
  slot;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot) {
  buff;
  slot;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetRect(const Box2D& rect) {
  rect;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetScissorRect(const Box2D& rect) {
  rect;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetRenderTarget(const SPtr<Texture>& rt) {
  rt;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetRenderTargets(const Vector<SPtr<Texture>>& rts) {
  rts;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalclearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color) {
  rt;
  color;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalclearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color) {
  rts;
  color;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetTopology(PRIMITIVE_TOPOLOGY_TYPE topology) {
  VkPrimitiveTopology vkTopology = VulkanTranslator::get(topology);
  vkCmdSetPrimitiveTopology(m_commandBuffer, vkTopology);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalsetVertexBuffer(uint32 slot, 
                                                 uint32 numBuffers,
                                                 const SPtr<VertexBuffer>& buffer) {
  slot;
  numBuffers;
  buffer;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetIndexBuffer(const SPtr<IndexBuffer>& buffer) {
  buffer;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) {
  bindingGroup;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalDrawIndexed(uint32 indexCountPerInstance,
                                    uint32 instanceCount,
                                    uint32 startIndexLocation,
                                    int32 baseVertexLocation,
                                    uint32 startIstanceLocation) {
  vkCmdDrawIndexed(m_commandBuffer, 
                   indexCountPerInstance, 
                   instanceCount, 
                   startIndexLocation, 
                   baseVertexLocation, 
                   startIstanceLocation);
}

/*
*/
void
VulkanGPUCommandBuffer::_internaldrawInstanced(uint32 vertexCountPerInstance,
                                      uint32 instanceCount,
                                      uint32 startVertexLocation,
                                      uint32 startInstanceLocation) {
  vkCmdDraw(m_commandBuffer, 
            vertexCountPerInstance, 
            instanceCount, 
            startVertexLocation, 
            startInstanceLocation);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalResourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers) {
  barriers;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClearSwapChainTexture(const LinearColor& color) {
  color;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetSwapChain() {}

/*
*/
void
VulkanGPUCommandBuffer::_internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) {
  barriers;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSyncFence(const UnqPtr<Fence>& fence, uint64 value) {
  throwIfFailed(m_commandBuffer->Signal(fence->m_fence, value));
  
}

} // namespace chEngineSDK