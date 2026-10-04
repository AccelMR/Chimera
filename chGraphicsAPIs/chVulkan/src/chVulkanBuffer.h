/************************************************************************/
/**
 * @file chVulkanBuffer.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of IBuffer.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIBuffer.h"

namespace chEngineSDK {

/**
 * Vulkan buffer whose memory comes from VMA. Buffers the CPU writes stay mapped for their
 * whole life; the buffer and its memory are freed through the deletion queue.
 */
class VulkanBuffer : public IBuffer
{
 public:
  VulkanBuffer(VmaAllocator allocator, const BufferCreateInfo& createInfo);
  ~VulkanBuffer() override;

  NODISCARD SIZE_T
  getSize() const override
  {
    return m_size;
  }

  void
  update(const void* data, SIZE_T size, uint32 offset = 0) override;

  NODISCARD uint32
  getBindlessIndex() const override
  {
    return m_bindlessIndex;
  }

  NODISCARD FORCEINLINE VkBuffer
  getHandle() const
  {
    return m_buffer;
  }

 private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VmaAllocator m_allocator = nullptr;
  VmaAllocation m_allocation = nullptr;
  void* m_mappedData = nullptr;
  SIZE_T m_size = 0;
  uint32 m_bindlessIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
};

} // namespace chEngineSDK
