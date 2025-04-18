/************************************************************************/
/**
 * @file chVulkanCommandBuffer.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan command buffer implementation.
 * This file contains the implementation of the command buffer interface
 */
/************************************************************************/

#include "chVulkanCommandBuffer.h"

#include "chICommandQueue.h"
#include "chVulkanAPI.h"
#include "chVulkanBuffer.h"
#include "chVulkanDescriptorSet.h"
#include "chVulkanRenderPass.h"
#include "chVulkanFrameBuffer.h"
#include "chVulkanPipeline.h"
#include "chVulkanPipelineLayout.h"


namespace chEngineSDK {
/*
*/
VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool commandPool)
    : m_device(device), m_commandPool(commandPool) {
  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };

  VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffer));
}

/*
*/
VulkanCommandBuffer::~VulkanCommandBuffer() {
  if (m_commandBuffer != VK_NULL_HANDLE) {
    // VkResult result = vkDeviceWaitIdle(m_device);
    // g_vulkanAPI().getQueue(QueueType::Graphics)->waitIdle();
    //if (result == VK_SUCCESS) {
      vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
      m_commandBuffer = VK_NULL_HANDLE;
    //}
  }
}

/*
*/
void
VulkanCommandBuffer::begin() {
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  
  VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
  m_state = CommandBufferState::Recording;
}

/*
*/
void
VulkanCommandBuffer::end() {
  VK_CHECK(vkEndCommandBuffer(m_commandBuffer));
  m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::beginRenderPass(const RenderPassBeginInfo& beginInfo) {
  auto vulkanRenderPass = std::static_pointer_cast<VulkanRenderPass>(beginInfo.renderPass);
  auto vulkanFramebuffer = std::static_pointer_cast<VulkanFrameBuffer>(beginInfo.framebuffer);

  Vector<VkClearValue> clearValues;
  for (const auto& clearValue : beginInfo.clearValues) {
    clearValues.push_back({ 
      .color = { clearValue.r, clearValue.g, clearValue.b, clearValue.a } 
    });
  }

  VkRenderPassBeginInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = vulkanRenderPass->getHandle(),
    .framebuffer = vulkanFramebuffer->getHandle(),
    .renderArea = {
      .offset = { 0, 0 },
      .extent = { 
        beginInfo.framebuffer->getWidth(), 
        beginInfo.framebuffer->getHeight() 
      }
    },
    .clearValueCount = static_cast<uint32>(clearValues.size()),
    .pClearValues = clearValues.data()
  };

  vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::endRenderPass() {
  vkCmdEndRenderPass(m_commandBuffer);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::bindPipeline(SPtr<IPipeline> pipeline) {
  auto vulkanPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
  vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getHandle());
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::bindVertexBuffer(SPtr<IBuffer> buffer, uint32 binding, uint64 offset) {
  auto vulkanBuffer = std::static_pointer_cast<VulkanBuffer>(buffer);
  VkBuffer vkBuffer = vulkanBuffer->getHandle();
  vkCmdBindVertexBuffers(m_commandBuffer, binding, 1, &vkBuffer, &offset);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::bindIndexBuffer(SPtr<IBuffer> buffer, IndexType indexType, uint32 offset) {
  auto vulkanBuffer = std::static_pointer_cast<VulkanBuffer>(buffer);
  VkBuffer vkBuffer = vulkanBuffer->getHandle();
  VkIndexType vkIndexType = static_cast<VkIndexType>(indexType);
  vkCmdBindIndexBuffer(m_commandBuffer, vkBuffer, offset, vkIndexType);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) {
  vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::drawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) {
  vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
  VkViewport viewport = {
    .x = x,
    .y = y,
    .width = width,
    .height = height,
    .minDepth = minDepth,
    .maxDepth = maxDepth
  };

  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
  //m_state = CommandBufferState::Executable;p
}

/*
*/
void
VulkanCommandBuffer::setScissor(uint32 x, uint32 y, uint32 width, uint32 height) {
  VkRect2D scissor = {
    .offset = { static_cast<int32>(x), static_cast<int32>(y) },
    .extent = { width, height }
  };

  vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
  //m_state = CommandBufferState::Executable;
}

/*
*/
void
VulkanCommandBuffer::bindDescriptorSets(PipelineBindPoint bindPoint,
                                      SPtr<IPipelineLayout> layout,
                                      uint32 firstSet,
                                      const Vector<SPtr<IDescriptorSet>>& descriptorSets,
                                      const Vector<uint32>& dynamicOffsets) {
  auto vulkanLayout = std::static_pointer_cast<VulkanPipelineLayout>(layout);
  
  Vector<VkDescriptorSet> vkSets;
  vkSets.reserve(descriptorSets.size());
  
  for (const auto& set : descriptorSets) {
    auto vulkanSet = std::static_pointer_cast<VulkanDescriptorSet>(set);
    vkSets.push_back(vulkanSet->getHandle());
  }
  
  VkPipelineBindPoint vkBindPoint;
  switch (bindPoint) {
    case PipelineBindPoint::Graphics:
      vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      break;
    case PipelineBindPoint::Compute:
      vkBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
      break;
    default:
      vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
  
  vkCmdBindDescriptorSets(m_commandBuffer,
                          vkBindPoint,
                          vulkanLayout->getHandle(),
                          firstSet,
                          static_cast<uint32>(vkSets.size()),
                          vkSets.data(),
                          static_cast<uint32>(dynamicOffsets.size()),
                          dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
}



} // namespace chEngineSDK