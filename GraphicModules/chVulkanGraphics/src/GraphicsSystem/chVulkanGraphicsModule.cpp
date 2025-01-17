/************************************************************************/
/**
 * @file chVulkanGraphicsModule.cpp
 * @author AccelMR
 * @date 2024/10/15
 *   Render hardware implementation specific for Vulkan.
 */
 /************************************************************************/

  /************************************************************************/
  /*
   * Includes
   */
  /************************************************************************/
#include "chVulkanGraphicsModule.h"

#include "chDebug.h"
#include "chScreen.h"
#include "chStringUtils.h"

#include "chVulkanGPUBuffer.h"
#include "chVulkanGPUPipelineState.h"

#if USING(CH_PLATFORM_LINUX)
#include <SDL3/SDL_vulkan.h>
#endif //USING(CH_PLATFORM_LINUX)

namespace chEngineSDK {
  
/*
*/
GraphicsModuleVulkan::GraphicsModuleVulkan()
{}

/*
*/
GraphicsModuleVulkan::~GraphicsModuleVulkan() {}

/*
*/
void
GraphicsModuleVulkan::_internalInit(WPtr<Screen> screen) {
  SPtr<Screen> pScreen = screen.lock();
  CH_ASSERT(pScreen.get());

  // Create Vulkan instance
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Chimera Vulkan Application";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Chimera Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  #if USING(CH_DEBUG_MODE)
  const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
  createInfo.enabledLayerCount = 1;
  createInfo.ppEnabledLayerNames = validationLayers;
  #else
  createInfo.enabledLayerCount = 0;
  #endif

  throwIfFailed(vkCreateInstance(&createInfo, nullptr, &m_instance));

  // create phical device(GPU)
  uint32 deviceCount = 0;
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
  CH_ASSERT(deviceCount > 0);

  Vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

  //Select the first device
  for (const auto& device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      m_physicalDevice = device;
      break;
    }
  }

  CH_ASSERT(m_physicalDevice != VK_NULL_HANDLE);

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);

  LOG_INFO(StringUtils::format("GPU: {0}", deviceProperties.deviceName));
  LOG_INFO(StringUtils::format("API Version: {0}", deviceProperties.apiVersion));

  // Create logical device
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
  Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());


  for (uint32 i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
       m_graphicsQueueFamilyIndex = i;
      break;
    }
  }

  //CH_ASSERT(groupFamily != -1);

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;

  throwIfFailed(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));

  _setupSwapchain(pScreen->getWidth(), pScreen->getHeight());
} 

/*
*/
void
GraphicsModuleVulkan::_setupSwapchain(uint32 width, uint32 height) {
  //TODO: this might be
  SwapChainDesc swapDesc{
    .width = width,
    .height = height,
    .format = FORMAT::kB8G8R8A8_UNORM,
    .frameCount = 2,
  };

  //m_swapChain = ch_shared_ptr_new<VulkanSwapChain>();
  //m_swapChain->init(swapDesc);

  //m_frameIndex = m_swapChain->getCurrentFrameIndex();

  m_commandPools.resize(swapDesc.frameCount);
  for (uint32 i = 0; i < swapDesc.frameCount; i++) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    throwIfFailed(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPools[i]));
  }
}

/*
*/
void 
GraphicsModuleVulkan::onStartUp()
{}

/*
*/
void
GraphicsModuleVulkan::onShutDown() {}

/*
*/
void
GraphicsModuleVulkan::createFence() {}

/*
*/
SPtr<GPUCommandBuffer>
GraphicsModuleVulkan::_internalCreateGPUCommandBuffer() {
  return nullptr;
}

/*
*/
void
GraphicsModuleVulkan::_internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) {}

/*
*/
void
GraphicsModuleVulkan::_internalPresent(int32, int32) {}

/*
*/
void
GraphicsModuleVulkan::_internalMoveToNextFrame() {}

/*
*/
void
GraphicsModuleVulkan::_internalWaitGPU() {}

/*
*/
SPtr<GPUPipelineState>
GraphicsModuleVulkan::_internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) {
  auto ret = ch_shared_ptr_new<VulkanGPUPipelineState>();
  ret->_init(pipelineStateDesc);
  return std::reinterpret_pointer_cast<GPUPipelineState>(ret);
}

/*
*/
void
GraphicsModuleVulkan::_internalOnResize(uint32 width, uint32 height) {}

/*
*/
SPtr<SwapChain>
GraphicsModuleVulkan::_internalGetSwapChain() {
  return nullptr;
}

/*
*/
void
GraphicsModuleVulkan::_internalResetSwapChainAllocator() {}

/*
*/
void
GraphicsModuleVulkan::createSurface()
{
  // if(SDL_Window* sdlWindow = m_screen->getPlatformHandler()) {
  //   CH_ASSERT(SDL_Vulkan_CreateSurface(sdlWindow, m_instance, &m_surface) == true);
  // }
}

GraphicsModuleVulkan&
g_VulkanGraphicsModule() {
  return reinterpret_cast<GraphicsModuleVulkan&>(GraphicsModule::instance());
}

}