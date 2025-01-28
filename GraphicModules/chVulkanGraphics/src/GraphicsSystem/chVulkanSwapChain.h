/************************************************************************/
/**
 * @file chVulkanSwapChain.h
 * @author AccelMR
 * @date 2024/10/20
 *  Vulkan specific implementation for chimera engine.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chSwapChain.h"
#include "chFormats.h"

namespace chEngineSDK {
class VulkanSwapChain final: public SwapChain { 
 public:
  friend class VulkanGPUCommandBuffer;
  friend class GraphicsModuleVulkan;

  /*
  *   Default constructor
  */
  VulkanSwapChain() = delete;

  VulkanSwapChain(const VkDevice& device, 
                  const VkSurfaceKHR& surface, 
                  const VkPhysicalDevice& physicalDevice)
    : m_device(device),
      m_surface(surface),
      m_physicalDevice(physicalDevice) {};

  /*
  *   Default destructor
  */
  ~VulkanSwapChain();

  void 
  init(const chGPUDesc::SwapChainDesc & desc);
  void 
  cleanup();

 private:

  void 
  _internalResize(uint32 width, uint32 height) override;

  uint32 
  _internalGetCurrentFrameIndex() const override { return m_currentFrame; }

  SPtr<Fence> 
  _internalGetCurrentFrameFence() const override { return m_frameFences[m_currentFrame]; }

  void 
  _internalPresent(uint32 syncInterval, uint32 flags) override;




 protected:
  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  Vector<VkImage> m_swapChainImages;
  Vector<VkImageView> m_swapChainImageViews;
  uint32 m_currentFrame = 0;
  Vector<SPtr<Fence>> m_frameFences;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  FORMAT m_format = FORMAT::kB8G8R8A8_UNORM;

  VkExtent2D m_swapChainExtent{};
  VkSurfaceFormatKHR m_surfaceFormat{};
};
}