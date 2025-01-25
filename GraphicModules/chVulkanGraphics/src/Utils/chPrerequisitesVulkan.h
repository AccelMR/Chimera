/************************************************************************/
/**
 * @file chPrerequisitesVulkan.h
 * @author AccelMR
 * @date 2024/10/15
 *   Must basic include for every Vulkan file
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include <vulkan/vulkan.h>

namespace chEngineSDK {

  /*
*/
inline void throwIfFailed(VkResult vkResult) {
  if (vkResult != VK_SUCCESS) {
    String message = std::system_category().message(vkResult);
    CH_EXCEPT(InternalErrorException, "HR failed in Vulkan module." + message);
  }
}

}
