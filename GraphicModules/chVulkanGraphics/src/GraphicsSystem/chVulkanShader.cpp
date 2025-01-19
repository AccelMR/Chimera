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
  m_name = desc.name;

  auto createShaderModule = [&](const std::vector<uint8>& byteCode, VkShaderModule& shaderModule) {
    if (!byteCode.empty()) {
      VkShaderModuleCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = byteCode.size();
      createInfo.pCode = reinterpret_cast<const uint32*>(byteCode.data());

      if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        CH_EXCEPT(InternalErrorException, "Failed to create Vulkan shader module!");
      }
    }
  };
  
  createShaderModule(desc.VS.byteCode, m_vertexShader);
  createShaderModule(desc.PS.byteCode, m_pixelShader);
  createShaderModule(desc.CS.byteCode, m_computeShader);
  createShaderModule(desc.MS.byteCode, m_meshShader);
}

} // namespace chEngineSDK