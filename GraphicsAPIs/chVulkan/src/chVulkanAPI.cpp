/************************************************************************/
/**
 * @file chVulkanAPI.cpp
 * @author AccelMR
 * @date 2025/04/07
 * @brief 
 * Vulkan API implementation of the graphics API interface.
 */
/************************************************************************/
#include "chVulkanAPI.h"

#include "chDebug.h"
#include "chVulkanSwapChain.h"

namespace chVulkanAPIHelpers{
const std::vector<const char*> VALIDATION_LAYERS = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VKAPI_ATTR VkBool32 VKAPI_CALL 
debugUtilsMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                          const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                          void *pUserData) {
  using namespace chEngineSDK;
  // Select prefix depending on flags passed to the callback
  String prefix;

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
  {
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[32m" + prefix + "\033[0m";
#endif //USING(CH_PLATFORM_WIN32)
    prefix = "VERBOSE: ";
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    prefix = "INFO: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[36m" + prefix + "\033[0m";
#endif //USING(CH_PLATFORM_WIN32)
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    prefix = "WARNING: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[33m" + prefix + "\033[0m";
#endif //USING(CH_PLATFORM_WIN32)
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    prefix = "ERROR: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[31m" + prefix + "\033[0m";
#endif //USING(CH_PLATFORM_WIN32)
  }

  // Display message to default output (console/logcat)
  chEngineSDK::StringStream debugMessage;
  if (pCallbackData->pMessageIdName)
  {
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << 
      pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
  }
  else
  {
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "] : " << 
      pCallbackData->pMessage;
  }

#if defined(__ANDROID__)
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    LOGE("%s", debugMessage.str().c_str());
  }
  else
  {
    LOGD("%s", debugMessage.str().c_str());
  }
#else
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    CH_LOG_ERROR(debugMessage.str());
  
  }
  else
  {
    CH_LOG_DEBUG(debugMessage.str());
  }
  fflush(stdout);
#endif

  // The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
  // We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
  // If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT
  return VK_FALSE;
}
} // namespace chVulkanAPIHelpers
using namespace chVulkanAPIHelpers;

namespace chEngineSDK {

/*
*/
void
VulkanAPI::initialize(const GraphicsAPIInfo& graphicsAPIInfo) {
  CH_LOG_DEBUG("Initializing Vulkan API");

  // Initialize Vulkan instance, physical device, and logical device.
  createInstance( graphicsAPIInfo);

  if (graphicsAPIInfo.enableValidationLayer) {
    setupDebugMessenger(graphicsAPIInfo);
  }

  pickPhysicalDevice();

  createLogicalDevice();

  CH_LOG_DEBUG("Vulkan API initialized successfully");
  CH_LOG_DEBUG("Using Adapter : " + getAdapterName());
}

/*
*/
String
VulkanAPI::getAdapterName() const {
  if (m_vulkanData->physicalDevice == VK_NULL_HANDLE) {
    return "No physical device selected";
  }

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(m_vulkanData->physicalDevice, &deviceProperties);

  return deviceProperties.deviceName;
}

/*
*/
SPtr<ISwapChain>
VulkanAPI::createSwapChain(uint32 width, uint32 height, WeakPtr<DisplaySurface> display, bool vsync) {
  SPtr<VulkanSwapChain> swapChain = chMakeShared<VulkanSwapChain>();
  swapChain->createSurface(m_vulkanData->physicalDevice, display);
  swapChain->createSwapChain(width, height, vsync);
  return swapChain;
}

/*
*/
void
VulkanAPI::createInstance(const GraphicsAPIInfo& graphicsAPIInfo) {
  CH_LOG_DEBUG("Creating Vulkan instance");

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "chEngine Application";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "chEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  Vector<const char*> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      #if USING(CH_PLATFORM_WIN32)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      #elif USING(CH_PLATFORM_LINUX)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME
      #endif

  };

  if (graphicsAPIInfo.enableValidationLayer) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (graphicsAPIInfo.enableValidationLayer) {
      if (!checkValidationLayerSupport()) {
          throw std::runtime_error("Validation layers requested but not available");
      }

    createInfo.enabledLayerCount = static_cast<uint32>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugUtilsMessageCallback; // we can add a callback function here
    debugCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
  }

  VK_CHECK (vkCreateInstance(&createInfo, nullptr, &m_vulkanData->instance));
}

/*
*/
void
VulkanAPI::pickPhysicalDevice() {
  uint32 deviceCount = 0;
  vkEnumeratePhysicalDevices(m_vulkanData->instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    CH_EXCEPT(VulkanErrorException, "Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_vulkanData->instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      m_vulkanData->physicalDevice = device;
      break;
    }
  }

  if (m_vulkanData->physicalDevice == VK_NULL_HANDLE) {
    m_vulkanData->physicalDevice = devices[0];
  }


  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(m_vulkanData->physicalDevice, &deviceProperties);

  VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(m_vulkanData->physicalDevice, &deviceMemoryProperties);

  VkDeviceSize totalMemory = 0;
  for (uint32 i = 0; i < deviceMemoryProperties.memoryHeapCount; i++) {
    if (deviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
      totalMemory += deviceMemoryProperties.memoryHeaps[i].size;
    }
  }

  CH_LOG_INFO(StringUtils::format("Adapter descriptor: [{0}]", deviceProperties.deviceName));
  CH_LOG_INFO(StringUtils::format("GPU Vendor ID:  [{0}]", deviceProperties.vendorID));
  CH_LOG_INFO(StringUtils::format("GPU Device ID:  [{0}]", deviceProperties.deviceID));
  CH_LOG_INFO(StringUtils::format("target_link_librariesTotal GPU Memory: [{0} MB]", totalMemory / (1024 * 1024)));

}

/*
*/
void
VulkanAPI::createLogicalDevice() {

  VkQueueFlags queueFlags = VK_QUEUE_GRAPHICS_BIT;
  uint32 queueFamilyCount = 0;

  if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanData->physicalDevice, &queueFamilyCount, nullptr);
    Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanData->physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32 i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        m_vulkanData->graphicsQueueFamilyIndex = i;
        break;
      }
    }
  }
 
  // Queue creation info, for now we are using only one queue
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = m_vulkanData->graphicsQueueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = static_cast<uint32>(DEVICE_EXTENSIONS.size());
  createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

  VK_CHECK (vkCreateDevice(m_vulkanData->physicalDevice, &createInfo, nullptr, &m_vulkanData->device));

  vkGetDeviceQueue(m_vulkanData->device, 0, 0, &m_vulkanData->graphicsQueue);
}

/*
*/
void
VulkanAPI::setupDebugMessenger(const GraphicsAPIInfo& graphicsAPIInfo) {
  if (!graphicsAPIInfo.enableValidationLayer) return;

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      m_vulkanData->instance, "vkCreateDebugUtilsMessengerEXT");

  if (func == nullptr) {
      CH_EXCEPT(VulkanErrorException, "Failed to load debug messenger extension");
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugUtilsMessageCallback; // we can add a callback function here
  createInfo.pUserData = nullptr;

  if (func(m_vulkanData->instance, &createInfo, nullptr, &m_vulkanData->debugMessenger) != VK_SUCCESS) {
    CH_EXCEPT(VulkanErrorException, "Failed to set up debug messenger");
  }
}

/*
*/
bool
VulkanAPI::checkValidationLayerSupport() const {
  uint32 layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  Vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : VALIDATION_LAYERS) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}


/*
*/
VulkanAPI& g_vulkanAPI() {
  return reinterpret_cast<VulkanAPI&>(IGraphicsAPI::instance());
}
} // namespace chEngineSDK