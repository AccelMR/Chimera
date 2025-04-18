/************************************************************************/
/**
 * @file chVulkanSampler.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan sampler class. This class is used to manage the Vulkan sampler
 * and its properties. It is used to create samplers and allocate them.
 */
/************************************************************************/
#include "chVulkanSampler.h"

#include "chVulkanAPI.h"

namespace chEngineSDK {

VulkanSampler::VulkanSampler(VkDevice device, const SamplerCreateInfo& createInfo)
    : m_device(device) {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  
  // Convertir filtros
  switch (createInfo.magFilter) {
    case SamplerFilter::Nearest:
      samplerInfo.magFilter = VK_FILTER_NEAREST;
      break;
    case SamplerFilter::Linear:
      samplerInfo.magFilter = VK_FILTER_LINEAR;
      break;
  }
  
  switch (createInfo.minFilter) {
    case SamplerFilter::Nearest:
      samplerInfo.minFilter = VK_FILTER_NEAREST;
      break;
    case SamplerFilter::Linear:
      samplerInfo.minFilter = VK_FILTER_LINEAR;
      break;
  }
  
  // Convertir modos de direccionamiento
  switch (createInfo.addressModeU) {
    case SamplerAddressMode::Repeat:
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      break;
    case SamplerAddressMode::MirroredRepeat:
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      break;
    case SamplerAddressMode::ClampToEdge:
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      break;
    case SamplerAddressMode::ClampToBorder:
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
      break;
    case SamplerAddressMode::MirrorClampToEdge:
      samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
      break;
  }
  
  switch (createInfo.addressModeV) {
    case SamplerAddressMode::Repeat:
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      break;
    case SamplerAddressMode::MirroredRepeat:
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      break;
    case SamplerAddressMode::ClampToEdge:
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      break;
    case SamplerAddressMode::ClampToBorder:
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
      break;
    case SamplerAddressMode::MirrorClampToEdge:
      samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
      break;
  }
  
  switch (createInfo.addressModeW) {
    case SamplerAddressMode::Repeat:
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      break;
    case SamplerAddressMode::MirroredRepeat:
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      break;
    case SamplerAddressMode::ClampToEdge:
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      break;
    case SamplerAddressMode::ClampToBorder:
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
      break;
    case SamplerAddressMode::MirrorClampToEdge:
      samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
      break;
  }
  
  // Configurar otros parámetros
  samplerInfo.anisotropyEnable = createInfo.anisotropyEnable ? VK_TRUE : VK_FALSE;
  samplerInfo.maxAnisotropy = createInfo.maxAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; // Simplificado por ahora
  samplerInfo.unnormalizedCoordinates = createInfo.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;
  samplerInfo.compareEnable = createInfo.compareEnable ? VK_TRUE : VK_FALSE;
  
  // Convertir operación de comparación
  switch (createInfo.compareOp) {
    case CompareOp::Never:
      samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
      break;
    case CompareOp::Less:
      samplerInfo.compareOp = VK_COMPARE_OP_LESS;
      break;
    case CompareOp::Equal:
      samplerInfo.compareOp = VK_COMPARE_OP_EQUAL;
      break;
    case CompareOp::LessOrEqual:
      samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      break;
    case CompareOp::Greater:
      samplerInfo.compareOp = VK_COMPARE_OP_GREATER;
      break;
    case CompareOp::NotEqual:
      samplerInfo.compareOp = VK_COMPARE_OP_NOT_EQUAL;
      break;
    case CompareOp::GreaterOrEqual:
      samplerInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
      break;
    case CompareOp::AlwaysOp:
      samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
      break;
  }
  
  // Configurar opciones de mipmapping
  switch (createInfo.mipmapMode) {
    case SamplerMipmapMode::Nearest:
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case SamplerMipmapMode::Linear:
      samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
  }
  
  samplerInfo.mipLodBias = createInfo.mipLodBias;
  samplerInfo.minLod = createInfo.minLod;
  samplerInfo.maxLod = createInfo.maxLod;
  
  VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler));
}

VulkanSampler::~VulkanSampler() {
  if (m_sampler != VK_NULL_HANDLE) {
    vkDestroySampler(m_device, m_sampler, nullptr);
    m_sampler = VK_NULL_HANDLE;
  }
}

} // namespace chEngineSDK