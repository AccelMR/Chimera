/************************************************************************/
/**
 * @file chVulkanAPI.h
 * @author AccelMR
 * @date 2025/04/07
 * @brief 
 * Vulkan API implementation of the graphics API interface.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chIGraphicsAPI.h"

namespace chEngineSDK {

struct VulkanData {
  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  uint32 graphicsQueueFamilyIndex = 0;
};

class VulkanAPI : public IGraphicsAPI {
 public:
  VulkanAPI() : m_vulkanData(chMakeUnique<VulkanData>()) {}
  ~VulkanAPI() override = default;

  void
  initialize(const GraphicsAPIInfo& graphicsAPIInfo) override;

  String
  getAdapterName() const override;

 private:

  void
  createInstance(const GraphicsAPIInfo& graphicsAPIInfo);
  
  void
  pickPhysicalDevice();

  void
  createLogicalDevice();

  void
  setupDebugMessenger(const GraphicsAPIInfo& graphicsAPIInfo);

  bool
  checkValidationLayerSupport() const;

  UnqPtr<VulkanData> m_vulkanData;
};
} // namespace chEngineSDK
