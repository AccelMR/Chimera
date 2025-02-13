/************************************************************************/
/**
 * @file chVulkanTranslator.h
 * @author AccelMR
 * @date 2024/10/25
 *    Contains hard-coded translations to map Chimera descriptors to Vulkan structures.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"
#include "chGPUResourceDescriptors.h"

namespace chEngineSDK {

using namespace chGPUDesc;

class VulkanTranslator
{
 public:
  /*
  */
  static VkBlendFactor
  get(const BLEND& blend);

  /*
  */
  static VkBlendOp
  get(const BLEND_OP& blendOp);

  /*
  */
  static VkCompareOp
  get(const COMPARISON_FUNC& compareFunc);

  /*
  */
  static VkStencilOp
  get(const STENCIL_OP& stencilOp);

  /*
  */
  static VkLogicOp
  get(const LOGIC_OP& stencilOp);

  /*
  */
  static VkStencilOpState
  get(const DepthStencilOpDesc& stencilOp);

  /*
  */
  static VkPipelineDepthStencilStateCreateInfo
  get(const DepthStencilStateDesc& depthStencilDesc);
  
  /*
   * Translates VertexBufferBindingDesc to VkVertexInputAttributeDescription.
   */
  static VkVertexInputAttributeDescription
  get(const VertexBufferBindingDesc& chDesc, uint32_t location);

  /*
   * Translates VertexBufferBindingDesc to VkVertexInputBindingDescription.
   */
  static VkVertexInputBindingDescription
  getBinding(const VertexBufferBindingDesc& chDesc);

  /*
   * Translates FILL_MODE to VkPolygonMode.
   */
  static VkPolygonMode
  get(const FILL_MODE& fillMode);

  /*
   * Translates CULL_MODE to VkCullModeFlags.
   */
  static VkCullModeFlags
  get(const CULL_MODE& cullMode);

  /*
   * Translates RasterizerStateDesc to VkPipelineRasterizationStateCreateInfo.
   */
  static VkPipelineRasterizationStateCreateInfo
  get(const RasterizerStateDesc& rastDesc);

  /*
   * Translates PRIMITIVE_TOPOLOGY_TYPE to VkPipelineInputAssemblyStateCreateInfo.
   */
  static VkPrimitiveTopology
  get(const PRIMITIVE_TOPOLOGY_TYPE& topology);

  /*
   * Translates PRIMITIVE_TOPOLOGY_TYPE to VkPrimitiveTopology.
   */
  static VkPrimitiveTopology
  getTopologyType(const PRIMITIVE_TOPOLOGY_TYPE& topology);

  /*
   * Translates FORMAT to VkFormat.
   */
  static VkFormat
  get(const FORMAT& format);

  /*
   * Translates VkFormat to FORMAT.
   */
  static FORMAT
  get(VkFormat format);

  /*
   * Combines multiple DescriptorSetLayouts into a VkPipelineLayout.
   */
  static Vector<VkDescriptorSetLayoutBinding>
  get(const BindingGroup& bindingGroup);

  /*
  */
  static VkPipelineColorBlendStateCreateInfo
  get(const BlendStateDesc& blendState, uint32 renderTargetCount);

  /*
  */
  static VkShaderStageFlags
  get(const ShaderStageFlag& stages);

  /*
  /*/
  static VkDescriptorType
  get(const DescriptorBinding::TYPE& bufferType);

 /*
 */
  static VkPresentModeKHR
  get(const SWAPCHAIN_EFFECT& effect);

  /*
  */
  static VkFilter
  get(const FILTER& filter);

  /*
  */
  static VkSamplerAddressMode
  get(const TEXTURE_ADDRESS_MODE& addressMode);

  /*
  */
  static VkSampleCountFlagBits
  get(const SampleCountFlag& sampleCountFlags);

  /*
  */
  static VkAttachmentLoadOp
  get(const AttachmentDesc::LOAD_OP& loadOp);

  /*
  */
  static VkAttachmentStoreOp
  get(const AttachmentDesc::STORE_OP& storeOp);

  static VkAccessFlags
  get(const chGPUDesc::AccessFlag& accessFlag);

  static VkPipelineStageFlags
  getPipelineFlags(const chGPUDesc::AccessFlag& accessFlag);

  static VkImageLayout
  get(const chGPUDesc::ResourceStates& state);

  static VkPipelineStageFlags
  getPipelineStageFlags(const chGPUDesc::ResourceStates& state);

  static VkColorComponentFlags
  get(const chGPUDesc::ColorWriteEnableFlag& colorWrite);
};

} // namespace chEngineSDK
