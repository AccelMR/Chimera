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

#include <vulkan/vulkan.h>

#if USING(CH_PLATFORM_WIN32)
#include <vulkan/vulkan_win32.h>
#elseif USING(CH_PLATFORM_LINUX)
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

static void throwVkResult(VkResult result, const char* file, int line) {
  if (result != VK_SUCCESS) {
    CH_EXCEPT(VulkanErrorException, StringUtils::format("Vulkan error: {0} at {1}:{2}", result, file, line));
  }
}

}
#define VK_CHECK(result) chEngineSDK::throwVkResult(result, __FILE__, __LINE__)