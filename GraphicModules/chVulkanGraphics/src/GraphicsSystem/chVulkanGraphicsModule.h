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

 protected:
  VkInstance m_instance;

  VkPhysicalDevice m_physicalDevice;
  VkSurfaceKHR m_surface;
  VkDevice m_device;
  //SPtr<VulkanSwapChain> m_swapChain;
  Vector<VkCommandPool> m_commandPools;
  uint32 m_graphicsQueueFamilyIndex = -1;
  uint32 m_frameIndex;
};

GraphicsModuleVulkan&
g_VulkanGraphicsModule();
}