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

  VkQueue presentQueue = VK_NULL_HANDLE;
  uint32 presentQueueFamilyIndex = 0;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
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
  createSwapChain(uint32 width, uint32 height, bool vsync = false) override;

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

  FORCEINLINE uint32
  getGraphicsQueueFamilyIndex() const {
    return m_vulkanData->graphicsQueueFamilyIndex;
  }

  FORCEINLINE VkQueue
  getGraphicsQueue() const {
    return m_vulkanData->graphicsQueue;
  }

  FORCEINLINE VkQueue
  getPresentQueue() const {
    return m_vulkanData->presentQueue;
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

  void
  createSurface(WeakPtr<DisplaySurface> display);

  UnqPtr<VulkanData> m_vulkanData;
};

CH_CORE_EXPORT VulkanAPI& g_vulkanAPI();

} // namespace chEngineSDK
