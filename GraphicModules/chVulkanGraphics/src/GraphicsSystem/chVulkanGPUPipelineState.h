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


  FORCEINLINE VkPipeline
  getPipeline() const {
    return m_pipeline;
  }

  FORCEINLINE VkPipelineLayout
  getPipelineLayout() const {
    return m_pipelineLayout;
  }

  FORCEINLINE VkDescriptorSet
  getDescriptorSet(uint32 index = 0) const {
    CH_ASSERT(index < m_descriptorSets.size());
    return m_descriptorSets[index];
  }

  FORCEINLINE VkRenderPass
  getRenderPass() const {
    return m_renderPass;
  }

 protected:
  friend class GraphicsModuleVulkan;
  friend class VulkanGPUResourceModule;

  void
  _init(const chGPUDesc::PipelineStateDesc& desc);

  void
  createRenderPass(const chGPUDesc::RenderPassDesc& desc);

  void
  createPipelineLayout(const Vector<chGPUDesc::BindingGroup>& bindingGroups);

  void
  createDescriptorPool(const Vector<chGPUDesc::BindingGroup>& bindingGroups);

  void
  createVertexInputState(const chGPUDesc::PipelineStateDesc& desc, 
                         VkPipelineVertexInputStateCreateInfo& vertexInputInfo);

  void
  createInputAssemblyState(const chGPUDesc::PipelineStateDesc& desc, 
                          VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo);

  void
  createViewportState(const chGPUDesc::PipelineStateDesc& desc, 
                      VkPipelineViewportStateCreateInfo& viewportInfo,
                      Vector<VkViewport>& viewports,
                      Vector<VkRect2D>& scissors);

  void
  createRasterizerState(const chGPUDesc::PipelineStateDesc& desc, 
                        VkPipelineRasterizationStateCreateInfo& rasterizationInfo);

  void
  createMultisamplingState(const chGPUDesc::PipelineStateDesc& desc, 
                           VkPipelineMultisampleStateCreateInfo& multisampleInfo);

  void
  createDepthStencilState(const chGPUDesc::PipelineStateDesc& desc, 
                         VkPipelineDepthStencilStateCreateInfo& depthStencilInfo);

  void
  createColorBlendState(const chGPUDesc::PipelineStateDesc& desc, 
                       VkPipelineColorBlendStateCreateInfo& colorBlendInfo,
                       Vector<VkPipelineColorBlendAttachmentState>& blendAttachments);


 private:
  VkPipeline m_pipeline;
  VkPipelineLayout m_pipelineLayout;
  VkRenderPass m_renderPass;
  Vector<VkDescriptorSetLayout> m_descriptorSetLayouts; 

  Vector<VkVertexInputAttributeDescription> m_vertexInputAttributes;
  Vector<VkVertexInputBindingDescription> m_vertexInputBindings;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  Vector<VkDescriptorSet> m_descriptorSets;
};
} // namespace chEngineSDK
