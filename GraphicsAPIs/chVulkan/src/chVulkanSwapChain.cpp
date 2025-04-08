/************************************************************************/
/**
 * @file chVulkanSwapChain.cpp
 * @author AccelMR
 * @date 2025/04/07
 * @details 
 * SwapChain implementation for Vulkan.
 * This class is used to create and manage the swap chain.
 * It is used to create the swap chain, and to present the swap chain.
 * It is used by the graphics API to create the swap chain.
 */
/************************************************************************/
#include "chVulkanSwapChain.h"

#include "chDebug.h"
#include "chDisplaySurface.h"
#include "chVulkanAPI.h"


#if USING(CH_PLATFORM_WIN32)
#elif USING(CH_PLATFORM_LINUX)
#include "chXCBGlobals.h"
#endif // USING(CH_PLATFORM_WIN32)


namespace chEngineSDK {
    /*
*/
VulkanSwapChain::~VulkanSwapChain() {
  cleanUp();
}

/*
*/
void
VulkanSwapChain::createSwapChain(uint32 width, uint32 height, bool vsync) {
  // Implementation of swap chain creation goes here
  // This is a placeholder for the actual implementation
  CH_LOG_DEBUG("Creating Vulkan swap chain with size: " + std::to_string(width) + "x" + std::to_string(height));
}

/*
*/
void
VulkanSwapChain::present() {
  // Implementation of presenting the swap chain goes here
  // This is a placeholder for the actual implementation
  CH_LOG_DEBUG("Presenting Vulkan swap chain");
}

/*
*/
void
VulkanSwapChain::cleanUp() {
  VkDevice device = g_vulkanAPI().getDevice();
  VkInstance instance = g_vulkanAPI().getInstance();

  if (m_swapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
  }

  if (m_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanSwapChain::resize(uint32 width, uint32 height) {
  // Implementation of resizing the swap chain goes here
  // This is a placeholder for the actual implementation
  CH_LOG_DEBUG("Resizing Vulkan swap chain to: " + std::to_string(width) + "x" + std::to_string(height));
}

/*
*/
void*
VulkanSwapChain::getNativeHandle() const {
  return (void*)m_surface;
}


/*
*/
void
VulkanSwapChain::createSurface(VkPhysicalDevice physicalDevice, WeakPtr<DisplaySurface> display) {
#if USING(CH_PLATFORM_WIN32)

  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
  surfaceCreateInfo.hwnd = (HWND)platformWindow;
  VK_CHECK (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &m_surface));

#elif USING(CH_PLATFORM_LINUX)

  CH_LOG_DEBUG("Creating Vulkan surface for Linux");

  if (display.expired()) {
    CH_EXCEPT(InternalErrorException, "DisplaySurface is expired");
  }
  SPtr<DisplaySurface> displayPtr = display.lock();

  VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.connection = XCBGlobals::getXCBConnection();
  surfaceCreateInfo.window = displayPtr->getPlatformHandlerInt();
  VkInstance instance = g_vulkanAPI().getInstance();
  VK_CHECK (vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &m_surface));

#endif // USING(CH_PLATFORM_WIN32)

  CH_ASSERT(m_surface != VK_NULL_HANDLE);

  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  CH_ASSERT(queueFamilyCount >= 1);

  Vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProps.data());

  Vector<VkBool32> supportsPresent(queueFamilyCount);
  for (uint32 i = 0; i < queueFamilyCount; i++) {
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &supportsPresent[i]);
  }

  uint32 graphicsQueueFamilyIndex = UINT32_MAX;
  uint32 presentQueueFamilyIndex = UINT32_MAX;
  for (uint32 i = 0; i < queueFamilyCount; i++) {
    if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {

      if (graphicsQueueFamilyIndex == UINT32_MAX) {
        graphicsQueueFamilyIndex = i;
      }

      if (supportsPresent[i] == VK_TRUE) {
        graphicsQueueFamilyIndex = i;
        presentQueueFamilyIndex = i;
        break;
      }
    }
  }

  if (presentQueueFamilyIndex == UINT32_MAX) {
    for (uint32 i = 0; i < queueFamilyCount; i++) {
      if (supportsPresent[i] == VK_TRUE) {
        presentQueueFamilyIndex = i;
        break;
      }
    }
  }

  if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
    CH_LOG_ERROR("Separate graphics and present queue families");
  }

  if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
    CH_LOG_ERROR("Separate graphics and present queue families");
  }

  m_queueNodeIndex = graphicsQueueFamilyIndex;

  uint32 formatCount = 0;
  VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                                                 m_surface, 
                                                 &formatCount, 
                                                 nullptr));
  CH_ASSERT(formatCount > 0);

  Vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                                                 m_surface, 
                                                 &formatCount, 
                                                 surfaceFormats.data()));

  VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
  Vector<VkFormat> preferredImageFormats = {
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM, 
    VK_FORMAT_A8B8G8R8_UNORM_PACK32 
  };

  for (auto& availableFormat: surfaceFormats) {
    auto availableFormatIt = std::find(preferredImageFormats.begin(),
                                       preferredImageFormats.end(),
                                       availableFormat.format);
    if (availableFormatIt != preferredImageFormats.end()) {
      selectedFormat = availableFormat;
      break;
    }
  }

  m_colorFormat = selectedFormat.format;
  m_colorSpace = selectedFormat.colorSpace;

}
} // namespace chEngineSDK