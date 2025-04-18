/************************************************************************/
/**
 * @file chVulkanDescriptorPool.cpp
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan Descriptor pool class. This class is used to manage descriptor sets
 *  and allocate them from the pool.
 * 
 */
/************************************************************************/


#include "chVulkanDescriptorPool.h"
#include "chVulkanDescriptorSet.h"
#include "chVulkanDescriptorSetLayout.h"
#include "chVulkanAPI.h"

namespace chEngineSDK {

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, 
                                          const DescriptorPoolCreateInfo& createInfo)
    : m_device(device) {
  Vector<VkDescriptorPoolSize> poolSizes;
  poolSizes.reserve(createInfo.poolSizes.size());

  for (const auto& size : createInfo.poolSizes) {
    VkDescriptorPoolSize poolSize{};
    
    switch (size.first) {
      case DescriptorType::UniformBuffer:
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
      case DescriptorType::StorageBuffer:
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
      case DescriptorType::CombinedImageSampler:
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        break;
    }
    
    poolSize.descriptorCount = size.second;
    poolSizes.push_back(poolSize);
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = createInfo.maxSets;
  
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool));
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
  if (m_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
  }
}

SPtr<IDescriptorSet> 
VulkanDescriptorPool::allocateDescriptorSet(const DescriptorSetAllocateInfo& allocInfo) {
  auto vulkanLayout = std::static_pointer_cast<VulkanDescriptorSetLayout>(allocInfo.layout);
  
  VkDescriptorSetLayout layout = vulkanLayout->getHandle();
  
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = m_pool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;
  
  VkDescriptorSet descriptorSet;
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocateInfo, &descriptorSet));
  
  return chMakeShared<VulkanDescriptorSet>(m_device, descriptorSet);
}
} // namespace chEngineSDK