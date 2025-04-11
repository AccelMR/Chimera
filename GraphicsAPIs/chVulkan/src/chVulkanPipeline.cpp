/************************************************************************/
/**
 * @file chVulkanPipeline.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan pipeline implementation.
 * This file contains the implementation of the pipeline interface
 * for Vulkan.
 */
/************************************************************************/

#include "chVulkanPipeline.h"
#include "chVulkanShader.h"
#include "chVulkanRenderPass.h"

namespace chEngineSDK {
/*
*/
VulkanPipeline::VulkanPipeline(VkDevice device, const PipelineCreateInfo& createInfo)
    : m_device(device) {

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };

  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

  Vector<VkPipelineShaderStageCreateInfo> shaderStages;

  for (const auto& [stage, shader] : createInfo.shaders) {
    auto vulkanShader = std::static_pointer_cast<VulkanShader>(shader);

    VkShaderStageFlagBits vkStage;
    switch (stage) {
      case ShaderStage::Vertex: vkStage = VK_SHADER_STAGE_VERTEX_BIT; break;
      case ShaderStage::Fragment: vkStage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
      case ShaderStage::Compute: vkStage = VK_SHADER_STAGE_COMPUTE_BIT; break;
      case ShaderStage::Geometry: vkStage = VK_SHADER_STAGE_GEOMETRY_BIT; break;
      default: continue;
    }
    VkPipelineShaderStageCreateInfo stageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = vkStage,
      .module = vulkanShader->getHandle(),
      .pName = vulkanShader->getEntryPoint().c_str(),
    };

    shaderStages.push_back(stageInfo);
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  VkPipelineViewportStateCreateInfo viewportState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = nullptr, // Dinámico
    .scissorCount = 1,
    .pScissors = nullptr   // Dinámico
  };

  VkPipelineRasterizationStateCreateInfo rasterizer{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  VkPipelineMultisampleStateCreateInfo multisampling{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlending{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment
  };

  Vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  auto vulkanRenderPass = std::static_pointer_cast<VulkanRenderPass>(createInfo.renderPass);

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = static_cast<uint32>(shaderStages.size()),
    .pStages = shaderStages.data(),
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = nullptr, // Por ahora sin depth/stencil
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicState,
    .layout = m_pipelineLayout,
    .renderPass = vulkanRenderPass->getHandle(),
    .subpass = createInfo.subpass
  };

  VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
}

/*
*/
VulkanPipeline::~VulkanPipeline() {
  if (m_pipeline) {
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
  }
  if (m_pipelineLayout) {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }
}

} // namespace chEngineSDK