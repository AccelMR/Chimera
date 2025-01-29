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

#include "chVulkanFence.h"
#include "chVulkanGPUCommandBuffer.h"
#include "chVulkanGPUBuffer.h"
#include "chVulkanGPUPipelineState.h"
#include "chVulkanTranslator.h"
#include "chVulkanSwapChain.h"

#if USING(CH_SDL_WINDOW)
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif // USING(CH_SDL_WINDOW)

namespace VulkanUtils {
using namespace chEngineSDK;

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

/*
 */
VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, 
                                        VkFormat *depthStencilFormat)
{
  std::vector<VkFormat> formatList = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
  };

  for (auto &format : formatList)
  {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
    if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
      *depthStencilFormat = format;
      return true;
    }
  }

  return false;
}


/*
*/
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debugUtilsMessengerCI)
{
  debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessageCallback;
}


/*
*/
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{nullptr};
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{nullptr};
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{nullptr};
void setup(VkInstance instance)
{
  vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
  vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
  vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
}

/*
*/
VkDebugUtilsMessengerEXT debugUtilsMessenger;
void setupDebugging(VkInstance instance)
{
  vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
  setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
  VkResult result = vkCreateDebugUtilsMessengerEXT(instance, 
                                                   &debugUtilsMessengerCI, 
                                                   nullptr, 
                                                   &debugUtilsMessenger);
  assert(result == VK_SUCCESS);
}

} // namespace VulkanUtils

/*                                                                            */
/*                                                                            */
/*                                                                            */
namespace chEngineSDK {
using namespace VulkanUtils;
using std::static_pointer_cast;

/*
*/
GraphicsModuleVulkan::GraphicsModuleVulkan()
{}

/*
*/
GraphicsModuleVulkan::~GraphicsModuleVulkan() {}

/*
*/
VkDescriptorSetLayout
GraphicsModuleVulkan::createDescriptorSetLayout(const BindingGroup& bindingGroup) {
    const auto bindings = VulkanTranslator::get(bindingGroup);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout.");
    }

    return descriptorSetLayout;
}

/*
*/
uint32
GraphicsModuleVulkan::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

  for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && 
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  CH_LOG_ERROR("Failed to find suitable memory type.");
  return INVALID_INDEX;
}

/*
*/
void
GraphicsModuleVulkan::createInstance()
{
  Vector<const char *> instanceExtensions =
  {
    VK_KHR_SURFACE_EXTENSION_NAME,
  };

#if USING(CH_SDL_WINDOW)
  // Obtener las extensiones requeridas para Vulkan desde SDL
  uint32 extensionCount = 0;
  char const* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  if (!sdlExtensions) {
    CH_EXCEPT(InternalErrorException, "Failed to get Vulkan instance extensions from SDL.");
  }

  instanceExtensions.insert(instanceExtensions.end(), 
                            sdlExtensions, 
                            sdlExtensions + extensionCount);
#endif // USING(CH_SDL_WINDOW)

#if USING(CH_DEBUG_MODE)
  instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  const bool validation = true;
#else
  const bool validation = false;
#endif //USING(CH_DEBUG_MODE)

// #if USING(CH_PLATFORM_WIN32)
//   instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
//   instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
// #elif defined(_DIRECT2DISPLAY)
//   instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
//   instanceExtensions.push_back(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
//   instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_XCB_KHR)
//   instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_IOS_MVK)
//   instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_MACOS_MVK)
//   instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_METAL_EXT)
//   instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
//   instanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_SCREEN_QNX)
//   instanceExtensions.push_back(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME);
// #endif

  // Get extensions supported by the instance and store for later use
  uint32 extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
  if (extCount > 0)
  {
    Vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, 
                                               &extCount,
                                               &extensions.front()) == VK_SUCCESS) {
      for (VkExtensionProperties &extension : extensions)       {
        m_supportedInstanceExtensions.push_back(extension.extensionName);
      }
    }
  }

  // Enabled requested instance extensions
  if (m_enabledInstanceExtensions.size() > 0) {
    for (const char *enabledExtension : m_enabledInstanceExtensions) {
      // Output message if requested extension is not available
      if (std::find(m_supportedInstanceExtensions.begin(), 
                    m_supportedInstanceExtensions.end(), 
                    enabledExtension) == m_supportedInstanceExtensions.end()) {
        CH_LOG_ERROR("Enabled instance extension \"" + String(enabledExtension) + 
                  "\" is not present at instance level");
      }
      instanceExtensions.push_back(enabledExtension);
    }
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Chimera Vulkan Application";
  appInfo.pEngineName = "Chimera Engine";
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instanceCreateInfo{};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
  if (validation) {
    setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
    debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
    instanceCreateInfo.pNext = &debugUtilsMessengerCI;
  }

	// Enable the debug utils extension if available (e.g. when debugging tools are present)
  if (validation || 
      std::find(m_supportedInstanceExtensions.begin(), 
                m_supportedInstanceExtensions.end(), 
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (instanceExtensions.size() > 0) {
    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
  }

  // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
  // Note that on Android this layer requires at least NDK r20
  const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
  if (validation) {
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties &layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
    }
    if (validationLayerPresent) {
      instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
      instanceCreateInfo.enabledLayerCount = 1;
    }
    else {
      CH_LOG_ERROR("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
    }
  }
 throwIfFailed(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

  // If the debug utils extension is present we set up debug functions, so samples can label objects for debugging
  if (std::find(m_supportedInstanceExtensions.begin(), 
                m_supportedInstanceExtensions.end(), 
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_supportedInstanceExtensions.end()) {
    VulkanUtils::setup(m_instance);
  }

  if (validation) {
    setupDebugging(m_instance);
  }
}

/*
*/
void
GraphicsModuleVulkan::createSurface()
{
#if USING(CH_SDL_WINDOW)
  SDL_Window* sdlWindow = m_screen.lock()->getPlatformHandler();
  CH_ASSERT(sdlWindow);

  if (!SDL_Vulkan_CreateSurface(sdlWindow, m_instance, nullptr, &m_surface)) {
    CH_EXCEPT(InternalErrorException, "Failed to create Vulkan surface from SDL.");
  }
#endif // USING(CH_SDL_WINDOW)
}

/*
*/
void
GraphicsModuleVulkan::createDevice()
{
  uint32 gpuCount = 0;
  throwIfFailed(vkEnumeratePhysicalDevices(m_instance, &gpuCount, nullptr));

  if (gpuCount == 0) {
    CH_LOG_ERROR("No Vulkan compatible GPU found.");
  }

  Vector<VkPhysicalDevice> physicalDevices(gpuCount);
  throwIfFailed(vkEnumeratePhysicalDevices(m_instance, 
                                           &gpuCount, 
                                           physicalDevices.data()));

  // Select the first physical device
  //todo: add a way to select the device(?)
  constexpr int32 selectedDevice = 0;
  m_physicalDevice = physicalDevices[selectedDevice];

  // Store properties and features of the physical device
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
  vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_deviceMemoryProperties);

  // Registrar la memoria total disponible
  VkDeviceSize totalMemory = 0;
  for (uint32 i = 0; i < m_deviceMemoryProperties.memoryHeapCount; i++) {
    if (m_deviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
      totalMemory += m_deviceMemoryProperties.memoryHeaps[i].size;
    }
  }

  CH_LOG_INFO(StringUtils::format("Adapter descriptor: [{0}]", m_deviceProperties.deviceName));
  CH_LOG_INFO(StringUtils::format("GPU Vendor ID:  [{0}]", m_deviceProperties.vendorID));
  CH_LOG_INFO(StringUtils::format("GPU Device ID:  [{0}]", m_deviceProperties.deviceID));
  CH_LOG_INFO(StringUtils::format("Total GPU Memory: [{0} MB]", totalMemory / (1024 * 1024)));

  CH_ASSERT(m_physicalDevice);

  // Get the queue family properties
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, 
                                           &queueFamilyCount, 
                                           nullptr);
  CH_ASSERT(queueFamilyCount > 0);

  m_queueFamilyProperties.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, 
                                           &queueFamilyCount, 
                                           m_queueFamilyProperties.data());

  Vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  const float defaultQueuePriority = 0.0f;
  VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

  if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
    m_graphicsQueueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &defaultQueuePriority;
    queueCreateInfos.push_back(queueInfo);
  }
  else {
    m_graphicsQueueFamilyIndex = 0;
  }

  if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
    uint32 queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
    if (queueFamilyIndex != m_graphicsQueueFamilyIndex) {
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndex;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  }
  else {
    m_computeQueueFamilyIndex = m_graphicsQueueFamilyIndex;
  }

  // Dedicated queue for queue
  if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
    uint32 queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
    if ((queueFamilyIndex != m_graphicsQueueFamilyIndex) && 
        (queueFamilyIndex != m_computeQueueFamilyIndex)) {
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndex;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  }
  else {
    m_transferQueueFamilyIndex = m_graphicsQueueFamilyIndex;
  }

  Vector<const char *> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &m_deviceFeatures;

  m_supportedInstanceExtensions.clear();
  uint32 extCount = 0;
  vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, nullptr);
  if (extCount > 0)
  {
    Vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, &extensions.front()) 
        == VK_SUCCESS)
    {
      for (auto &ext : extensions)
      {
        m_supportedInstanceExtensions.push_back(ext.extensionName);
      }
    }
  }

  if (deviceExtensions.size() > 0) {
    for (const char* enabledExtension : deviceExtensions) {
      // Output message if requested extension is not available
      if (!extensionSupported(enabledExtension)) {
        CH_LOG_ERROR("Enabled device extension \"" + 
                  String(enabledExtension) + 
                  "\" is not present at device level");
      }
    }

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  }

  throwIfFailed(vkCreateDevice(m_physicalDevice, 
                               &deviceCreateInfo, 
                               nullptr, 
                               &m_device));

  // Get the graphics queue
  if (m_graphicsQueueFamilyIndex != INVALID_INDEX) {
    CH_LOG_INFO("Graphics queue family index: " + StringUtils::toString(m_graphicsQueueFamilyIndex));
    vkGetDeviceQueue(m_device, 
                     m_graphicsQueueFamilyIndex, 
                     0, 
                     &m_graphicsQueue);
  }

  // Get the compute queue
  if (m_computeQueueFamilyIndex != INVALID_INDEX) {
    CH_LOG_INFO("Compute queue family index: " + StringUtils::toString(m_computeQueueFamilyIndex));
    vkGetDeviceQueue(m_device, 
                     m_computeQueueFamilyIndex, 
                     0, 
                     &m_computeQueue);
  }

  // Get the transfer queue
  if (m_transferQueueFamilyIndex != INVALID_INDEX) {
    CH_LOG_INFO("Transfer queue family index: " + StringUtils::toString(m_transferQueueFamilyIndex));
    vkGetDeviceQueue(m_device, 
                     m_transferQueueFamilyIndex, 
                     0, 
                     &m_transferQueue);
  }

  // Get the present queue
  if (m_presentQueueFamilyIndex != INVALID_INDEX) {
    CH_LOG_INFO("Present queue family index: " + StringUtils::toString(m_presentQueueFamilyIndex));
    vkGetDeviceQueue(m_device, 
                     m_presentQueueFamilyIndex, 
                     0, 
                     &m_presentQueue);
  }
}

/*
*/
void
GraphicsModuleVulkan::createCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    CH_EXCEPT(InternalErrorException, "Failed to create command pool.");
  }

  CH_LOG_INFO("Command pool created successfully.");
}

/*
 */
void 
GraphicsModuleVulkan::_internalInit(WeakPtr<Screen> screen)
{
  CH_ASSERT(screen.lock());
  m_screen = screen;

  createInstance();
  createSurface();
  createDevice();

  VkBool32 presentSupport = VK_FALSE;
  for (uint32 i = 0; i < m_queueFamilyProperties.size(); ++i) {
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
    if (presentSupport) {
      m_presentQueueFamilyIndex = i;
      break;
    }
  }
  CH_ASSERT(m_presentQueueFamilyIndex != INVALID_INDEX);

  _setupSwapchain(screen.lock()->getWidth(), screen.lock()->getHeight());

  createCommandPool();
  CH_ASSERT(commandPool);

  CH_LOG_INFO("Vulkan initialized successfully.");
}

/*
*/
void
GraphicsModuleVulkan::_setupSwapchain(uint32 width, uint32 height) {

  VkBool32 validFormat{ false };
  // Samples that make use of stencil will require a depth + stencil format, so we select from a different list
  validFormat = getSupportedDepthStencilFormat(m_physicalDevice, &m_depthFormat);
  CH_ASSERT(validFormat);

  //TODO: this might be generated from the screen
  SwapChainDesc swapDesc{
    .width = width,
    .height = height,
    .format = FORMAT::kB8G8R8A8_UNORM,
    .frameCount = 3,
  };

  auto vulkanSwapChain = chMakeShared<VulkanSwapChain>(m_device, m_surface, m_physicalDevice);
  vulkanSwapChain->init(swapDesc);
  m_swapChain = vulkanSwapChain;
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
SPtr<GPUCommandBuffer>
GraphicsModuleVulkan::_internalCreateGPUCommandBuffer() {
  return std::reinterpret_pointer_cast<GPUCommandBuffer>(chMakeShared<VulkanGPUCommandBuffer>());
}

/*
*/
void
GraphicsModuleVulkan::_internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) 
{}

/*
*/
SPtr<GPUPipelineState>
GraphicsModuleVulkan::_internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) {
  auto ret = chMakeShared<VulkanGPUPipelineState>();
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
  return m_swapChain;
}

/*
*/
void
GraphicsModuleVulkan::_internalResetSwapChainAllocator() {}

/*
*/
SPtr<Fence>
GraphicsModuleVulkan::_internalCreateFence() {
  CH_ASSERT(m_device);
  return chMakeUnique<VulkanFence>(m_device);
}

/*
*/
void
GraphicsModuleVulkan::_internalSyncGPU(const WeakPtr<Fence> fence, uint64 value) {
  auto vulkanFence = std::reinterpret_pointer_cast<VulkanFence>(fence.lock());
  CH_ASSERT(vulkanFence);
  vkQueueSubmit(m_graphicsQueue, 0, nullptr, vulkanFence->getFence());
  vulkanFence->wait(value);
}

/*
 */
int32 
GraphicsModuleVulkan::getQueueFamilyIndex(VkQueueFlagBits queueFlags)
{
  // Dedicated queue for compute
  // Try to find a queue family index that supports compute but not graphics
  if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
  {
    for (uint32 i = 0; i < static_cast<uint32>(m_queueFamilyProperties.size()); i++)
    {
      if ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
          ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
      {
        return i;
      }
    }
  }

  // Dedicated queue for transfer
  // Try to find a queue family index that supports transfer but not graphics and compute
  if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
  {
    for (uint32 i = 0; i < static_cast<uint32>(m_queueFamilyProperties.size()); i++)
    {
      if ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
          ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && 
           ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
      {
        return i;
      }
    }
  }

  // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
  for (uint32 i = 0; i < static_cast<uint32>(m_queueFamilyProperties.size()); i++)
  {
    if ((m_queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
    {
      return i;
    }
  }

  CH_EXCEPT(RunTimeException, "Could not find a matching queue family index");
}

/*
*/
bool
GraphicsModuleVulkan::extensionSupported(const String extension) {
  return std::find(m_supportedInstanceExtensions.begin(), 
                   m_supportedInstanceExtensions.end(), 
                   extension) != m_supportedInstanceExtensions.end();
}

GraphicsModuleVulkan&
g_VulkanGraphicsModule() {
  return reinterpret_cast<GraphicsModuleVulkan&>(GraphicsModule::instance());
}

}