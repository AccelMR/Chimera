/************************************************************************/
/**
 * @file chVulkanGPUBuffer.h
 * @author AccelMR
 * @date 2024/12/11
 *   Vulkan implementation of a GPU buffer.
 */
 /************************************************************************/
 #pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include <chGPUBuffer.h>

namespace chEngineSDK {
class VulkanGPUBuffer final : public GPUBuffer
{
 public:
  /*
   *   Default constructor
   */
  VulkanGPUBuffer() = default;
  ~VulkanGPUBuffer();

 protected:
 friend class VulkanGPUResourceModule;

  void
  _init(const SIZE_T& size) override;

  void
  _internalUpdate(const SIZE_T& size, const GPUBufferData data) override;
  
 private:
  VkBuffer m_buffer;
  VkDeviceMemory m_memory;
};
} // namespace chEngineSDK