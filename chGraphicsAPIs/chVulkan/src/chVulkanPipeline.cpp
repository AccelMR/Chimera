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

#include "chVertexLayout.h"
#include "chVulkanDescriptorSetLayout.h"
#include "chVulkanPipelineLayout.h"
#include "chVulkanShader.h"
#include "chVulkanRenderPass.h"

namespace chEngineSDK {
/*
*/
VulkanPipeline::VulkanPipeline(VkDevice device, const PipelineCreateInfo& createInfo)
    : m_device(device) {

  Vector<VkDescriptorSetLayout> descriptorSetLayouts;
  for (const auto& layout : createInfo.setLayouts) {
    auto vulkanLayout = std::static_pointer_cast<VulkanDescriptorSetLayout>(layout);
    descriptorSetLayouts.push_back(vulkanLayout->getHandle());
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .setLayoutCount = static_cast<uint32>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data(),
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr,
  };
  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

  Vector<VkPipelineShaderStageCreateInfo> shaderStages(createInfo.shaders.size());
  Vector<String> entryPointCopies(createInfo.shaders.size());

  SIZE_T index = 0;
  for (const auto& [stage, shader] : createInfo.shaders) {
    auto vulkanShader = std::static_pointer_cast<VulkanShader>(shader);
    CH_ASSERT(vulkanShader); // Make sure it exist, can be overload but it's only debug

    VkShaderStageFlagBits vkStage;
    switch (stage) {
      case ShaderStage::Vertex: vkStage = VK_SHADER_STAGE_VERTEX_BIT; break;
      case ShaderStage::Fragment: vkStage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
      case ShaderStage::Compute: vkStage = VK_SHADER_STAGE_COMPUTE_BIT; break;
      case ShaderStage::Geometry: vkStage = VK_SHADER_STAGE_GEOMETRY_BIT; break;
      default: continue;
    }
    entryPointCopies[index] = vulkanShader->getEntryPoint();
    shaderStages[index] = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = vkStage,
      .module = vulkanShader->getHandle(),
      .pName = entryPointCopies[index].c_str(),
      .pSpecializationInfo = nullptr
    };
    index++;
  }

  Vector<VkVertexInputBindingDescription> bindingDescriptions;
  Vector<VkVertexInputAttributeDescription> attributeDescriptions;

  const auto &layout = createInfo.vertexLayout;

  for (uint32 i = 0; i < layout.getBindingCount(); i++) {
    VkVertexInputBindingDescription bindingDesc = {
      .binding = i,
      .stride = layout.getStride(i),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    bindingDescriptions.push_back(bindingDesc);
  }

  uint32 location = 0;
  for (const auto &attribute : layout.getAttributes()) {
    VkVertexInputAttributeDescription attrDesc = {
      .location = location++,
      .binding = attribute.binding,
      .format = convertVertexFormatToVkFormat(attribute.format),
      .offset = attribute.offset
    };
    attributeDescriptions.push_back(attrDesc);
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = static_cast<uint32>(bindingDescriptions.size()),
    .pVertexBindingDescriptions = bindingDescriptions.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32>(attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data()
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  VkPipelineViewportStateCreateInfo viewportState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = nullptr, // Dinámico
    .scissorCount = 1,
    .pScissors = nullptr   // Dinámico
  };

  VkPipelineRasterizationStateCreateInfo rasterizer{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
  };

  VkPipelineMultisampleStateCreateInfo multisampling{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 0.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlending{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
  };

  Vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .dynamicStateCount = static_cast<uint32>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  auto vulkanRenderPass = std::static_pointer_cast<VulkanRenderPass>(createInfo.renderPass);
  if (!vulkanRenderPass) {
    CH_LOG_ERROR(Vulkan, "VulkanRenderPass is null");
    return;
  }

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = createInfo.depthStencil.enable ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable = createInfo.depthStencil.writeEnable ? VK_TRUE : VK_FALSE;

  switch (createInfo.depthStencil.compareOp) {
    case CompareOp::Less:
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
      break;
    case CompareOp::Equal:
      depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
      break;
    default:
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = static_cast<uint32>(shaderStages.size()),
    .pStages = shaderStages.data(),
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pTessellationState = nullptr,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicState,
    .layout = m_pipelineLayout,
    .renderPass = vulkanRenderPass->getHandle(),
    .subpass = createInfo.subpass,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0
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

/*
*/
SPtr<IPipelineLayout>
VulkanPipeline::getLayout() const {
  return std::make_shared<VulkanPipelineLayout>(m_device, m_pipelineLayout);
}

} // namespace chEngineSDK
