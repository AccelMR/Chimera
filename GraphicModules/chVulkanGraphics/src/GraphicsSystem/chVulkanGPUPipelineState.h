/*****************************************************************************/
/**
 * @file    chVulkanGPUPipelineState.h
 * @author  AccelMR
 * @date    2024/12/11
 * @brief   Vulkan implementation of a GPU pipeline state.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "chPrerequisitesVulkan.h"

#include <chGPUPipelineState.h>

namespace chEngineSDK {

class VulkanGPUPipelineState final : public GPUPipelineState
{
 public:
  /**
   *   Default constructor
   */
  VulkanGPUPipelineState() = default;
  ~VulkanGPUPipelineState();

 protected:
  friend class GraphicsModuleVulkan;
  friend class VulkanGPUResourceModule;

  void
  _init(const chGPUDesc::PipelineStateDesc& desc);

  void
  createRenderPass(const chGPUDesc::RenderPassDesc& desc);

  void
  createPipelineLayout(const chGPUDesc::BindingGroup& bindingGroup);

 private:
  VkPipeline m_pipeline;
  VkPipelineLayout m_pipelineLayout;
  VkRenderPass m_renderPass;
  Vector<VkDescriptorSetLayout> m_descriptorSetLayouts; 
};
} // namespace chEngineSDK