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

#if defined(CH_VULKAN_IMGUI)
#define CH_VK_IMGUI IN_USE
#else
#define CH_VK_IMGUI IN_NOT_USE
#endif // defined(CH_VULKAN_IMGUI)

#if USING(CH_VK_IMGUI)
#include <imgui_impl_vulkan.h>

  #if USING(CH_DISPLAY_SDL3)
    #include <imgui_impl_sdl3.h>
  #endif // USING(CH_DISPLAY_SDL3

#endif // USING(CH_VK_IMGUI)


#if USING(CH_DISPLAY_SDL3)
# include <SDL3/SDL_vulkan.h>
#endif // USING(CH_DISPLAY_SDL3)

#if USING(CH_PLATFORM_WIN32)
# include <vulkan/vulkan_win32.h>
#elif USING(CH_PLATFORM_LINUX)
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

FORCEINLINE static void
throwVkResult(VkResult result, const char* file, int line) {
  if (result != VK_SUCCESS) {
    CH_EXCEPT(VulkanErrorException, chString::format("Vulkan error: {0} at {1}:{2}", result, file, line));
  }
}

FORCEINLINE static VkImageType
chTextureTypeToVkImageType(TextureType type) {
  switch (type) {
    case TextureType::Texture1D:
      return VK_IMAGE_TYPE_1D;
    case TextureType::Texture2D:
      return VK_IMAGE_TYPE_2D;
    case TextureType::Texture3D:
      return VK_IMAGE_TYPE_3D;
    case TextureType::TextureCube:
    default:
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported Vulkan image type: {0}", static_cast<uint32>(type)));
  }
}

FORCEINLINE static Format
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
    case VK_FORMAT_B8G8R8A8_UNORM:
      return Format::B8G8R8A8_UNORM;
    default:
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported Vulkan format: {0}", format));
  }
}

FORCEINLINE static VkFormat
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
    case Format::B8G8R8A8_UNORM:
      return VK_FORMAT_B8G8R8A8_UNORM;
    default:
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported Vulkan format: {0}", static_cast<uint32>(format)));
  }
}

FORCEINLINE static TextureViewType
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
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported Vulkan image view type: {0}", viewType));
  }
}

FORCEINLINE static VkImageViewType
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
                chString::format("Unsupported Vulkan image view type: {0}",
                                    static_cast<uint32>(viewType)));
  }
}

FORCEINLINE static VkImageLayout
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

FORCEINLINE static VkPipelineStageFlags
pipelineStageToVkPipelineStage(PipelineStageFlags stageFlags) {
  VkPipelineStageFlags result = 0;

  if (stageFlags & PipelineStage::TopOfPipe) {
    result |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }
  if (stageFlags & PipelineStage::DrawIndirect) {
    result |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  }
  if (stageFlags & PipelineStage::VertexInput) {
    result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  }
  if (stageFlags & PipelineStage::VertexShader) {
    result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  }
  if (stageFlags & PipelineStage::FragmentShader) {
    result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  if (stageFlags & PipelineStage::ColorAttachmentOutput) {
    result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }
  if (stageFlags & PipelineStage::ComputeShader) {
    result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  }
  if (stageFlags & PipelineStage::Transfer) {
    result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  if (stageFlags & PipelineStage::BottomOfPipe) {
    result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  // if (stageFlags & PipelineStage::Host)
  //   result |= VK_PIPELINE_STAGE_HOST_BIT;
  if (stageFlags & PipelineStage::AllGraphics) {
    result |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  }
  if (stageFlags & PipelineStage::AllCommands) {
    result |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  return (result == 0) ?
          static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) :
          result;
}

FORCEINLINE static VkAccessFlags
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

FORCEINLINE static VkFormat
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
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported vertex format: {0}", static_cast<uint32>(format)));
  }
}

FORCEINLINE static VkSampleCountFlagBits
chSampleCountToVkSampleCount(SampleCount sampleCount) {
  switch (sampleCount) {
    case SampleCount::Count1:
      return VK_SAMPLE_COUNT_1_BIT;
    case SampleCount::Count2:
      return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount::Count4:
      return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount::Count8:
      return VK_SAMPLE_COUNT_8_BIT;
    case SampleCount::Count16:
      return VK_SAMPLE_COUNT_16_BIT;
    case SampleCount::Count32:
      return VK_SAMPLE_COUNT_32_BIT;
    case SampleCount::Count64:
      return VK_SAMPLE_COUNT_64_BIT;
    default:
      CH_EXCEPT(VulkanErrorException, chString::format("Unsupported sample count: {0}", static_cast<uint32>(sampleCount)));
  }
}

FORCEINLINE static VkImageUsageFlags
chTextureUsageToVkImageUsage(TextureUsageFlags usage) {
  VkImageUsageFlags result = 0;

  if (usage.isSet(TextureUsage::TransferSrc)) {
    result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  if (usage.isSet(TextureUsage::TransferDst)) {
    result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  if (usage.isSet(TextureUsage::Sampled)) {
    result |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if (usage.isSet(TextureUsage::Storage)) {
    result |= VK_IMAGE_USAGE_STORAGE_BIT;
  }
  if (usage.isSet(TextureUsage::ColorAttachment)) {
    result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  if (usage.isSet(TextureUsage::DepthStencil)) {
    result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  if (usage.isSet(TextureUsage::Transient)) {
    result |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
  }
  if (usage.isSet(TextureUsage::InputAttachment)) {
    result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  return result;
}

} // namespace chEngineSDK
#define VK_CHECK(result) chEngineSDK::throwVkResult(result, __FILE__, __LINE__)
