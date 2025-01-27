/************************************************************************/
/**
 * @file chVulkanShader.h
 * @author AccelMR
 * @date 2024/12/ 17
 * @brief Vulkan implementation of a GPU shader.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesVulkan.h"

#include "chShader.h"

namespace chEngineSDK
{
// Forward declaration
namespace chGPUDesc {
  struct ShaderDesc;
}

/**
 * @brief Vulkan implementation of a GPU shader.
 */
class VulkanShader: public Shader
{
 public:
  VulkanShader() = default;
  virtual ~VulkanShader() = default;

  void
  _init(const chGPUDesc::ShaderDesc& desc);
  
  VkShaderModule m_shaderModule;
};

} // namespace chEngineSDK
