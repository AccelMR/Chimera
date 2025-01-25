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

#include "chFormats.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"
#include "chVulkanShader.h"

namespace chEngineSDK {
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
}

/*
*/
void
VulkanGPUPipelineState::_init(const chGPUDesc::PipelineStateDesc& desc) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();
  const VkSurfaceKHR& surface = VulkanAPI.getSurface();

  // Crear Pipeline Layout
  Vector<VkPipelineShaderStageCreateInfo> shaderStages;
  auto addShaderStage = [&](const SPtr<VulkanShader> shader, 
                            VkShaderStageFlagBits stage) {
    if (shader) {
      VkPipelineShaderStageCreateInfo shaderStageInfo{};
      shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStageInfo.stage = stage;
      shaderStageInfo.module = shader->m_vertexShader;
      shaderStageInfo.pName = shader->getName().c_str();
      shaderStages.push_back(shaderStageInfo);
    }
  };

  auto vertexShader = std::reinterpret_pointer_cast<VulkanShader>(desc.VS);
  addShaderStage(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
  auto pixelShader = std::reinterpret_pointer_cast<VulkanShader>(desc.PS);
  addShaderStage(pixelShader, VK_SHADER_STAGE_FRAGMENT_BIT);
  auto computeShader = std::reinterpret_pointer_cast<VulkanShader>(desc.CS);
  addShaderStage(computeShader, VK_SHADER_STAGE_COMPUTE_BIT);
  auto meshShader = std::reinterpret_pointer_cast<VulkanShader>(desc.MS);
  addShaderStage(meshShader, VK_SHADER_STAGE_MESH_BIT_NV);

  // Create VkPipelineVertexInputStateCreateInfo

  auto createVertexInputAtrributeDescription = [](uint32 binding,
                                                  uint32 location,
                                                  VkFormat format,
                                                  uint32 offset) 
                                                  -> VkVertexInputAttributeDescription {
    VkVertexInputAttributeDescription vInputAttribDescription {};
    vInputAttribDescription.binding = binding;
    vInputAttribDescription.location = location;
    vInputAttribDescription.format = format;
    vInputAttribDescription.offset = offset;
    return vInputAttribDescription;
  };

  uint32 bindingStride = 0;
  Vector<VkVertexInputAttributeDescription> vertexInputAttributes;
  for (auto& binding : desc.vertexBufferBindingsDesc) {
    bindingStride += chFormatUtils::getFormatSize(binding.format);
    vertexInputAttributes.push_back(
     createVertexInputAtrributeDescription(0, binding.slot,
                                           VulkanTranslator::get(binding.format),
                                           binding.byteStride));
  }

  // Vertex bindings an attributes based on ImGui vertex definition
  //std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
  //	vks::initializers::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
  //};

  VkVertexInputBindingDescription binding{};
  binding.binding = 0; //TODO: not sure about this. We only have one binding
  binding.stride = bindingStride;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  //vertexInputInfo.pNext = nullptr;
  //vertexInputInfo.flags = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 
    static_cast<uint32>(desc.vertexBufferBindingsDesc.size());
  vertexInputInfo.pVertexBindingDescriptions = nullptr;

  vertexInputInfo.vertexAttributeDescriptionCount = 
    static_cast<uint32>(desc.vertexBufferBindingsDesc.size());
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

  // Create VkPipelineInputAssemblyStateCreateInfo
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

  inputAssembly.topology = VulkanTranslator::get(desc.topology);
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Create VkPipelineViewportStateCreateInfo
  VkViewport viewport{};
  viewport.x = desc.viewport.minPoint.x;
  viewport.y = desc.viewport.minPoint.y;
  viewport.width = desc.viewport.maxPoint.x;
  viewport.height = desc.viewport.maxPoint.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  // TODO: Implement scissor
  const Vector2 viewPortSize = desc.viewport.getSize();
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {static_cast<uint32>(viewPortSize.x), static_cast<uint32>(viewPortSize.y)};

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Create VkPipelineRasterizationStateCreateInfo
  const RasterizerStateDesc& rasterizerDesc = desc.rasterizerStateDesc;
  VkPipelineRasterizationStateCreateInfo rasterizerState{};
  rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  //rasterizerState.pNext = nullptr; // No additional data to pass
  //rasterizerState.flags = 0; // No flags to set
  rasterizerState.depthClampEnable = VK_FALSE; //TODO: Implement depth clamp
  rasterizerState.rasterizerDiscardEnable = VK_FALSE; //TODO: Implement rasterizer discard
  rasterizerState.polygonMode = VulkanTranslator::get(rasterizerDesc.fillMode);
  rasterizerState.cullMode = VulkanTranslator::get(rasterizerDesc.cullMode);
  rasterizerState.frontFace = rasterizerDesc.frontCounterClockWise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
  rasterizerState.depthBiasEnable = rasterizerDesc.depthBias != 0 ? VK_TRUE : VK_FALSE;
  rasterizerState.depthBiasConstantFactor = static_cast<float>(rasterizerDesc.depthBias);
  rasterizerState.depthBiasClamp = rasterizerDesc.depthBiasClamp;
  rasterizerState.depthBiasSlopeFactor = rasterizerDesc.slopeScaledDepthBias;
  rasterizerState.lineWidth = rasterizerDesc.lineWidth;

  // Create Color Blending
  VkPipelineColorBlendStateCreateInfo colorBlending = VulkanTranslator::get(desc.blendState);
  
  // Create Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = VulkanTranslator::get(desc.sampleDesc);
  
  // Create Depth and Stencil
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = desc.depthStencilStateDesc.depthEnable;
  depthStencil.depthWriteEnable = desc.depthStencilStateDesc.depthWriteMask == DEPTH_WRITE_MASK::kALL;
  //depthStencil.depthCompareOp = VulkanTranslator::get(desc.depthStencilStateDesc.depthFunc);
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE; //TODO: Implement stencil

  // Create Pipeline Layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0; // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
  
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create Vulkan pipeline layout!");
  }

  // Crear Graphics Pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  //pipelineInfo.pNext = nullptr;
  //pipelineInfo.flags = 0;
  pipelineInfo.stageCount = static_cast<uint32>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizerState;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = m_pipelineLayout;
  //pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create Vulkan graphics pipeline!");
  }

  // Cleanup: Destruir los Shader Modules después de usarlos
  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(device, stage.module, nullptr);
  }

}

} // namespace chEngineSDK
