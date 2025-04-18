/************************************************************************/
/**
 * @file chPipelineLayout.cpp
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 * Pipeline layout class. This class is used to manage the pipeline layout
 * 
 */
/************************************************************************/
#include "chVulkanPipelineLayout.h"
#include "chVulkanDescriptorSetLayout.h"
#include "chVulkanAPI.h"

namespace chEngineSDK {

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, 
                                         const Vector<SPtr<IDescriptorSetLayout>>& setLayouts)
    : m_device(device), m_ownsPipelineLayout(true) {
  Vector<VkDescriptorSetLayout> vkLayouts;
  vkLayouts.reserve(setLayouts.size());
  
  for (const auto& layout : setLayouts) {
    auto vulkanLayout = std::static_pointer_cast<VulkanDescriptorSetLayout>(layout);
    vkLayouts.push_back(vulkanLayout->getHandle());
  }
  
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(vkLayouts.size());
  pipelineLayoutInfo.pSetLayouts = vkLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;
  
  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
  if (m_pipelineLayout != VK_NULL_HANDLE && m_ownsPipelineLayout) {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }
}
} // namespace chEngineSDK