/************************************************************************/
/**
 * @file chNulkanVertexBuffer.h
 * @author AccelMR
 * @date 2025/01/27
 *   Vertex buffer implementation for Vulkan.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chVertexBuffer.h"

namespace chEngineSDK {
class VulkanVertexBuffer : public VertexBuffer
{
 public:
  VulkanVertexBuffer() = default;
  ~VulkanVertexBuffer() = default;

  void
  init(const chGPUDesc::VertexBufferDesc& desc);

  void
  _internalUpdate(const chGPUDesc::VertexBufferDesc& desc, 
                  const VertexBufferData data) override;

 private:
  VkBuffer m_buffer;
  VkDeviceMemory m_bufferMemory;
  VkDevice m_device;
};
} // namespace chEngineSDK