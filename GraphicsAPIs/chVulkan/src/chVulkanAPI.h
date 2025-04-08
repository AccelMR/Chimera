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

  NODISCARD String
  getAdapterName() const override;

  NODISCARD SPtr<ISwapChain>
  createSwapChain(uint32 width, uint32 height, WeakPtr<DisplaySurface> display, bool vsync = false) override;

  FORCEINLINE VkDevice
  getDevice() const {
    return m_vulkanData->device;
  }

  FORCEINLINE VkPhysicalDevice
  getPhysicalDevice() const {
    return m_vulkanData->physicalDevice;
  }

  FORCEINLINE VkInstance
  getInstance() const {
    return m_vulkanData->instance;
  }

 private:

  void
  createInstance(const GraphicsAPIInfo& graphicsAPIInfo);
  
  NODISCARD bool
  pickPhysicalDevice();

  NODISCARD bool
  isDeviceSuitable(VkPhysicalDevice device) const;

  NODISCARD Optional<uint32>
  findQueueFamily(VkPhysicalDevice device, VkQueueFlags queueFlags) const;

  void
  createLogicalDevice();

  void
  setupDebugMessenger(const GraphicsAPIInfo& graphicsAPIInfo);

  bool
  checkValidationLayerSupport() const;

  UnqPtr<VulkanData> m_vulkanData;
};

CH_CORE_EXPORT VulkanAPI& g_vulkanAPI();

} // namespace chEngineSDK
