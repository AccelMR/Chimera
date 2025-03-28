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

  const VkFormat& 
  getDepthFormat() const {
    return m_depthFormat;
  }

  /*
  */
  FORCEINLINE uint32 
  getGraphicsQueueFamilyIndex() const{
    return m_graphicsQueueFamilyIndex;
  }

 /*
 */
  uint32
  getPresentQueueFamilyIndex() const {
    return m_presentQueueFamilyIndex;
  }

  /*
  */
  VkDescriptorSetLayout
  createDescriptorSetLayout(const BindingGroup& bindingGroup);

  /*
  */
  uint32
  findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);

  VkQueue
  getGraphicsQueue() const {
    return m_graphicsQueue;
  }

  FORCEINLINE VkQueue
  getComputeQueue() const {
    return m_computeQueue;
  }

  FORCEINLINE VkQueue
  getTransferQueue() const {
    return m_transferQueue;
  }

  FORCEINLINE VkQueue
  getPresentQueue() const {
    return m_presentQueue;
  }

 protected:
  virtual void
  _internalInit(WeakPtr<Screen> screen) override;
  
  void
  _setupSwapchain(uint32 width, uint32 height);

  void 
  onStartUp() override;

  void 
  onShutDown() override;
  SPtr<GPUCommandBuffer>
  _internalCreateGPUCommandBuffer() override;

  void
  _internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) override;

  SPtr<GPUPipelineState>
  _internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) override;

  void
  _internalOnResize(uint32 width, uint32 height) override;

  SPtr<SwapChain>
  _internalGetSwapChain() override;

  void
  _internalResetSwapChainAllocator() override;

  SPtr<Fence>
  _internalCreateFence() override;

  void
  _internalSyncGPU(const WeakPtr<Fence> fence, uint64 value) override;

  SPtr<RenderPass>
  _internalCreateRenderPass(const RenderPassDesc& renderPassDesc) override;

  SPtr<Framebuffer>
  _internalCreateFramebuffer(const FramebufferDesc& framebufferDesc) override;

 private:
  void
  createSurface();

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
  VkCommandPool commandPool;
  uint32 m_graphicsQueueFamilyIndex = INVALID_INDEX;
  uint32 m_computeQueueFamilyIndex = INVALID_INDEX;
  uint32 m_transferQueueFamilyIndex = INVALID_INDEX;
  uint32 m_presentQueueFamilyIndex = INVALID_INDEX;
  uint32 m_frameIndex;

  VkQueue m_graphicsQueue;
  VkQueue m_computeQueue;
  VkQueue m_transferQueue;
  VkQueue m_presentQueue;

  WeakPtr<Screen> m_screen;

#if USING(CH_DEBUG_MODE)
  PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
  PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
  PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };
#endif //USING(CH_DEBUG_MODE)

};

GraphicsModuleVulkan&
g_VulkanGraphicsModule();
}