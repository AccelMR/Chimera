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
#include "chVulkanSampler.h"
#include "chVulkanShader.h"

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
  const VkSurfaceKHR& surface = VulkanAPI.getSurface();

  // Crear Pipeline Layout
  Vector<VkPipelineShaderStageCreateInfo> shaderStages;
  auto addShaderStage = [&](const SPtr<VulkanShader> shader, 
                            VkShaderStageFlagBits stage) {
    if (shader) {
      VkPipelineShaderStageCreateInfo shaderStageInfo{};
      shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStageInfo.stage  = stage;
      shaderStageInfo.module = shader->m_shaderModule;
      shaderStageInfo.pName  = shader->getEntryPoint().c_str();
      shaderStages.push_back(shaderStageInfo);
    }
  };

  auto vertexShader = reinterpret_pointer_cast<VulkanShader>(desc.VS);
  addShaderStage(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
  auto pixelShader = reinterpret_pointer_cast<VulkanShader>(desc.PS);
  addShaderStage(pixelShader, VK_SHADER_STAGE_FRAGMENT_BIT);
  auto computeShader = reinterpret_pointer_cast<VulkanShader>(desc.CS);
  addShaderStage(computeShader, VK_SHADER_STAGE_COMPUTE_BIT);
  auto meshShader = reinterpret_pointer_cast<VulkanShader>(desc.MS);
  addShaderStage(meshShader, VK_SHADER_STAGE_MESH_BIT_NV);

  uint32 bindingStride = 0;
  Vector<VkVertexInputAttributeDescription> vertexInputAttributes;

  for (auto& binding : desc.vertexBufferBindingsDesc) {
    bindingStride += chFormatUtils::getFormatSize(binding.format);

    VkVertexInputAttributeDescription attribDescription {};
    attribDescription.binding = 0;
    attribDescription.location = binding.slot;
    attribDescription.format = VulkanTranslator::get(binding.format);
    attribDescription.offset = binding.byteStride;
    vertexInputAttributes.push_back(attribDescription);
  }

  VkVertexInputBindingDescription binding{};
  binding.binding = 0; //TODO: not sure about this. We only have one binding
  binding.stride = bindingStride;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 
    static_cast<uint32>(desc.vertexBufferBindingsDesc.size());
  vertexInputInfo.pVertexBindingDescriptions = &binding;

  vertexInputInfo.vertexAttributeDescriptionCount = 
    static_cast<uint32>(desc.vertexBufferBindingsDesc.size());
  vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

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
  VkPipelineColorBlendStateCreateInfo colorBlending = 
    VulkanTranslator::get(desc.blendState, desc.numRenderTextures);
  
  // Create Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = desc.sampleDesc.sampleShadingEnable ? VK_TRUE : VK_FALSE;
  multisampling.rasterizationSamples = VulkanTranslator::get(desc.sampleDesc.count);
  multisampling.minSampleShading = desc.sampleDesc.minSampleShading;
  const Vector<uint32>& sampleMask = desc.sampleDesc.sampleMask;
  multisampling.pSampleMask = sampleMask.empty() ? nullptr : sampleMask.data();
  multisampling.alphaToCoverageEnable = desc.sampleDesc.alphaToCoverageEnable ? 
                                        VK_TRUE : 
                                        VK_FALSE;
  multisampling.alphaToOneEnable = desc.sampleDesc.alphaToOneEnable ? VK_TRUE : VK_FALSE;

  // Create Depth and Stencil
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = desc.depthStencilStateDesc.depthEnable;
  depthStencil.depthWriteEnable = desc.depthStencilStateDesc.depthWriteMask == DEPTH_WRITE_MASK::kALL;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;
  depthStencil.stencilTestEnable = VK_FALSE;

  createPipelineLayout(desc.bindingGroups);
  CH_ASSERT(m_pipelineLayout != VK_NULL_HANDLE);
  createRenderPass(desc.renderPassDesc);
  CH_ASSERT(m_renderPass != VK_NULL_HANDLE);

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
  pipelineInfo.renderPass = m_renderPass;
  pipelineInfo.subpass = 0;

  throwIfFailed(vkCreateGraphicsPipelines(device, 
                                          VK_NULL_HANDLE, 
                                          1, 
                                          &pipelineInfo, 
                                          nullptr, 
                                          &m_pipeline));

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(device, stage.module, nullptr);
  }
}

/*
*/
void
VulkanGPUPipelineState::createRenderPass(const RenderPassDesc& desc) {
  Vector<VkAttachmentDescription> attachmentDescriptions;
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  for (const auto &colorAttachmentFormat : desc.colorAttachments) {
    if (colorAttachmentFormat != FORMAT::kUNKNOWN)
    {
      VkAttachmentDescription colorAttachment{};
      colorAttachment.format = VulkanTranslator::get(colorAttachmentFormat);
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      attachmentDescriptions.push_back(colorAttachment);
    }
  }

  VkAttachmentReference depthReference{};
  if (desc.depthStencilAttachment != FORMAT::kUNKNOWN) {
    depthReference.attachment = static_cast<uint32>(attachmentDescriptions.size());
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VulkanTranslator::get(desc.depthStencilAttachment);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachmentDescriptions.push_back(depthAttachment);
  }

  Vector<VkAttachmentReference> colorReferences;
  for (uint32 attachmentIndex = 0; 
       attachmentIndex < desc.colorAttachments.size(); 
       ++attachmentIndex)  {
    if (desc.colorAttachments[attachmentIndex] != FORMAT::kUNKNOWN) {
      colorReferences.push_back({attachmentIndex,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    }
  }

  // Configurar Subpass
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32>(colorReferences.size());
  subpass.pColorAttachments = colorReferences.data();
  subpass.pDepthStencilAttachment = (desc.depthStencilAttachment != FORMAT::kUNKNOWN)
                                        ? &depthReference
                                        : nullptr;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32>(attachmentDescriptions.size());
  renderPassInfo.pAttachments = attachmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 0;
  renderPassInfo.pDependencies = nullptr;

  throwIfFailed(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass));
}

/*
*/
void 
VulkanGPUPipelineState::createPipelineLayout(const chGPUDesc::BindingGroup& bindingGroup) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  // Crear descriptor set layout bindings
  Vector<VkDescriptorSetLayoutBinding> descriptorBindings;

  for (const auto& textureBinding : bindingGroup.textures) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = textureBinding.slot;
    binding.descriptorType = VulkanTranslator::get(textureBinding.type);
    binding.descriptorCount = static_cast<uint32>(textureBinding.textures.size());
    binding.stageFlags = VulkanTranslator::get(textureBinding.stages);

    if (!textureBinding.samplers.empty()) {
      Vector<VkSampler> immutableSamplers(textureBinding.samplers.size());
      for (const auto& sampler : textureBinding.samplers) {
        SPtr<VulkanSampler> vulkanSampler = std::static_pointer_cast<VulkanSampler>(sampler);
        immutableSamplers.push_back(vulkanSampler->getSampler());
      }
      binding.pImmutableSamplers = immutableSamplers.data();
    } 
    else {
      binding.pImmutableSamplers = nullptr;
    }

    descriptorBindings.push_back(binding);
  }

  for (const auto& bufferBinding : bindingGroup.buffers) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bufferBinding.slot;
    binding.descriptorType = VulkanTranslator::get(bufferBinding.type);
    binding.descriptorCount = 1;
    binding.stageFlags = VulkanTranslator::get(bufferBinding.stages);
    binding.pImmutableSamplers = nullptr;

    descriptorBindings.push_back(binding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32>(descriptorBindings.size());
  layoutInfo.pBindings = descriptorBindings.data();

  VkDescriptorSetLayout descriptorSetLayout;
  throwIfFailed(vkCreateDescriptorSetLayout(device,
                                            &layoutInfo,
                                            nullptr,
                                            &descriptorSetLayout));

  m_descriptorSetLayouts.push_back(descriptorSetLayout);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(m_descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  throwIfFailed(vkCreatePipelineLayout(device, 
                                       &pipelineLayoutInfo,
                                       nullptr,
                                       &m_pipelineLayout));
}

} // namespace chEngineSDK
