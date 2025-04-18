/************************************************************************/
/**
 * @file chVulkanDescriptorSetLayout.cpp
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan descriptor set layout class. This class is used to manage the
 *  descriptor set.
 * 
 */
/************************************************************************/

#include "chVulkanDescriptorSetLayout.h"

#include "chVulkanAPI.h"

namespace chEngineSDK {

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, 
                                                    const DescriptorSetLayoutCreateInfo& createInfo)
    : m_device(device) {
  Vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(createInfo.bindings.size());

  for (const auto& binding : createInfo.bindings) {
    VkDescriptorSetLayoutBinding vkBinding{};
    vkBinding.binding = binding.binding;
    
    switch (binding.type) {
      case DescriptorType::UniformBuffer:
        vkBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
      case DescriptorType::StorageBuffer:
        vkBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
      case DescriptorType::CombinedImageSampler:
        vkBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        break;
    }
    
    vkBinding.descriptorCount = binding.count;
    
    VkShaderStageFlags stageFlags = 0;
    if (binding.stageFlags.isSet(ShaderStage::Vertex)) {
      stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (binding.stageFlags.isSet(ShaderStage::Fragment)) {
      stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (binding.stageFlags.isSet(ShaderStage::Compute)) {
      stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    vkBinding.stageFlags = stageFlags;
    vkBinding.pImmutableSamplers = nullptr;
    
    bindings.push_back(vkBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
  if (m_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;
  }
}
} // namespace chEngineSDK