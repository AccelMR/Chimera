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

#include "chDisplaySurface.h"
#include "chVulkanBuffer.h"
#include "chVulkanCommandBuffer.h"
#include "chVulkanCommandPool.h"
#include "chVulkanCommandQueue.h"
#include "chVulkanFrameBuffer.h"
#include "chVulkanPipeline.h"
#include "chVulkanRenderPass.h"
#include "chVulkanShader.h"
#include "chVulkanSwapChain.h"
#include "chVulkanSynchronization.h"
#include "chVulkanTexture.h"


#if USING(CH_PLATFORM_WIN32)
#elif USING(CH_PLATFORM_LINUX)
#include "chXCBGlobals.h"
#endif // USING(CH_PLATFORM_WIN32)

namespace chVulkanAPIHelpers{
FORCEINLINE constexpr chEngineSDK::Array<const char*, 1> VALIDATION_LAYERS = {
  "VK_LAYER_KHRONOS_validation"
};

FORCEINLINE constexpr chEngineSDK::Array<const char*, 1> DEVICE_EXTENSIONS = {
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
    CH_LOG_ERROR(Vulkan, debugMessage.str());
  
  }
  else
  {
    CH_LOG_DEBUG(Vulkan, debugMessage.str());
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
  CH_LOG_DEBUG(Vulkan, "Initializing Vulkan API");

  // Initialize Vulkan instance, physical device, and logical device.
  createInstance( graphicsAPIInfo);

  
  if (graphicsAPIInfo.enableValidationLayer) {
    setupDebugMessenger(graphicsAPIInfo);
  }
  
  if (!pickPhysicalDevice()){
    CH_EXCEPT(VulkanErrorException, "Failed to pick a physical device");
  }

  createLogicalDevice();

  createSurface(graphicsAPIInfo.weakDisplaySurface);

  CH_LOG_DEBUG(Vulkan, "Vulkan API initialized successfully");
  CH_LOG_DEBUG(Vulkan, "Using Adapter : " + getAdapterName());
}

/*
*/
NODISCARD String
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
NODISCARD SPtr<ISwapChain>
VulkanAPI::createSwapChain(uint32 width, uint32 height, bool vsync) {
  SPtr<VulkanSwapChain> swapChain = 
    chMakeShared<VulkanSwapChain>(m_vulkanData->device,
                                  m_vulkanData->physicalDevice,
                                  m_vulkanData->surface,
                                  m_vulkanData->surfaceFormat,
                                  m_vulkanData->colorSpace,
                                  m_graphicsQueueFamilyIndex,
                                  m_presentQueueFamilyIndex);
  swapChain->create(width, height, vsync);
  return swapChain;
}

/*
*/
NODISCARD SPtr<IBuffer>
VulkanAPI::createBuffer(const BufferCreateInfo& createInfo) {
  return chMakeShared<VulkanBuffer>(m_vulkanData->device, 
                                    m_vulkanData->physicalDevice,
                                    createInfo);
}

/*
*/
NODISCARD SPtr<ITexture>
VulkanAPI::createTexture(const TextureCreateInfo& createInfo) {
  return chMakeShared<VulkanTexture>(m_vulkanData->device,
                                     m_vulkanData->physicalDevice,
                                     createInfo);
}

/*
*/
NODISCARD SPtr<ICommandPool>
VulkanAPI::createCommandPool(QueueType queueType, bool transient) {
  uint32 queueFamilyIndex = 0;
    
  switch (queueType) {
    case QueueType::Graphics:
      queueFamilyIndex = m_graphicsQueueFamilyIndex;
      break;
    case QueueType::Present:
      queueFamilyIndex = m_presentQueueFamilyIndex;
      break;
    default:
      CH_LOG_WARNING(Vulkan, "Unsupported queue type, falling back to graphics queue");
      queueFamilyIndex = m_graphicsQueueFamilyIndex;
  }
  
  return chMakeShared<VulkanCommandPool>(m_vulkanData->device, queueFamilyIndex, transient);
}

/*
*/
NODISCARD SPtr<IFence>
VulkanAPI::createFence(bool signaled) {
  return chMakeShared<VulkanFence>(m_vulkanData->device, signaled);
}

/*
*/
NODISCARD SPtr<ISemaphore>
VulkanAPI::createSemaphore() {
  return chMakeShared<VulkanSemaphore>(m_vulkanData->device);
}

/*
*/
NODISCARD SPtr<IShader>
VulkanAPI::createShader(const ShaderCreateInfo& createInfo) {
  return chMakeShared<VulkanShader>(m_vulkanData->device, createInfo);
}

/*
*/
NODISCARD SPtr<IPipeline>
VulkanAPI::createPipeline(const PipelineCreateInfo& createInfo) {
  return chMakeShared<VulkanPipeline>(m_vulkanData->device, createInfo);
}

/*
*/
NODISCARD SPtr<IRenderPass>
VulkanAPI::createRenderPass(const RenderPassCreateInfo& createInfo) {
  return chMakeShared<VulkanRenderPass>(m_vulkanData->device, createInfo);
}

/*
*/
NODISCARD SPtr<IFrameBuffer>
VulkanAPI::createFrameBuffer(const FrameBufferCreateInfo& createInfo) {
  return chMakeShared<VulkanFrameBuffer>(m_vulkanData->device, createInfo);
}

/*
*/
NODISCARD SPtr<ICommandQueue>
VulkanAPI::getQueue(QueueType queueType) {
  switch (queueType) {
  case QueueType::Graphics:
    return m_graphicsQueue;
  default:
  //TODO: Add more queue types
    CH_EXCEPT(VulkanErrorException, "Invalid queue type");
    return nullptr;
    break;
  }
}

/*
*/
void
VulkanAPI::createInstance(const GraphicsAPIInfo& graphicsAPIInfo) {
  CH_LOG_DEBUG(Vulkan, "Creating Vulkan instance");

  VkApplicationInfo appInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "chEngine Application",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "chEngine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_2
  };

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

  VkInstanceCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo
  };

  createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (graphicsAPIInfo.enableValidationLayer) {
    if (!checkValidationLayerSupport()) {
      CH_EXCEPT (VulkanErrorException, "Validation layers requested but not available");
    }
    
    createInfo.enabledLayerCount = static_cast<uint32>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
  }
  else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
  }

  VK_CHECK (vkCreateInstance(&createInfo, nullptr, &m_vulkanData->instance));
}

/*
*/
NODISCARD bool
VulkanAPI::pickPhysicalDevice() {
  uint32 deviceCount = 0;
  vkEnumeratePhysicalDevices(m_vulkanData->instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    CH_LOG_ERROR(Vulkan, "Failed to find GPUs with Vulkan support");
  }

  Vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_vulkanData->instance, &deviceCount, devices.data());

  std::sort(devices.begin(), devices.end(), [](const VkPhysicalDevice& a, const VkPhysicalDevice& b) {
    VkPhysicalDeviceProperties propertiesA;
    vkGetPhysicalDeviceProperties(a, &propertiesA);
    VkPhysicalDeviceProperties propertiesB;

    int32 scoreA = 0;
    int32 scoreB = 0;

    switch (propertiesA.deviceType) {
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: scoreA = 4; break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: scoreA = 3; break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: scoreA = 2; break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU: scoreA = 1; break;
      default: scoreA = 0;
    }

    switch (propertiesB.deviceType) {
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: scoreB = 4; break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: scoreB = 3; break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: scoreB = 2; break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU: scoreB = 1; break;
      default: scoreB = 0;
    }

    if (scoreA == scoreB) {
      VkPhysicalDeviceMemoryProperties memPropsA, memPropsB;
      vkGetPhysicalDeviceMemoryProperties(a, &memPropsA);
      vkGetPhysicalDeviceMemoryProperties(b, &memPropsB);
      
      VkDeviceSize localMemA = 0, localMemB = 0;
      for (uint32 i = 0; i < memPropsA.memoryHeapCount; i++) {
          if (memPropsA.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
              localMemA += memPropsA.memoryHeaps[i].size;
          }
      }
      
      for (uint32 i = 0; i < memPropsB.memoryHeapCount; i++) {
          if (memPropsB.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
              localMemB += memPropsB.memoryHeaps[i].size;
          }
      }
      
      return localMemA > localMemB;
    }
  
    return scoreA > scoreB;

  });

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
        m_vulkanData->physicalDevice = device;
        break;
    }
  }

  if (m_vulkanData->physicalDevice == VK_NULL_HANDLE) {
      return false;
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

  CH_LOG_INFO(Vulkan, "Adapter descriptor: [{0}]", deviceProperties.deviceName);
  CH_LOG_INFO(Vulkan, "GPU Vendor ID:  [{0}]", deviceProperties.vendorID);
  CH_LOG_INFO(Vulkan, "GPU Device ID:  [{0}]", deviceProperties.deviceID);
  CH_LOG_INFO(Vulkan, "target_link_librariesTotal GPU Memory: [{0} MB]", totalMemory / (1024 * 1024));

  return true;
}

/*
*/
NODISCARD bool
VulkanAPI::isDeviceSuitable(VkPhysicalDevice device) const {
    // Check for queue families
    auto graphicsQueueFamily = findQueueFamily(device, VK_QUEUE_GRAPHICS_BIT);
    if (!graphicsQueueFamily.has_value()) {
        return false;
    }
    
    // Check for extension support
    uint32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    Vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    Set<String> requiredExtensions(
        chVulkanAPIHelpers::DEVICE_EXTENSIONS.begin(), 
        chVulkanAPIHelpers::DEVICE_EXTENSIONS.end()
    );
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    if (!requiredExtensions.empty()) {
        return false;
    }

    //TODO: Check for swapchain support

    // Additional checks can be added here (e.g., swapchain support if needed)
    return true;
}

/*
*/
NODISCARD Optional<uint32>
VulkanAPI::findQueueFamily(VkPhysicalDevice device, VkQueueFlags queueFlags) const {
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (uint32 i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & queueFlags) {
      return i;
    }
  }

  return std::nullopt;
}

/*
*/
void
VulkanAPI::createLogicalDevice() {

  auto graphicsQueueFamily = findQueueFamily(m_vulkanData->physicalDevice, 
                                            VK_QUEUE_GRAPHICS_BIT);
  
  if (!graphicsQueueFamily.has_value()) {
    CH_EXCEPT(VulkanErrorException, "Failed to find a suitable queue family");
  }

  m_graphicsQueueFamilyIndex = *graphicsQueueFamily;
  // For now we are using the same queue for graphics and present
  m_presentQueueFamilyIndex = *graphicsQueueFamily;

  // auto computeQueueFamily = findQueueFamily(m_vulkanData->physicalDevice, VK_QUEUE_COMPUTE_BIT);
  // auto transferQueueFamily = findQueueFamily(m_vulkanData->physicalDevice, VK_QUEUE_TRANSFER_BIT);

  // Set<uint32> uniqueQueueFamilies = { m_vulkanData->graphicsQueueFamilyIndex };

  // Queue creation info, for now we are using only one queue
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = m_graphicsQueueFamilyIndex,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority
  };

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueCreateInfo,
    .enabledExtensionCount = static_cast<uint32>(DEVICE_EXTENSIONS.size()),
    .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
    .pEnabledFeatures = &deviceFeatures
  };

  VK_CHECK (vkCreateDevice(m_vulkanData->physicalDevice, 
                           &createInfo, 
                           nullptr,
                           &m_vulkanData->device));

  // Get the graphics queue and encapsulate it in a command queue
  m_graphicsQueue = chMakeShared<VulkanCommandQueue>(m_vulkanData->device,
                                          m_graphicsQueueFamilyIndex, 
                                          QueueType::Graphics);
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

  VkDebugUtilsMessengerCreateInfoEXT createInfo{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugUtilsMessageCallback,
    .pUserData = nullptr,
  };

  VK_CHECK(func(m_vulkanData->instance, &createInfo, nullptr, &m_vulkanData->debugMessenger));
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
void
VulkanAPI::createSurface(WeakPtr<DisplaySurface> display) {
  if (m_vulkanData->surface != VK_NULL_HANDLE) {
    CH_LOG_WARNING(Vulkan, "Vulkan surface already created");
    return;
  }

#if USING(CH_PLATFORM_WIN32)

  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
  surfaceCreateInfo.hwnd = (HWND)platformWindow;
  VK_CHECK (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &m_surface));

#elif USING(CH_PLATFORM_LINUX)

  CH_LOG_DEBUG(Vulkan, "Creating Vulkan surface for Linux");

  if (display.expired()) {
    CH_EXCEPT(InternalErrorException, "DisplaySurface is expired");
  }
  SPtr<DisplaySurface> displayPtr = display.lock();

  VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.connection = XCBGlobals::getXCBConnection();
  surfaceCreateInfo.window = displayPtr->getPlatformHandlerInt();
  VkInstance instance = g_vulkanAPI().getInstance();
  VK_CHECK (vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &m_vulkanData->surface));

#endif // USING(CH_PLATFORM_WIN32)

  VkBool32 presentSupported = VK_FALSE;
  VK_CHECK (vkGetPhysicalDeviceSurfaceSupportKHR(m_vulkanData->physicalDevice, 
                                                 m_graphicsQueueFamilyIndex, 
                                                 m_vulkanData->surface, 
                                                 &presentSupported));
  if (presentSupported != VK_TRUE) {
    CH_EXCEPT(VulkanErrorException, "Failed to create Vulkan surface");
  }

  uint32 formatCount = 0;
  VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanData->physicalDevice, 
                                                 m_vulkanData->surface, 
                                                 &formatCount, 
                                                 nullptr));

  Vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanData->physicalDevice, 
                                                 m_vulkanData->surface, 
                                                 &formatCount, 
                                                 surfaceFormats.data()));

  m_vulkanData->surfaceFormat = surfaceFormats[0].format;
  m_vulkanData->colorSpace = surfaceFormats[0].colorSpace;
}

/*
*/
void
VulkanAPI::waitIdle() {
   VkResult res =  vkDeviceWaitIdle(m_vulkanData->device);
    if (res != VK_SUCCESS) {
      std::cout << res << std::endl;
    }
}

/*
*/
VulkanAPI& g_vulkanAPI() {
  return reinterpret_cast<VulkanAPI&>(IGraphicsAPI::instance());
}
} // namespace chEngineSDK