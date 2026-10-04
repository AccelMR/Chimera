/************************************************************************/
/**
 * @file chVulkanCommandList.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of ICommandList.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chICommandList.h"

namespace chEngineSDK {

/**
 * Vulkan command list. It keeps the bindless set and the shared pipeline layout, so
 * begin() binds the heap once and pushConstants() needs no pipeline.
 */
class VulkanCommandList : public ICommandList
{
 public:
  VulkanCommandList(VkDevice device, VkCommandPool commandPool);
  ~VulkanCommandList() override;

  VulkanCommandList(const VulkanCommandList&) = delete;
  VulkanCommandList&
  operator=(const VulkanCommandList&) = delete;

  /**
   * Also binds the bindless heap, so every pipeline bound afterwards can read it.
   */
  void
  begin();

  void
  end();

  void
  beginRendering(const RenderingDesc& desc) override;

  void
  endRendering() override;

  void
  barrier(Span<const TextureBarrier> textureBarriers) override;

  void
  bindPipeline(const IPipeline& pipeline) override;

  void
  pushConstants(const void* data, uint32 size, uint32 offset = 0) override;

  void
  bindVertexBuffer(const IBuffer& buffer, uint32 binding = 0, uint64 offset = 0) override;

  void
  bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset = 0) override;

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
  setViewport(float x,
              float y,
              float width,
              float height,
              float minDepth = 0.0f,
              float maxDepth = 1.0f) override;

  void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) override;

  NODISCARD FORCEINLINE VkCommandBuffer
  getHandle() const
  {
    return m_commandBuffer;
  }

 private:
  VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSet m_bindlessSet = VK_NULL_HANDLE;
};

} // namespace chEngineSDK
