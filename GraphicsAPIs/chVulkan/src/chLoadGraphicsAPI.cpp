
#include "chVulkanAPI.h"
#include "chLogger.h"

using namespace chEngineSDK;

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_INFO(Vulkan, "Loading Vulkan Plugin");

  IGraphicsAPI::startUp<VulkanAPI>();
  
  CH_LOG_INFO(Vulkan, "Vulkan plugin loaded.");
}


namespace chEngineSDK
{
  CH_LOG_DEFINE_CATEGORY_SHARED(Vulkan, All);
} // namespace chEngineSDK