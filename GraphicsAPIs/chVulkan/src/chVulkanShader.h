/************************************************************************/
/**
 * @file chVulkanShader.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Vulkan shader implementation.
 * This file contains the implementation of the shader interface
 * for Vulkan.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chIShader.h"

namespace chEngineSDK {
class VulkanShader : public IShader {
 public:
  VulkanShader(VkDevice device, const ShaderCreateInfo& createInfo);
  ~VulkanShader() override;

  NODISCARD virtual VkShaderModule
  getHandle() const {
    return m_shaderModule;
  }

  NODISCARD FORCEINLINE virtual String
  getEntryPoint() const {
    return m_entryPoint;
  }

 private:
  VkShaderModule m_shaderModule = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  ShaderStage m_type = ShaderStage::Vertex;
  String m_entryPoint;
};
} // namespace chEngineSDK