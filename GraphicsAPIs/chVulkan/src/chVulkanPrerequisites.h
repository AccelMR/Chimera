/************************************************************************/
/**
 * @file chVulkanPrerequisites.h
 * @author AccelMR
 * @date 2025/04/07
 * @brief
 *  Vulkan API specific prerequisites.
 *  This file includes the most common and required Vulkan objects.
 */
 /************************************************************************/

#pragma once

#include "chPrerequisitesCore.h"
#include "chException.h"
#include "chStringUtils.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chVertexLayout.h"

#include <vulkan/vulkan.h>

#if USING(CH_PLATFORM_WIN32)
#include <vulkan/vulkan_win32.h>
#elif USING(CH_PLATFORM_LINUX)
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_xlib.h>
#endif // USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {
CH_LOG_DECLARE_EXTERN(Vulkan);
class VulkanErrorException : public Exception
{
public:
VulkanErrorException(const String& inDescription,
                         const String& inSource,
                         const char* inFile,
                         long inLine)
    : Exception("VulkanErrorException", inDescription, inSource, inFile, inLine) {}
};

static void 
throwVkResult(VkResult result, const char* file, int line) {
  if (result != VK_SUCCESS) {
    CH_EXCEPT(VulkanErrorException, StringUtils::format("Vulkan error: {0} at {1}:{2}", result, file, line));
  }
}

static Format
vkFormatToChFormat(VkFormat format) {
  switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
    return Format::R8G8B8A8_UNORM;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    return Format::R16G16B16A16_SFLOAT;
    case VK_FORMAT_D32_SFLOAT:
    return Format::D32_SFLOAT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
    return Format::D24_UNORM_S8_UINT;
    case VK_FORMAT_B8G8R8A8_SRGB:
      return Format::B8G8R8A8_SRGB;
    default:
      CH_EXCEPT(VulkanErrorException, StringUtils::format("Unsupported Vulkan format: {0}", format));
  }
  return Format::Unknown;
}

static VkFormat
chFormatToVkFormat(Format format) {
  switch (format) {
    case Format::R8G8B8A8_UNORM:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::R16G16B16A16_SFLOAT:
      return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::D32_SFLOAT:
      return VK_FORMAT_D32_SFLOAT;
    case Format::D24_UNORM_S8_UINT:
    return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::B8G8R8A8_SRGB:
      return VK_FORMAT_B8G8R8A8_SRGB;
    default:
      CH_EXCEPT(VulkanErrorException, StringUtils::format("Unsupported Vulkan format: {0}", static_cast<uint32>(format)));
  }
  return VK_FORMAT_UNDEFINED;
}

static TextureViewType
vkTextureViewTypeToChTextureViewType(VkImageViewType viewType) {
  switch (viewType) {
    case VK_IMAGE_VIEW_TYPE_1D:
      return TextureViewType::View1D;
    case VK_IMAGE_VIEW_TYPE_2D:
      return TextureViewType::View2D;
    case VK_IMAGE_VIEW_TYPE_3D:
      return TextureViewType::View3D;
    case VK_IMAGE_VIEW_TYPE_CUBE:
      return TextureViewType::ViewCube;
    default:
      CH_EXCEPT(VulkanErrorException, StringUtils::format("Unsupported Vulkan image view type: {0}", viewType));
  }
  return TextureViewType::View1D;
}

static VkImageViewType
chTextureViewTypeToVkTextureViewType(TextureViewType viewType) {
  switch (viewType) {
    case TextureViewType::View1D:
      return VK_IMAGE_VIEW_TYPE_1D;
    case TextureViewType::View2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case TextureViewType::View3D:
      return VK_IMAGE_VIEW_TYPE_3D;
    case TextureViewType::ViewCube:
      return VK_IMAGE_VIEW_TYPE_CUBE;
    default:
      CH_EXCEPT(VulkanErrorException, 
                StringUtils::format("Unsupported Vulkan image view type: {0}", 
                                    static_cast<uint32>(viewType)));
  }
  return VK_IMAGE_VIEW_TYPE_1D;
}

static VkImageLayout 
textureLayoutToVkImageLayout(TextureLayout layout) {
  switch (layout) {
    case TextureLayout::Undefined:
      return VK_IMAGE_LAYOUT_UNDEFINED;
    case TextureLayout::General:
      return VK_IMAGE_LAYOUT_GENERAL;
    case TextureLayout::ColorAttachment:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case TextureLayout::DepthStencilAttachment:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case TextureLayout::DepthStencilReadOnly:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case TextureLayout::ShaderReadOnly:
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case TextureLayout::TransferSrc:
      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case TextureLayout::TransferDst:
      return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case TextureLayout::PresentSrc:
      return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
      return VK_IMAGE_LAYOUT_UNDEFINED;
  }
}

static VkPipelineStageFlags 
pipelineStageToVkPipelineStage(PipelineStageFlags stageFlags) {
  VkPipelineStageFlags result = 0;
  
  if (stageFlags & PipelineStage::TopOfPipe)
    result |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  if (stageFlags & PipelineStage::DrawIndirect)
    result |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  if (stageFlags & PipelineStage::VertexInput)
    result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  if (stageFlags & PipelineStage::VertexShader)
    result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  if (stageFlags & PipelineStage::FragmentShader)
    result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  if (stageFlags & PipelineStage::ColorAttachmentOutput)
    result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  if (stageFlags & PipelineStage::ComputeShader)
    result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  if (stageFlags & PipelineStage::Transfer)
    result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  if (stageFlags & PipelineStage::BottomOfPipe)
    result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  // if (stageFlags & PipelineStage::Host)
  //   result |= VK_PIPELINE_STAGE_HOST_BIT;
  if (stageFlags & PipelineStage::AllGraphics)
    result |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  if (stageFlags & PipelineStage::AllCommands)
    result |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  
  return result == 0 ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : result;
}

static VkAccessFlags 
accessFlagsToVkAccessFlags(AccessFlags accessFlags) {
  VkAccessFlags result = 0;
  
  // if (accessFlags & Access::IndirectCommandRead)
  //     result |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
  // if (accessFlags & Access::IndexRead)
  //     result |= VK_ACCESS_INDEX_READ_BIT;
  // if (accessFlags & Access::VertexAttributeRead)
  //     result |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  // if (accessFlags & Access::UniformRead)
  //     result |= VK_ACCESS_UNIFORM_READ_BIT;
  // if (accessFlags & Access::InputAttachmentRead)
  //     result |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
  if (accessFlags.isSet(Access::ShaderRead)) {
    result |= VK_ACCESS_SHADER_READ_BIT;
  }
  if (accessFlags.isSet(Access::ShaderWrite)) {
    result |= VK_ACCESS_SHADER_WRITE_BIT;
  }
  if (accessFlags.isSet(Access::ColorAttachmentRead)) {
    result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  }
  if (accessFlags.isSet(Access::ColorAttachmentWrite)) {
    result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }
  if (accessFlags.isSet(Access::DepthStencilAttachmentRead)) {
    result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }
  if (accessFlags.isSet(Access::DepthStencilAttachmentWrite)) {
    result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }
  if (accessFlags.isSet(Access::TransferRead)) {
    result |= VK_ACCESS_TRANSFER_READ_BIT;
  }
  if (accessFlags.isSet(Access::TransferWrite)) {
    result |= VK_ACCESS_TRANSFER_WRITE_BIT;
  }
  if (accessFlags.isSet(Access::HostRead)) {
    result |= VK_ACCESS_HOST_READ_BIT;
  }
  if (accessFlags.isSet(Access::HostWrite)) {
    result |= VK_ACCESS_HOST_WRITE_BIT;
  }
  if (accessFlags.isSet(Access::MemoryRead)) {
    result |= VK_ACCESS_MEMORY_READ_BIT;
  }
  if (accessFlags.isSet(Access::MemoryWrite)) {
    result |= VK_ACCESS_MEMORY_WRITE_BIT;
  }
  
  return result;
}

static VkFormat
convertVertexFormatToVkFormat(VertexFormat format) {
  switch (format) {
    case VertexFormat::Float:
      return VK_FORMAT_R32_SFLOAT;
    case VertexFormat::Float2:
      return VK_FORMAT_R32G32_SFLOAT;
    case VertexFormat::Float3:
      return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexFormat::Float4:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    case VertexFormat::Int:
      return VK_FORMAT_R32_SINT;
    case VertexFormat::Int2:
      return VK_FORMAT_R32G32_SINT;
    case VertexFormat::Int3:
      return VK_FORMAT_R32G32B32_SINT;
    case VertexFormat::Int4:
      return VK_FORMAT_R32G32B32A32_SINT;
    case VertexFormat::UInt:
      return VK_FORMAT_R32_UINT;
    case VertexFormat::UInt2:
      return VK_FORMAT_R32G32_UINT;
    case VertexFormat::UInt3:
      return VK_FORMAT_R32G32B32_UINT;
    case VertexFormat::UInt4:
      return VK_FORMAT_R32G32B32A32_UINT;
    case VertexFormat::Byte4:
      return VK_FORMAT_R8G8B8A8_SINT;
    case VertexFormat::Byte4Normalized:
      return VK_FORMAT_R8G8B8A8_SNORM;
    case VertexFormat::UByte4:
      return VK_FORMAT_R8G8B8A8_UINT;
    case VertexFormat::UByte4Normalized:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case VertexFormat::Short2:
      return VK_FORMAT_R16G16_SINT;
    case VertexFormat::Short2Normalized:
      return VK_FORMAT_R16G16_SNORM;
    case VertexFormat::Short4:
      return VK_FORMAT_R16G16B16A16_SINT;
    case VertexFormat::Short4Normalized:
      return VK_FORMAT_R16G16B16A16_SNORM;
    default:
      CH_EXCEPT(VulkanErrorException, StringUtils::format("Unsupported vertex format: {0}", static_cast<uint32>(format)));
  }
  return VK_FORMAT_UNDEFINED;
}

} // namespace chEngineSDK
#define VK_CHECK(result) chEngineSDK::throwVkResult(result, __FILE__, __LINE__)