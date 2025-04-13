/************************************************************************/
/**
 * @file chVulkanShader.cpp
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Vulkan shader implementation.
 * This file contains the implementation of the shader interface
 * for Vulkan.
 */
/************************************************************************/

#include "chVulkanShader.h"

#include "chStringUtils.h"

namespace chEngineSDK {
/*
*/
VulkanShader::VulkanShader(VkDevice device, const ShaderCreateInfo& createInfo)
    : m_device(device), m_type(createInfo.stage), m_entryPoint(createInfo.entryPoint) {
  VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = createInfo.sourceCode.size(),
      .pCode = reinterpret_cast<const uint32*>(createInfo.sourceCode.data()),
  };

  VK_CHECK(vkCreateShaderModule(m_device, &shaderModuleCreateInfo, nullptr, &m_shaderModule));
}

/*
*/
VulkanShader::~VulkanShader() {
  if (m_shaderModule) {
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    m_shaderModule = VK_NULL_HANDLE;
  }
}


} // namespace chEngineSDK