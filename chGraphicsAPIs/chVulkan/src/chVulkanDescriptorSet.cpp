/************************************************************************/
/**
 * @file chVulkanDescriptorSet.cpp
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan descriptor set class. This class is used to manage the
 *  descriptor set.
 */
/************************************************************************/
#include "chVulkanDescriptorSet.h"

namespace chEngineSDK {

VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet)
    : m_device(device), m_descriptorSet(descriptorSet) {
}

VulkanDescriptorSet::~VulkanDescriptorSet() {}
} // namespace chEngineSDK