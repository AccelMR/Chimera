
#include "chVulkanAPI.h"
#include "chLogger.h"

using namespace chEngineSDK;

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_DEBUG(Vulkan, "Loading Vulkan Plugin...");

  IGraphicsAPI::startUp<VulkanAPI>();
}


namespace chEngineSDK
{
  CH_LOG_DEFINE_CATEGORY_SHARED(Vulkan, All);
} // namespace chEngineSDK
