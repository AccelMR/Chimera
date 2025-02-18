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

#include "chGPUResourceDescriptors.h"
#include <chGPUPipelineState.h>

namespace chEngineSDK {
class RenderPass;
class VulkanRenderPass;

class VulkanGPUPipelineState final : public GPUPipelineState {
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

  FORCEINLINE SPtr<RenderPass>
  getRenderPass() const {
    return std::reinterpret_pointer_cast<RenderPass>(m_renderPass);
  }

  FORCEINLINE chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE
  getTopology() const {
    return m_topology;
  }

  FORCEINLINE SPtr<Framebuffer>
  getFramebuffer() const {
    return m_framebuffer;
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
  createColorBlendState(const chGPUDesc::BlendStateDesc& blendStateDesc);


 private:
  VkPipeline m_pipeline;
  VkPipelineLayout m_pipelineLayout;
  VkPipelineColorBlendStateCreateInfo m_blendStateInfo;
  Vector<VkPipelineColorBlendAttachmentState> m_blendAttachments;
  SPtr<VulkanRenderPass> m_renderPass;
  int32 m_subPassIndex = INVALID_INDEX;
  SPtr<Framebuffer> m_framebuffer;
  Vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
  chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE m_topology = 
  chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE::kUNDEFINED; 

  Vector<VkVertexInputAttributeDescription> m_vertexInputAttributes;
  Vector<VkVertexInputBindingDescription> m_vertexInputBindings;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  Vector<VkDescriptorSet> m_descriptorSets;

};
} // namespace chEngineSDK
