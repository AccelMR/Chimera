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

#include <vulkan/vulkan.h>

#if USING(CH_PLATFORM_WIN32)
#include <vulkan/vulkan_win32.h>
#elif USING(CH_PLATFORM_LINUX)
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_xlib.h>
#endif // USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {
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
    default:
      CH_EXCEPT(VulkanErrorException, StringUtils::format("Unsupported Vulkan format: {0}", format));
  }
  return Format::Unknown;
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

}
#define VK_CHECK(result) chEngineSDK::throwVkResult(result, __FILE__, __LINE__)