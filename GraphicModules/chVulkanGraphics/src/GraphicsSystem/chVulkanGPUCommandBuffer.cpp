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

#include "chVulkanVertexBuffer.h"
#include "chVulkanIndexBuffer.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanGPUBuffer.h"
#include "chVulkanTranslator.h"
#include "chVulkanTexture.h"
#include "chVulkanSwapChain.h"
#include "chVulkanGPUPipelineState.h"
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

  CH_ASSERT(m_commandPool != VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = (commandBufferType == COMMAND_BUFFER_TYPES::kDIRECT) ? 
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY : 
                    VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;
  throwIfFailed(vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer));
}

/*
*/
void
VulkanGPUCommandBuffer::_internalBegin() {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  throwIfFailed(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

/*
 * TODO: Implement flags
 */
void
VulkanGPUCommandBuffer::_internalReset(const SPtr<GPUPipelineState>& pipelineState) {
  VkCommandBufferResetFlagBits flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
  vkResetCommandBuffer(m_commandBuffer, flags);
  _internalBegin();
  _internalSetPipeLineState(pipelineState);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClose() {
  throwIfFailed(vkEndCommandBuffer(m_commandBuffer));
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) {
  m_pipelineState = std::reinterpret_pointer_cast<VulkanGPUPipelineState>(pipelineState);
  VkPipeline pipeline = m_pipelineState->getPipeline();
  m_descriptorSet = m_pipelineState->getDescriptorSet();
  m_renderPass = m_pipelineState->getRenderPass();
  vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) {
  Vector<SPtr<GPUBuffer>> buffers = {buff};
  _internalSetGPUBuffers(buffers, slot);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buffers, uint32 slot) {
  if (buffers.empty()) {
    return;
  }

  auto vulkanPipelineState = std::static_pointer_cast<VulkanGPUPipelineState>(m_pipelineState);
  VkDescriptorSet descriptorSet = vulkanPipelineState->getDescriptorSet();

  Vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());
  for (size_t i = 0; i < buffers.size(); ++i) {
    auto vulkanBuffer = std::static_pointer_cast<VulkanGPUBuffer>(buffers[i]);
    bufferInfos[i].buffer = vulkanBuffer->getBuffer();
    bufferInfos[i].offset = 0;
    bufferInfos[i].range = VK_WHOLE_SIZE;
  }

  // Actualizar descriptores
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = slot;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = static_cast<uint32>(buffers.size());
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.pBufferInfo = bufferInfos.data();

  vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

  vkCmdBindDescriptorSets(m_commandBuffer, 
                          VK_PIPELINE_BIND_POINT_GRAPHICS, 
                          vulkanPipelineState->getPipelineLayout(),
                          0, 1, &descriptorSet, 0, nullptr);
}


/*
*/
void
VulkanGPUCommandBuffer::_internalSetRect(const Box2D& rect) {
  VkViewport viewport{};
  viewport.x = static_cast<float>(rect.minPoint.x);
  viewport.y = static_cast<float>(rect.minPoint.y);
  viewport.width =  static_cast<float>(rect.getSize().x);
  viewport.height = static_cast<float>(rect.getSize().y);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetScissorRect(const Box2D& rect) {
   VkViewport viewport = {};
  viewport.x = static_cast<float>(rect.minPoint.x);
  viewport.y = static_cast<float>(rect.minPoint.y);
  viewport.width = static_cast<float>(rect.getSize().x);
  viewport.height = static_cast<float>(rect.getSize().y);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetRenderTarget(const SPtr<Texture>& rt) {
  _internalSetRenderTargets({rt});
}
/*
*/
void
VulkanGPUCommandBuffer::_internalSetRenderTargets(const Vector<SPtr<Texture>>& rts) {
  if (rts.empty()) {
    CH_LOG_ERROR("No render targets provided.");
    return;
  }

  auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(rts[0]);
  VkFramebuffer framebuffer = vulkanTexture->getFrameBuffer();
  CH_ASSERT(framebuffer != VK_NULL_HANDLE);
  CH_ASSERT(m_renderPass != VK_NULL_HANDLE);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = vulkanTexture->getExtent();

  Vector<VkClearValue> clearValues(rts.size());
  for (size_t i = 0; i < rts.size(); ++i) {
    clearValues[i].color = {0.0f, 0.0f, 0.0f, 1.0f};
  }

  renderPassInfo.clearValueCount = static_cast<uint32>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color) {
  _internalClearRenderTargets({rt}, color);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color) {
  if (rts.empty()) {
    CH_LOG_ERROR("No render targets provided.");
    return;
  }

  for (auto& rt : rts) {
    auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(rt);
    
    VkClearValue clearValue{};
    clearValue.color = {color.r, color.g, color.b, color.a};

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    vkCmdClearColorImage(m_commandBuffer, 
                         vulkanTexture->getImage(), 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                         &clearValue.color, 
                         1, 
                         &subresourceRange);
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetTopology(PRIMITIVE_TOPOLOGY_TYPE topology) {
  VkPrimitiveTopology vkTopology = VulkanTranslator::get(topology);
  vkCmdSetPrimitiveTopology(m_commandBuffer, vkTopology);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetVertexBuffer(uint32 slot, 
                                                 uint32 numBuffers,
                                                 const SPtr<VertexBuffer>& buffer) {
  SPtr<VulkanVertexBuffer> vulkanVertexBuff = 
    std::reinterpret_pointer_cast<VulkanVertexBuffer>(buffer);
  VkBuffer vertexBuffer = vulkanVertexBuff->getBuffer();
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vertexBuffer, &offset);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetIndexBuffer(const SPtr<IndexBuffer>& buffer) {
  SPtr<VulkanIndexBuffer> vulkanIndexBuff = 
    std::reinterpret_pointer_cast<VulkanIndexBuffer>(buffer);
  VkBuffer indexBuffer = vulkanIndexBuff->getBuffer();
  vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

/*
 */
void
VulkanGPUCommandBuffer::_internalSetBindingBufferGroup(const chGPUDesc::BindingGroup &bindingGroup) {

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
VulkanGPUCommandBuffer::_internalDrawInstanced(uint32 vertexCountPerInstance,
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
  CH_EXCEPT(NotImplementedException, "VulkanGPUCommandBuffer::_internalResourceBarrier");
}

/*
*/
void
VulkanGPUCommandBuffer::_internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VulkanTranslator::get(barriers.transition.stateBefore);
  barrier.newLayout = VulkanTranslator::get(barriers.transition.stateAfter);
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  auto swapChain = std::reinterpret_pointer_cast<VulkanSwapChain>(g_VulkanGraphicsModule().getSwapChain());
  CH_ASSERT(swapChain);
  CH_ASSERT(swapChain->getSwapChainImage() != VK_NULL_HANDLE);
  barrier.image = swapChain->getSwapChainImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage = 
    VulkanTranslator::getPipelineStageFlags(barriers.transition.stateBefore);
  VkPipelineStageFlags destinationStage = 
    VulkanTranslator::getPipelineStageFlags(barriers.transition.stateAfter);

  vkCmdPipelineBarrier(m_commandBuffer, 
                       sourceStage, 
                       destinationStage, 
                       0, 
                       0, 
                       nullptr, 
                       0, 
                       nullptr, 
                       1, 
                       &barrier);
}

/*
*/
bool
VulkanGPUCommandBuffer::_internalPresent(int32 syncInterval, int32 flags) {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  auto swapChain = std::reinterpret_pointer_cast<VulkanSwapChain>(rhi.getSwapChain());
  return swapChain->present(syncInterval, flags, this);
}
} // namespace chEngineSDK