/************************************************************************/
/**
 * @file chVulkanPipeline.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of IPipeline.
 */
/************************************************************************/
#include "chVulkanPipeline.h"

#include "chVertexLayout.h"
#include "chVulkanAPI.h"
#include "chVulkanShader.h"

namespace chEngineSDK {
namespace {
VkCompareOp
toVkCompareOp(CompareOp compareOp)
{
  switch (compareOp) {
  case CompareOp::Never:
    return VK_COMPARE_OP_NEVER;
  case CompareOp::Less:
    return VK_COMPARE_OP_LESS;
  case CompareOp::Equal:
    return VK_COMPARE_OP_EQUAL;
  case CompareOp::LessOrEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case CompareOp::Greater:
    return VK_COMPARE_OP_GREATER;
  case CompareOp::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  case CompareOp::GreaterOrEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case CompareOp::AlwaysOp:
  default:
    return VK_COMPARE_OP_ALWAYS;
  }
}

VkBlendFactor
toVkBlendFactor(BlendFactor factor)
{
  switch (factor) {
  case BlendFactor::Zero:
    return VK_BLEND_FACTOR_ZERO;
  case BlendFactor::One:
    return VK_BLEND_FACTOR_ONE;
  case BlendFactor::SrcColor:
    return VK_BLEND_FACTOR_SRC_COLOR;
  case BlendFactor::OneMinusSrcColor:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case BlendFactor::DstColor:
    return VK_BLEND_FACTOR_DST_COLOR;
  case BlendFactor::OneMinusDstColor:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  case BlendFactor::SrcAlpha:
    return VK_BLEND_FACTOR_SRC_ALPHA;
  case BlendFactor::OneMinusSrcAlpha:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case BlendFactor::DstAlpha:
    return VK_BLEND_FACTOR_DST_ALPHA;
  case BlendFactor::OneMinusDstAlpha:
  default:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  }
}

VkBlendOp
toVkBlendOp(BlendOp op)
{
  switch (op) {
  case BlendOp::Subtract:
    return VK_BLEND_OP_SUBTRACT;
  case BlendOp::ReverseSubtract:
    return VK_BLEND_OP_REVERSE_SUBTRACT;
  case BlendOp::Min:
    return VK_BLEND_OP_MIN;
  case BlendOp::Max:
    return VK_BLEND_OP_MAX;
  case BlendOp::Add:
  default:
    return VK_BLEND_OP_ADD;
  }
}

VkCullModeFlags
toVkCullMode(CullMode cullMode)
{
  switch (cullMode) {
  case CullMode::None:
    return VK_CULL_MODE_NONE;
  case CullMode::Front:
    return VK_CULL_MODE_FRONT_BIT;
  case CullMode::Back:
  default:
    return VK_CULL_MODE_BACK_BIT;
  }
}

VkPrimitiveTopology
toVkTopology(PrimitiveTopology topology)
{
  switch (topology) {
  case PrimitiveTopology::PointList:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case PrimitiveTopology::LineList:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case PrimitiveTopology::LineStrip:
    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case PrimitiveTopology::TriangleStrip:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  case PrimitiveTopology::TriangleList:
  default:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
}

// One per attribute of a vertex layout; more than this is not a real mesh format.
constexpr uint32 kMaxVertexAttributes = 16;
constexpr uint32 kMaxVertexBindings = 8;
} // namespace

/*
 */
VulkanPipeline::VulkanPipeline(VkDevice device,
                               VkPipelineLayout layout,
                               const GraphicsPipelineDesc& desc)
{
  CH_ASSERT(desc.vertexShader && desc.fragmentShader);
  CH_ASSERT(desc.colorAttachmentCount <= GraphicsLimits::MAX_COLOR_ATTACHMENTS);

  const auto* vertexShader = static_cast<const VulkanShader*>(desc.vertexShader.get());
  const auto* fragmentShader = static_cast<const VulkanShader*>(desc.fragmentShader.get());
  const Array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      VkPipelineShaderStageCreateInfo{.sType =
                                          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                      .module = vertexShader->getHandle(),
                                      .pName = vertexShader->getEntryPoint().c_str(),
                                      .pSpecializationInfo = nullptr},
      VkPipelineShaderStageCreateInfo{.sType =
                                          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                      .module = fragmentShader->getHandle(),
                                      .pName = fragmentShader->getEntryPoint().c_str(),
                                      .pSpecializationInfo = nullptr}};

  // HLSL inputs get their locations in declaration order, so attribute i is location i.
  const VertexLayout& vertexLayout = desc.vertexLayout;
  const auto& attributes = vertexLayout.getAttributes();
  CH_ASSERT(attributes.size() <= kMaxVertexAttributes);
  CH_ASSERT(vertexLayout.getBindingCount() <= kMaxVertexBindings);

  Array<VkVertexInputBindingDescription, kMaxVertexBindings> bindings{};
  for (uint32 i = 0; i < vertexLayout.getBindingCount(); ++i) {
    bindings[i] = {.binding = i,
                   .stride = vertexLayout.getStride(i),
                   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  }

  Array<VkVertexInputAttributeDescription, kMaxVertexAttributes> vertexAttributes{};
  for (uint32 i = 0; i < attributes.size(); ++i) {
    vertexAttributes[i] = {.location = i,
                           .binding = attributes[i].binding,
                           .format = convertVertexFormatToVkFormat(attributes[i].format),
                           .offset = attributes[i].offset};
  }

  const VkPipelineVertexInputStateCreateInfo vertexInput{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = vertexLayout.getBindingCount(),
      .pVertexBindingDescriptions = bindings.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32>(attributes.size()),
      .pVertexAttributeDescriptions = vertexAttributes.data()};

  const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = toVkTopology(desc.topology),
      .primitiveRestartEnable = VK_FALSE};

  const VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr};

  const bool hasDepthBias =
      desc.raster.depthBiasConstant != 0.0f || desc.raster.depthBiasSlope != 0.0f;
  // The viewport flips Y so the engine's Direct3D clip space works, which keeps the
  // winding: the front face is the one the description asks for.
  const VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = desc.raster.polygonMode == PolygonMode::Line ? VK_POLYGON_MODE_LINE
                                                                  : VK_POLYGON_MODE_FILL,
      .cullMode = toVkCullMode(desc.raster.cullMode),
      .frontFace = desc.raster.frontFace == FrontFace::Clockwise
                       ? VK_FRONT_FACE_CLOCKWISE
                       : VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = hasDepthBias ? VK_TRUE : VK_FALSE,
      .depthBiasConstantFactor = desc.raster.depthBiasConstant,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = desc.raster.depthBiasSlope,
      .lineWidth = 1.0f};

  const VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = chSampleCountToVkSampleCount(desc.samples),
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE};

  const bool hasDepth = desc.depthFormat != Format::Unknown;
  const VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthTestEnable = hasDepth && desc.depth.testEnable ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = hasDepth && desc.depth.writeEnable ? VK_TRUE : VK_FALSE,
      .depthCompareOp = toVkCompareOp(desc.depth.compareOp),
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f};

  Array<VkPipelineColorBlendAttachmentState, GraphicsLimits::MAX_COLOR_ATTACHMENTS>
      blendAttachments{};
  Array<VkFormat, GraphicsLimits::MAX_COLOR_ATTACHMENTS> colorFormats{};
  for (uint32 i = 0; i < desc.colorAttachmentCount; ++i) {
    const BlendAttachmentState& blend = desc.blendStates[i];
    blendAttachments[i] = {
        .blendEnable = blend.enable ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = toVkBlendFactor(blend.srcColorFactor),
        .dstColorBlendFactor = toVkBlendFactor(blend.dstColorFactor),
        .colorBlendOp = toVkBlendOp(blend.colorOp),
        .srcAlphaBlendFactor = toVkBlendFactor(blend.srcAlphaFactor),
        .dstAlphaBlendFactor = toVkBlendFactor(blend.dstAlphaFactor),
        .alphaBlendOp = toVkBlendOp(blend.alphaOp),
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
    colorFormats[i] = chFormatToVkFormat(desc.colorFormats[i]);
  }

  const VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = desc.colorAttachmentCount,
      .pAttachments = blendAttachments.data(),
      .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

  const Array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .dynamicStateCount = static_cast<uint32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()};

  const VkPipelineRenderingCreateInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = desc.colorAttachmentCount,
      .pColorAttachmentFormats = colorFormats.data(),
      .depthAttachmentFormat = hasDepth ? chFormatToVkFormat(desc.depthFormat)
                                        : VK_FORMAT_UNDEFINED,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};

  const VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &renderingInfo,
      .flags = 0,
      .stageCount = static_cast<uint32>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pTessellationState = nullptr,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = layout,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0};

  VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                     &m_pipeline));
}

/*
 */
VulkanPipeline::~VulkanPipeline()
{
  g_vulkanAPI().getDeletionQueue().enqueue(VK_OBJECT_TYPE_PIPELINE, m_pipeline);
}

} // namespace chEngineSDK
