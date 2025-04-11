/************************************************************************/
/**
 * @file chIBuffer.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Interface for the buffer. This is the base class for all buffers.
 * It is used to create buffers and allocate them.
 * It is also used to reset buffers and free them.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIBuffer.h"

namespace chEngineSDK {
class VulkanBuffer : public IBuffer {
 public:
  VulkanBuffer(VkDevice device,
               VkPhysicalDevice physicalDevice, 
               const BufferCreateInfo& createInfo);

  ~VulkanBuffer();

  NODISCARD virtual SIZE_T
  getSize() const override { return m_size; }

  virtual void
  update(const void* data, SIZE_T size, uint32 offset = 0) override;

  NODISCARD FORCEINLINE VkBuffer
  getHandle() const { return m_buffer; }

 private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  SIZE_T m_size = 0;
  bool m_mappable = false;
  void* m_mappedData = nullptr;
};
} // namespace chEngineSDK