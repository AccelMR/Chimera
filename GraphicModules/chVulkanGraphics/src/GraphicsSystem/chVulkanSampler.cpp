/************************************************************************/
/**
 * @file chVulkanSampler.cpp
 * @author AccelMR
 * @date 2025/01/26
 *   Vulkan implementation of a GPU sampler.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanSampler.h"

#include "chVulkanTranslator.h"

namespace chEngineSDK {
/*
*/
void
VulkanSampler::_internalInit(const chGPUDesc::SamplerDesc& desc) {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VulkanTranslator::get(desc.magFilter);
  samplerInfo.minFilter = VulkanTranslator::get(desc.minFilter);
  samplerInfo.addressModeU = VulkanTranslator::get(desc.addressU);
  samplerInfo.addressModeV = VulkanTranslator::get(desc.addressV);
  samplerInfo.addressModeW = VulkanTranslator::get(desc.addressW);
  samplerInfo.anisotropyEnable = desc.anisotropyEnable ? VK_TRUE : VK_FALSE;
  samplerInfo.maxAnisotropy = desc.maxAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = desc.compareEnable ? VK_TRUE : VK_FALSE;
  samplerInfo.compareOp = VulkanTranslator::get(desc.comparisonFunc);

  switch (desc.mipMapMode) {
    case chGPUDesc::FILTER::kLINEAR:
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    case chGPUDesc::FILTER::kNEAREST:
    default:
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
  }

  samplerInfo.mipLodBias = desc.mipLODBias;
  samplerInfo.minLod = desc.minLOD;
  samplerInfo.maxLod = desc.maxLOD;

  VkDevice device = g_VulkanGraphicsModule().getDevice();
  VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
  throwIfFailed(result);
}
} // namespace chEngineSDK

