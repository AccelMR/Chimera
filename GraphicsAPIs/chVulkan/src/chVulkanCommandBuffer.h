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
#pragma once

#include "chVulkanPrerequisites.h"
#include "chICommandBuffer.h"

namespace chEngineSDK {
class VulkanCommandBuffer : public ICommandBuffer {
 public:
  VulkanCommandBuffer(VkDevice device, VkCommandPool commandPool);
  ~VulkanCommandBuffer() override;

  NODISCARD void*
  getRaw() const override {
    return static_cast<void*>(m_commandBuffer);
  }

  void
  begin() override;
  void
  end() override;

  void
  beginRenderPass(const RenderPassBeginInfo& beginInfo) override;

  void
  endRenderPass() override;

  void
  bindPipeline(SPtr<IPipeline> pipeline) override;

  void
  bindVertexBuffer(SPtr<IBuffer> buffer,
                   uint32 binding = 0,
                   uint64 offset = 0) override;

  void
  bindIndexBuffer(SPtr<IBuffer> buffer,
                  IndexType indexType,
                  uint32 offset = 0) override;

  void
  draw(uint32 vertexCount,
       uint32 instanceCount = 1,
       uint32 firstVertex = 0,
       uint32 firstInstance = 0) override;

  void
  drawIndexed(uint32 indexCount,
              uint32 instanceCount = 1,
              uint32 firstIndex = 0,
              int32 vertexOffset = 0,
              uint32 firstInstance = 0) override;

  void
  setViewport(float x, float y,
              float width, float height,
              float minDepth = 0.0f, float maxDepth = 1.0f) override;

  void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) override;

  void
  bindDescriptorSets(PipelineBindPoint bindPoint,
                     SPtr<IPipelineLayout> layout,
                     uint32 firstSet,
                     const Vector<SPtr<IDescriptorSet>>& descriptorSets,
                     const Vector<uint32>& dynamicOffsets = {}) override;


  NODISCARD CommandBufferState
  getState() const {
    return m_state;
  }

  NODISCARD VkCommandBuffer
  getHandle() const {
    return m_commandBuffer;
  }

 private:
  VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  CommandBufferState m_state = CommandBufferState::Initial;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
};
} // namespace chEngineSDK
