
#include "chVulkanAPI.h"
#include "chDebug.h"

using namespace chEngineSDK;

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_INFO("Loading Vulkan Plugin");

  IGraphicsAPI::startUp<VulkanAPI>();
  
  CH_LOG_INFO("Vulkan plugin loaded.");
}