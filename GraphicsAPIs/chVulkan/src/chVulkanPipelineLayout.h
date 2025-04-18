/************************************************************************/
/**
 * @file chVulkanPipelineLayout.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Vulkan pipeline layout class. This class is used to manage the pipeline layout
 *  for the graphics pipeline. It is used to describe the format of the
 *  pipeline data and how it is laid out in memory.
 */
/************************************************************************/
#pragma once
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIPipelineLayout.h"

namespace chEngineSDK {
class VulkanPipelineLayout : public IPipelineLayout {
public:
  VulkanPipelineLayout(VkDevice device, const Vector<SPtr<IDescriptorSetLayout>>& setLayouts);
  VulkanPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
    : m_device(device), m_pipelineLayout(pipelineLayout), m_ownsPipelineLayout(false) {}
  ~VulkanPipelineLayout() override;

  NODISCARD FORCEINLINE VkPipelineLayout
  getHandle() const { return m_pipelineLayout; }

private:
  bool m_ownsPipelineLayout = true;
  VkDevice m_device = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};
} // namespace chEngineSDK