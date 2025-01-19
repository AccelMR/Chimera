/************************************************************************/
/**
 * @file chVulkanTranslator.cpp
 * @author AccelMR
 * @date 2024/10/25
 *    Vulkan-specific implementation to translate Chimera descriptors to Vulkan structures.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanTranslator.h"
#include "chFormats.h"

namespace chEngineSDK {

VkBlendFactor
VulkanTranslator::get(const BLEND& blend) {
  switch (blend) {
    case BLEND::kBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
    case BLEND::kBLEND_ONE: return VK_BLEND_FACTOR_ONE;
    case BLEND::kBLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
    case BLEND::kBLEND_INV_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BLEND::kBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
    case BLEND::kBLEND_INV_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BLEND::kBLEND_DEST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
    case BLEND::kBLEND_INV_DEST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BLEND::kBLEND_DEST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
    case BLEND::kBLEND_INV_DEST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BLEND::kBLEND_SRC_ALPHA_SAT: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    default: return VK_BLEND_FACTOR_ZERO;
  }
}

VkCompareOp
VulkanTranslator::get(const COMPARISON_FUNC& compareFunc) {
  switch (compareFunc) {
    case COMPARISON_FUNC::kNEVER: return VK_COMPARE_OP_NEVER;
    case COMPARISON_FUNC::kLESS: return VK_COMPARE_OP_LESS;
    case COMPARISON_FUNC::kEQUAL: return VK_COMPARE_OP_EQUAL;
    case COMPARISON_FUNC::kLESS_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case COMPARISON_FUNC::kGREATER: return VK_COMPARE_OP_GREATER;
    case COMPARISON_FUNC::kNOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
    case COMPARISON_FUNC::kGREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case COMPARISON_FUNC::kALWAYS: return VK_COMPARE_OP_ALWAYS;
    default: return VK_COMPARE_OP_ALWAYS;
  }
}

/*
*/
VkStencilOp
VulkanTranslator::get(const STENCIL_OP& stencilOp) {
  switch (stencilOp) {
    case STENCIL_OP::kKEEP:         return VK_STENCIL_OP_KEEP;
    case STENCIL_OP::kZERO:         return VK_STENCIL_OP_ZERO;
    case STENCIL_OP::kREPLACE:      return VK_STENCIL_OP_REPLACE;
    case STENCIL_OP::kINCR_SAT:     return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case STENCIL_OP::kDECR_SAT:     return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case STENCIL_OP::kINVERT:       return VK_STENCIL_OP_INVERT;
    case STENCIL_OP::kINCR:         return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case STENCIL_OP::kDECR:         return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:                        return VK_STENCIL_OP_KEEP;
  }
}

/*
*/
VkStencilOpState
VulkanTranslator::get(const DepthStencilOpDesc& stencilOp) {
  VkStencilOpState stencil{};
  stencil.failOp = VulkanTranslator::get(stencilOp.stencilFailOp);
  stencil.passOp = VulkanTranslator::get(stencilOp.stencilPassOp);
  stencil.depthFailOp = VulkanTranslator::get(stencilOp.stencilDepthFailOp);
  stencil.compareOp = VulkanTranslator::get(stencilOp.stencilFunc);
  //stencil.compareMask = stencilOp.stencilReadMask;
  //stencil.writeMask = stencilOp.stencilWriteMask;
  stencil.reference = 1;  // Se puede parametrizar si es necesario.

  return stencil;
}

VkBlendOp
VulkanTranslator::get(const BLEND_OP& blendOp) {
  switch (blendOp) {
    case BLEND_OP::kBLEND_OP_ADD: return VK_BLEND_OP_ADD;
    case BLEND_OP::kBLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
    case BLEND_OP::kBLEND_OP_REV_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BLEND_OP::kBLEND_OP_MIN: return VK_BLEND_OP_MIN;
    case BLEND_OP::kBLEND_OP_MAX: return VK_BLEND_OP_MAX;
    default: return VK_BLEND_OP_ADD;
  }
}

VkPipelineDepthStencilStateCreateInfo
VulkanTranslator::get(const DepthStencilStateDesc& depthStencilDesc) {
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = depthStencilDesc.depthEnable ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable = depthStencilDesc.depthWriteMask == DEPTH_WRITE_MASK::kALL ? VK_TRUE : VK_FALSE;
  depthStencil.depthCompareOp = VulkanTranslator::get(depthStencilDesc.DepthFunc);
  depthStencil.depthBoundsTestEnable = VK_FALSE;  // No parece haber un campo equivalente en `DepthStencilStateDesc`.
  depthStencil.minDepthBounds = 0.0f;  // Opcional
  depthStencil.maxDepthBounds = 1.0f;  // Opcional
  depthStencil.stencilTestEnable = depthStencilDesc.stencilEnable ? VK_TRUE : VK_FALSE;
  depthStencil.front = VulkanTranslator::get(depthStencilDesc.FrontFace);
  depthStencil.back = VulkanTranslator::get(depthStencilDesc.BackFace);

  return depthStencil;
}

/*
 * Translates VertexBufferBindingDesc to VkVertexInputAttributeDescription.
 */
VkVertexInputAttributeDescription
VulkanTranslator::get(const VertexBufferBindingDesc& chDesc, uint32_t location) {
  VkVertexInputAttributeDescription attribDesc{};
  attribDesc.location = location;
  attribDesc.binding = chDesc.slot;
  attribDesc.format = VulkanTranslator::get(chDesc.format);
  attribDesc.offset = chDesc.byteStride;
  return attribDesc;
}

/*
 * Translates VertexBufferBindingDesc to VkVertexInputBindingDescription.
 */
VkVertexInputBindingDescription
VulkanTranslator::getBinding(const VertexBufferBindingDesc& chDesc) {
  VkVertexInputBindingDescription bindingDesc{};
  bindingDesc.binding = chDesc.slot;
  bindingDesc.stride = chDesc.byteStride;
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Assumes per-vertex data.
  return bindingDesc;
}

/*
 * Translates FILL_MODE to VkPolygonMode.
 */
VkPolygonMode
VulkanTranslator::get(const FILL_MODE& fillMode) {
  switch (fillMode) {
    case FILL_MODE::kWIREFRAME:
      return VK_POLYGON_MODE_LINE;
    case FILL_MODE::kSOLID:
      return VK_POLYGON_MODE_FILL;
    default:
      CH_EXCEPT(InternalErrorException, "Invalid FILL_MODE provided for Vulkan translation!");
  }
}

/*
 * Translates CULL_MODE to VkCullModeFlags.
 */
VkCullModeFlags
VulkanTranslator::get(const CULL_MODE& cullMode) {
  switch (cullMode) {
    case CULL_MODE::kNONE:
      return VK_CULL_MODE_NONE;
    case CULL_MODE::kFRONT:
      return VK_CULL_MODE_FRONT_BIT;
    case CULL_MODE::kBACK:
      return VK_CULL_MODE_BACK_BIT;
    default:
      CH_EXCEPT(InternalErrorException, "Invalid CULL_MODE provided for Vulkan translation!");
  }
}

/*
 * Translates RasterizerStateDesc to VkPipelineRasterizationStateCreateInfo.
 */
VkPipelineRasterizationStateCreateInfo
VulkanTranslator::get(const RasterizerStateDesc& rastDesc) {
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VulkanTranslator::get(rastDesc.fillMode);
  rasterizer.cullMode = VulkanTranslator::get(rastDesc.cullMode);
  rasterizer.frontFace = rastDesc.frontCounterClockWise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = rastDesc.depthBias != 0;
  rasterizer.depthBiasConstantFactor = static_cast<float>(rastDesc.depthBias);
  rasterizer.depthBiasClamp = rastDesc.depthBiasClamp;
  rasterizer.depthBiasSlopeFactor = rastDesc.slopeScaledDepthBias;
  rasterizer.lineWidth = 1.0f;
  return rasterizer;
}

/*
 * Translates PRIMITIVE_TOPOLOGY_TYPE to VkPipelineInputAssemblyStateCreateInfo.
 */
VkPrimitiveTopology
VulkanTranslator::get(const PRIMITIVE_TOPOLOGY_TYPE& topology) {
  switch (topology) {
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_POINT:
      return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_LINE:
      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_PATCH:
      return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    default:
      CH_EXCEPT(InternalErrorException, "Invalid PRIMITIVE_TOPOLOGY_TYPE provided for Vulkan translation!");
  }
}

/*
 * Translates PRIMITIVE_TOPOLOGY_TYPE to VkPrimitiveTopology.
 */
VkPrimitiveTopology
VulkanTranslator::getTopologyType(const PRIMITIVE_TOPOLOGY_TYPE& topology) {
  switch (topology) {
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_POINT:
      return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_LINE:
      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_PATCH:
      return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    default:
      CH_EXCEPT(InternalErrorException, "Invalid PRIMITIVE_TOPOLOGY_TYPE provided for Vulkan translation!");
  }
}

/*
 * Translates FORMAT to VkFormat.
 */
VkFormat
VulkanTranslator::get(const FORMAT& format) {
  switch (format) {
    case FORMAT::kR32G32B32A32_FLOAT:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    case FORMAT::kR8G8B8A8_UNORM:
      return VK_FORMAT_R8G8B8A8_UNORM;
    // Agrega más traducciones aquí según tus descriptores
    default:
      CH_EXCEPT(InternalErrorException, "Invalid FORMAT provided for Vulkan translation!");
  }
}

/*
 * Translates BindingGroup to VkDescriptorSetLayout.
 */
VkDescriptorSetLayout
VulkanTranslator::get(const BindingGroup& bindingGroup, const VkDevice& device) {
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  for (const auto& buffer : bindingGroup.buffers) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = buffer.slot;
    layoutBinding.descriptorType = (buffer.type == BufferBindingDesc::TYPE::kUNIFORM)
                                       ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                       : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    bindings.push_back(layoutBinding);
  }

  for (const auto& texture : bindingGroup.textures) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = texture.slot;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    bindings.push_back(layoutBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VkDescriptorSetLayout descriptorSetLayout;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create descriptor set layout!");
  }

  return descriptorSetLayout;
}

/*
 * Combines multiple DescriptorSetLayouts into a VkPipelineLayout.
 */
VkPipelineLayout
VulkanTranslator::get(const Vector<BindingGroup>& bindingGroups, const VkDevice& device) {
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

  for (const auto& group : bindingGroups) {
    descriptorSetLayouts.push_back(VulkanTranslator::get(group, device));
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

  VkPipelineLayout pipelineLayout;
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create pipeline layout!");
  }

  return pipelineLayout;
}

VkPipelineColorBlendStateCreateInfo
VulkanTranslator::get(const BlendStateDesc& blendState) {
  VkPipelineColorBlendStateCreateInfo blendStateInfo{};
  blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendStateInfo.logicOpEnable = VK_FALSE; // DX12 usa logicOp, Vulkan no suele necesitarlo para blending normal
  blendStateInfo.logicOp = VK_LOGIC_OP_COPY; // Solo si necesitas lógica de píxeles

  blendStateInfo.attachmentCount = static_cast<uint32_t>(blendState.renderTargetBlendDesc.size());

  Vector<VkPipelineColorBlendAttachmentState> attachments(blendState.renderTargetBlendDesc.size());

  for (uint32_t i = 0; i < blendState.renderTargetBlendDesc.size(); ++i) {
    const auto& rtBlendDesc = blendState.renderTargetBlendDesc[i];

    VkPipelineColorBlendAttachmentState& blendAttachment = attachments[i];
    blendAttachment.blendEnable = (rtBlendDesc.srcBlend != BLEND::kBLEND_ONE || 
                                   rtBlendDesc.destBlend != BLEND::kBLEND_ZERO) ? VK_TRUE : VK_FALSE;

    blendAttachment.srcColorBlendFactor = VulkanTranslator::get(rtBlendDesc.srcBlend);
    blendAttachment.dstColorBlendFactor = VulkanTranslator::get(rtBlendDesc.destBlend);
    blendAttachment.colorBlendOp = VulkanTranslator::get(rtBlendDesc.blendOP);

    blendAttachment.srcAlphaBlendFactor = VulkanTranslator::get(rtBlendDesc.srcsBlendAlpha);
    blendAttachment.dstAlphaBlendFactor = VulkanTranslator::get(rtBlendDesc.destBlendAlpha);
    blendAttachment.alphaBlendOp = VulkanTranslator::get(rtBlendDesc.blenOpAlpha);

    blendAttachment.colorWriteMask = rtBlendDesc.renderTargetWritemask;
  }

  blendStateInfo.pAttachments = attachments.data();
  blendStateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());

  return blendStateInfo;
}

VkSampleCountFlagBits
VulkanTranslator::get(uint32 count) {
  switch (count) {
    case 1: return VK_SAMPLE_COUNT_1_BIT;
    case 2: return VK_SAMPLE_COUNT_2_BIT;
    case 4: return VK_SAMPLE_COUNT_4_BIT;
    case 8: return VK_SAMPLE_COUNT_8_BIT;
    case 16: return VK_SAMPLE_COUNT_16_BIT;
    case 32: return VK_SAMPLE_COUNT_32_BIT;
    case 64: return VK_SAMPLE_COUNT_64_BIT;
    default: return VK_SAMPLE_COUNT_1_BIT;
  }
}


VkPipelineMultisampleStateCreateInfo
VulkanTranslator::get(const SampleDesc& sampleDesc) {
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VulkanTranslator::get(sampleDesc.count);
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  return multisampling;
}


} // namespace chEngineSDK
