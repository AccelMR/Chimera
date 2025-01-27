/************************************************************************/
/**
 * @file chVulkanShader.cpp
 * @author AccelMR
 * @date 2024/12/ 17
 * @brief Vulkan implementation of a GPU shader.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanShader.h"

#include "chGPUResourceModule.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"


namespace chEngineSDK
{

/*
*/
void
VulkanShader::_init(const chGPUDesc::ShaderDesc& desc) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();
  const Vector<uint8>& byteCode = desc.byteCode;

  CH_ASSERT(byteCode.size() > 0);
  CH_ASSERT(!desc.entryFunc.empty());
  
  m_entryPoint = desc.entryFunc;

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = byteCode.size();
  createInfo.pCode = reinterpret_cast<const uint32*>(byteCode.data());

  throwIfFailed(vkCreateShaderModule(device, &createInfo, nullptr, &m_shaderModule));
}

} // namespace chEngineSDK