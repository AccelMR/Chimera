/*****************************************************************************/
/**
 * @file    chVulkanGPUPipelineState.cpp
 * @author  AccelMR
 * @date    2024/12/11
 * @brief   Vulkan implementation of a GPU pipeline state.
 */
/*****************************************************************************/\

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "chVulkanGPUPipelineState.h"

#include "chCommandParser.h"
#include "chDebug.h"
#include "chFormats.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanRenderPass.h"
#include "chVulkanSampler.h"
#include "chVulkanShader.h"
#include "chVulkanTranslator.h"

namespace chEngineSDK {
using std::reinterpret_pointer_cast;

/*
*/
VulkanGPUPipelineState::~VulkanGPUPipelineState() {
  const VkDevice& device = g_VulkanGraphicsModule().getDevice();
  if (m_pipeline) {
    vkDestroyPipeline(device, m_pipeline, nullptr);
  }

  if (m_pipelineLayout) {
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
  }

  for (auto descriptorSetLayout : m_descriptorSetLayouts) {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  }
}

/*
*/
void
VulkanGPUPipelineState::_init(const chGPUDesc::PipelineStateDesc& desc) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  m_renderPass = reinterpret_pointer_cast<VulkanRenderPass>(desc.renderPass);
  CH_ASSERT(m_renderPass);
  m_subPassIndex = desc.subPassIndex;

  Vector<VkPipelineShaderStageCreateInfo> shaderStages;
  auto addShaderStage = [&](const SPtr<VulkanShader>& shader, VkShaderStageFlagBits stage) {
    if (shader) {
      VkPipelineShaderStageCreateInfo shaderStageInfo{};
      shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStageInfo.stage = stage;
      shaderStageInfo.module = shader->m_shaderModule;
      shaderStageInfo.pName = shader->getEntryPoint().c_str();
      shaderStages.push_back(shaderStageInfo);
    }
  };

  addShaderStage(std::reinterpret_pointer_cast<VulkanShader>(desc.VS), VK_SHADER_STAGE_VERTEX_BIT);
  addShaderStage(std::reinterpret_pointer_cast<VulkanShader>(desc.PS), VK_SHADER_STAGE_FRAGMENT_BIT);
  addShaderStage(std::reinterpret_pointer_cast<VulkanShader>(desc.CS), VK_SHADER_STAGE_COMPUTE_BIT);
  addShaderStage(std::reinterpret_pointer_cast<VulkanShader>(desc.MS), VK_SHADER_STAGE_MESH_BIT_NV);

  // Crear Pipeline Layout con los BindingGroups
  createPipelineLayout(desc.bindingGroups);
  CH_ASSERT(m_pipelineLayout != VK_NULL_HANDLE);

  // Crear Vertex Input State
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  createVertexInputState(desc, vertexInputInfo);

  // Crear Input Assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  createInputAssemblyState(desc, inputAssembly);


  VkPipelineViewportStateCreateInfo viewportInfo{};
  Vector<VkViewport> vkViewports;
  Vector<VkRect2D> vkScissors;
  
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  if (!desc.viewports.empty()) {
      createViewportState(desc, viewportInfo, vkViewports, vkScissors);
  } 
  else {
    CommandParser& commandParser = CommandParser::getInstance();
    const int32 width = std::stoi(commandParser.getParam("Width", "1280"));
    const int32 height = std::stoi(commandParser.getParam("Height", "720"));

    VkViewport defaultViewport{};
    defaultViewport.x = 0.0f;
    defaultViewport.y = 0.0f;
    defaultViewport.width = static_cast<float>(width);
    defaultViewport.height = static_cast<float>(height);
    defaultViewport.minDepth = 0.0f;
    defaultViewport.maxDepth = 1.0f;
    vkViewports.push_back(defaultViewport);

    VkRect2D defaultScissor{};
    defaultScissor.offset = {0, 0};
    defaultScissor.extent = {static_cast<uint32>(width), static_cast<uint32>(height)};
    vkScissors.push_back(defaultScissor);

    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = vkViewports.data();
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = vkScissors.data();
  }

  // Estados dinámicos
  Vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  // Crear Rasterizer State
  VkPipelineRasterizationStateCreateInfo rasterizerState{};
  createRasterizerState(desc, rasterizerState);

  // Crear Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  createMultisamplingState(desc, multisampling);

  // Crear Color Blend
  createColorBlendState(desc.blendState);
  m_blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

  // Crear Depth-Stencil
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  createDepthStencilState(desc, depthStencil);

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = static_cast<uint32>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportInfo; 
  pipelineInfo.pRasterizationState = &rasterizerState;
  pipelineInfo.pColorBlendState = &m_blendStateInfo;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.renderPass = m_renderPass->getRenderPass();
  pipelineInfo.subpass = m_subPassIndex;

  throwIfFailed(vkCreateGraphicsPipelines(device, 
                                          VK_NULL_HANDLE, 
                                          1, 
                                          &pipelineInfo, 
                                          nullptr, 
                                          &m_pipeline));

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(device, stage.module, nullptr);
  }

  m_bindingBufferGroup = desc.bindingGroups;
  createDescriptorPool(desc.bindingGroups);

}

/*
*/
void 
VulkanGPUPipelineState::createPipelineLayout(const Vector<chGPUDesc::BindingGroup>& bindingGroups) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  Vector<VkDescriptorSetLayout> descriptorSetLayouts;

  for (const auto& bindingGroup : bindingGroups) {
    Vector<VkDescriptorSetLayoutBinding> layoutBindings;
    for (const auto& binding : bindingGroup.bindings) {
      VkDescriptorSetLayoutBinding layoutBinding{};
      layoutBinding.binding = binding.slot;
      layoutBinding.descriptorType = VulkanTranslator::get(binding.type);
      layoutBinding.descriptorCount = 1;
      layoutBinding.stageFlags = VulkanTranslator::get(binding.stages);
      layoutBinding.pImmutableSamplers = nullptr;

      layoutBindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    throwIfFailed(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));

    descriptorSetLayouts.push_back(descriptorSetLayout);
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

  throwIfFailed(vkCreatePipelineLayout(device, 
                                       &pipelineLayoutInfo, 
                                       nullptr, 
                                       &m_pipelineLayout));

  m_descriptorSetLayouts = descriptorSetLayouts;
}

/*
*/
void
VulkanGPUPipelineState::createDescriptorPool(const Vector<chGPUDesc::BindingGroup>& bindingGroups) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  Vector<VkDescriptorPoolSize> poolSizes;
  for (const auto& bindingGroup : bindingGroups) {
    for (const auto& binding : bindingGroup.bindings) {
      VkDescriptorPoolSize poolSize{};
      poolSize.type = VulkanTranslator::get(binding.type);
      poolSize.descriptorCount = 1;
      poolSizes.push_back(poolSize);
    }
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;

  throwIfFailed(vkCreateDescriptorPool(device, 
                                       &poolInfo, 
                                       nullptr, 
                                       &m_descriptorPool));

  m_descriptorSets.resize(bindingGroups.size());
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32>(bindingGroups.size());
  allocInfo.pSetLayouts = m_descriptorSetLayouts.data();

  throwIfFailed(vkAllocateDescriptorSets(device, 
                                         &allocInfo, 
                                         m_descriptorSets.data()));
}

/*
*/
void
VulkanGPUPipelineState::createVertexInputState(const chGPUDesc::PipelineStateDesc& desc,
                                               VkPipelineVertexInputStateCreateInfo& vertexInputInfo) {
  m_vertexInputAttributes.clear();
  m_vertexInputBindings.clear();

  uint32 bindingIndex = 0;
  for (const auto& binding : desc.vertexBufferBindingsDesc) {
    VkVertexInputAttributeDescription attribDescription{};
    attribDescription.binding = bindingIndex;
    attribDescription.location = binding.slot;
    attribDescription.format = VulkanTranslator::get(binding.format);
    attribDescription.offset = binding.byteStride;
    m_vertexInputAttributes.push_back(attribDescription);

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = bindingIndex;
    bindingDesc.stride = chFormatUtils::getFormatSize(binding.format);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    m_vertexInputBindings.push_back(bindingDesc);

    ++bindingIndex;
  }

  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32>(m_vertexInputBindings.size());
  vertexInputInfo.pVertexBindingDescriptions = m_vertexInputBindings.empty() ? nullptr : m_vertexInputBindings.data();
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32>(m_vertexInputAttributes.size());
  vertexInputInfo.pVertexAttributeDescriptions = m_vertexInputAttributes.empty() ? nullptr : m_vertexInputAttributes.data();
}

/*
*/
void
VulkanGPUPipelineState::createInputAssemblyState(const chGPUDesc::PipelineStateDesc& desc,
                                                 VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo) {
  inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VulkanTranslator::get(desc.topology);
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
  m_topology = desc.topology;
}

/*
*/
void
VulkanGPUPipelineState::createViewportState(const chGPUDesc::PipelineStateDesc& desc,
                                            VkPipelineViewportStateCreateInfo& viewportInfo,
                                            Vector<VkViewport>& viewports,
                                            Vector<VkRect2D>& scissors) {
  viewports.clear();
  scissors.clear();

  for (const auto& viewport : desc.viewports) {
    VkViewport vkViewport{};
    vkViewport.x = viewport.minPoint.x;
    vkViewport.y = viewport.minPoint.y;
    vkViewport.width = viewport.getSize().x;
    vkViewport.height = viewport.getSize().y;
    vkViewport.minDepth = 0.0f;
    vkViewport.maxDepth = 1.0f;
    viewports.push_back(vkViewport);

    VkRect2D scissor{};
    scissor.offset = {static_cast<int32>(viewport.minPoint.x), static_cast<int32>(viewport.minPoint.y)};
    scissor.extent = {static_cast<uint32>(viewport.getSize().x), static_cast<uint32>(viewport.getSize().y)};
    scissors.push_back(scissor);
  }

  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.viewportCount = static_cast<uint32>(viewports.size());
  viewportInfo.pViewports = viewports.data();
  viewportInfo.scissorCount = static_cast<uint32>(scissors.size());
  viewportInfo.pScissors = scissors.data();
}

/*
*/
void
VulkanGPUPipelineState::createRasterizerState(const chGPUDesc::PipelineStateDesc& desc,
                                              VkPipelineRasterizationStateCreateInfo& rasterizationInfo) {
  const auto& rasterizerState = desc.rasterizerStateDesc;

  rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationInfo.depthClampEnable = VK_FALSE;
  rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizationInfo.polygonMode = VulkanTranslator::get(rasterizerState.fillMode);
  rasterizationInfo.cullMode = VulkanTranslator::get(rasterizerState.cullMode);
  rasterizationInfo.frontFace = rasterizerState.frontCounterClockWise ? 
                                VK_FRONT_FACE_COUNTER_CLOCKWISE : 
                                VK_FRONT_FACE_CLOCKWISE;
  rasterizationInfo.depthBiasEnable = rasterizerState.depthBias != 0 ? VK_TRUE : VK_FALSE;
  rasterizationInfo.depthBiasConstantFactor = rasterizerState.depthBias;
  rasterizationInfo.depthBiasClamp = rasterizerState.depthBiasClamp;
  rasterizationInfo.depthBiasSlopeFactor = rasterizerState.slopeScaledDepthBias;
  rasterizationInfo.lineWidth = rasterizerState.lineWidth;
}

/*
*/
void
VulkanGPUPipelineState::createMultisamplingState(const chGPUDesc::PipelineStateDesc& desc,
                                                 VkPipelineMultisampleStateCreateInfo& multisampling) {
  const auto& sampleDesc = desc.sampleDesc;
  const auto& sampleMask = desc.sampleDesc.sampleMask;

  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = sampleDesc.sampleShadingEnable ? VK_TRUE : VK_FALSE;
  multisampling.rasterizationSamples = VulkanTranslator::get(sampleDesc.count);
  multisampling.minSampleShading = sampleDesc.minSampleShading;
  multisampling.pSampleMask = sampleMask.empty() ? nullptr : sampleMask.data();
  multisampling.alphaToCoverageEnable = sampleDesc.alphaToCoverageEnable ? VK_TRUE : VK_FALSE;
  multisampling.alphaToOneEnable = sampleDesc.alphaToOneEnable ? VK_TRUE : VK_FALSE;
}

/*
*/
void
VulkanGPUPipelineState::createDepthStencilState(const chGPUDesc::PipelineStateDesc& desc,
                                                VkPipelineDepthStencilStateCreateInfo& depthStencilInfo) {
  const auto& depthStencilState = desc.depthStencilStateDesc;
  
  depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  depthStencilInfo.depthTestEnable = depthStencilState.depthEnable ? VK_TRUE : VK_FALSE;
  depthStencilInfo.depthWriteEnable = depthStencilState.depthWriteMask == DEPTH_WRITE_MASK::kALL ? VK_TRUE : VK_FALSE;
  depthStencilInfo.depthCompareOp = VulkanTranslator::get(depthStencilState.DepthFunc);

  depthStencilInfo.stencilTestEnable = depthStencilState.stencilEnable ? VK_TRUE : VK_FALSE;

  if (depthStencilState.stencilEnable) {
    depthStencilInfo.front = VulkanTranslator::get(depthStencilState.FrontFace);
    depthStencilInfo.back = VulkanTranslator::get(depthStencilState.BackFace);
  }
  else {
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};
  }

  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.minDepthBounds = 0.0f;
  depthStencilInfo.maxDepthBounds = 1.0f;
}

/*
*/
void 
VulkanGPUPipelineState::createColorBlendState(const chGPUDesc::BlendStateDesc& blendStateDesc) {
  m_blendStateInfo = {};
  m_blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  m_blendStateInfo.logicOpEnable = VK_FALSE;

  const chGPUDesc::SubpassDesc& subpass = m_renderPass->getSubpassDesc(m_subPassIndex);
  size_t numColorAttachments = subpass.colorAttachments.size();

  m_blendAttachments.resize(numColorAttachments);

  for (size_t i = 0; i < numColorAttachments; ++i) {
    auto& blendState = m_blendAttachments[i];

    if (i < blendStateDesc.renderTargetBlendDesc.size()) {
      const auto& rtBlend = blendStateDesc.renderTargetBlendDesc[i];

      blendState.blendEnable = (rtBlend.srcBlend != BLEND::kBLEND_ONE || 
                                rtBlend.destBlend != BLEND::kBLEND_ZERO);

      blendState.srcColorBlendFactor = VulkanTranslator::get(rtBlend.srcBlend);
      blendState.dstColorBlendFactor = VulkanTranslator::get(rtBlend.destBlend);
      blendState.colorBlendOp = VulkanTranslator::get(rtBlend.blendOP);

      blendState.srcAlphaBlendFactor = VulkanTranslator::get(rtBlend.srcsBlendAlpha);
      blendState.dstAlphaBlendFactor = VulkanTranslator::get(rtBlend.destBlendAlpha);
      blendState.alphaBlendOp = VulkanTranslator::get(rtBlend.blenOpAlpha);

      blendState.colorWriteMask = VulkanTranslator::get(rtBlend.renderTargetWritemask);
    } 
    else {
      blendState.blendEnable = VK_FALSE;
      blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      blendState.colorBlendOp = VK_BLEND_OP_ADD;
      blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      blendState.alphaBlendOp = VK_BLEND_OP_ADD;
      blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                VK_COLOR_COMPONENT_G_BIT | 
                                VK_COLOR_COMPONENT_B_BIT | 
                                VK_COLOR_COMPONENT_A_BIT;
    }
  }

  m_blendStateInfo.attachmentCount = static_cast<uint32>(m_blendAttachments.size());
  m_blendStateInfo.pAttachments = m_blendAttachments.data();
}

} // namespace chEngineSDK
