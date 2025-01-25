/************************************************************************/
/**
 * @file chVulkanGraphicsModule.h
 * @author AccelMR
 * @date 2024/10/15
 *   Render hardware implementation specific for Vulkan.
 */
 /************************************************************************/
 #pragma once

  /************************************************************************/
  /*
   * Includes
   */
  /************************************************************************/
#include "chPrerequisitesVulkan.h"

#include <chGraphicsModule.h>

namespace chEngineSDK {
class VulkanSwapChain;

class GraphicsModuleVulkan final : public GraphicsModule
{
 public:
  /*
   *   Default constructor
   */
  GraphicsModuleVulkan();
  GraphicsModuleVulkan(const SPtr<Screen> _screen);
  ~GraphicsModuleVulkan();

  FORCEINLINE const VkSurfaceKHR&
  getSurface() const {
    return m_surface;
    
  }

  FORCEINLINE const VkDevice&
  getDevice() const {
    return m_device;
  }

  FORCEINLINE const 
  VkPhysicalDevice&
  getPhysicalDevice() const {
    return m_physicalDevice;
  }

  FORCEINLINE const VkFormat& 
  getDepthFormat() const {
    return m_depthFormat;
  }

 protected:
  virtual void
  _internalInit(WPtr<Screen> screen) override;
  
  void
  _setupSwapchain(uint32 width, uint32 height);

  void 
  onStartUp() override;

  void 
  onShutDown() override;

  void 
  createFence() override;

  SPtr<GPUCommandBuffer>
  _internalCreateGPUCommandBuffer() override;

  void
  _internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) override;

  void
  _internalPresent(int32, int32) override;

  void
  _internalMoveToNextFrame() override;

  void
  _internalWaitGPU() override;

  SPtr<GPUPipelineState>
  _internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) override;

  void
  _internalOnResize(uint32 width, uint32 height) override;

  SPtr<SwapChain>
  _internalGetSwapChain() override;

  void
  _internalResetSwapChainAllocator() override;

  void
  createSurface();

 private:

  void 
  createInstance();

  void
  createDevice();

  void
  createCommandPool();

  int32 
  getQueueFamilyIndex(VkQueueFlagBits queueFlags);

  bool
  extensionSupported(const String extension);

 public:
  VkInstance m_instance;
  Vector<String> m_supportedInstanceExtensions;
  Vector<const char*> m_enabledInstanceExtensions;
  VkPhysicalDeviceProperties m_deviceProperties{};
  VkPhysicalDeviceFeatures m_deviceFeatures{};
  VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties{};
  Vector<VkQueueFamilyProperties> m_queueFamilyProperties;
  VkFormat m_depthFormat;

  VkPhysicalDevice m_physicalDevice;
  VkSurfaceKHR m_surface;
  VkDevice m_device;
  //SPtr<VulkanSwapChain> m_swapChain;
  VkCommandPool m_commandPool;
  uint32 m_graphicsQueueFamilyIndex = -1;
  uint32 m_computeQueueFamilyIndex = -1;
  uint32 m_transferQueueFamilyIndex = -1;
  uint32 m_frameIndex;

  WPtr<Screen> m_screen;

#if USING(CH_DEBUG_MODE)
  PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
  PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
  PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };
#endif //USING(CH_DEBUG_MODE)

};

GraphicsModuleVulkan&
g_VulkanGraphicsModule();
}