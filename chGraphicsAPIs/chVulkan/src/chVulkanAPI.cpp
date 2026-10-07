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

#include <cstring>

#include <vk_mem_alloc.h>

#include "chAlgorithm.h"
#include "chSTDStreams.h"
#include "chVulkanBuffer.h"
#include "chVulkanPipeline.h"
#include "chVulkanSampler.h"
#include "chVulkanShader.h"
#include "chVulkanSwapChain.h"
#include "chVulkanTexture.h"
#include "chVulkanTextureView.h"

namespace chVulkanAPIHelpers {
constexpr chEngineSDK::Array<const chEngineSDK::ANSICHAR*, 1> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"};

// Shaders read every resource from one bindless heap (ResourceDescriptorHeap), which DXC
// maps to a single binding, so that binding must accept several descriptor types.
constexpr chEngineSDK::Array<const chEngineSDK::ANSICHAR*, 2> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME};

/**
 * Feature structs of the device, linked in the order vkGetPhysicalDeviceFeatures2 and
 * vkCreateDevice read them. Not copyable: the structs point to each other.
 */
struct DeviceFeatureChain
{
  DeviceFeatureChain()
  {
    mutableDescriptor.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
    vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13.pNext = &mutableDescriptor;
    vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12.pNext = &vulkan13;
    core.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    core.pNext = &vulkan12;
  }

  DeviceFeatureChain(const DeviceFeatureChain&) = delete;
  DeviceFeatureChain&
  operator=(const DeviceFeatureChain&) = delete;

  VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptor{};
  VkPhysicalDeviceVulkan13Features vulkan13{};
  VkPhysicalDeviceVulkan12Features vulkan12{};
  VkPhysicalDeviceFeatures2 core{};
};

/**
 * Calls visit(feature, name) for every feature the engine needs, so checking support and
 * enabling them use the same list.
 */
template<typename Visitor>
void
visitRequiredFeatures(DeviceFeatureChain& chain, Visitor&& visit)
{
  visit(chain.vulkan13.dynamicRendering, "dynamicRendering");
  visit(chain.vulkan13.synchronization2, "synchronization2");
  visit(chain.vulkan12.timelineSemaphore, "timelineSemaphore");
  visit(chain.vulkan12.descriptorIndexing, "descriptorIndexing");
  visit(chain.vulkan12.runtimeDescriptorArray, "runtimeDescriptorArray");
  visit(chain.vulkan12.descriptorBindingPartiallyBound, "descriptorBindingPartiallyBound");
  visit(chain.vulkan12.descriptorBindingUpdateUnusedWhilePending,
        "descriptorBindingUpdateUnusedWhilePending");
  visit(chain.vulkan12.descriptorBindingUniformBufferUpdateAfterBind,
        "descriptorBindingUniformBufferUpdateAfterBind");
  visit(chain.vulkan12.descriptorBindingSampledImageUpdateAfterBind,
        "descriptorBindingSampledImageUpdateAfterBind");
  visit(chain.vulkan12.descriptorBindingStorageImageUpdateAfterBind,
        "descriptorBindingStorageImageUpdateAfterBind");
  visit(chain.vulkan12.descriptorBindingStorageBufferUpdateAfterBind,
        "descriptorBindingStorageBufferUpdateAfterBind");
  visit(chain.vulkan12.shaderSampledImageArrayNonUniformIndexing,
        "shaderSampledImageArrayNonUniformIndexing");
  visit(chain.vulkan12.shaderStorageImageArrayNonUniformIndexing,
        "shaderStorageImageArrayNonUniformIndexing");
  visit(chain.vulkan12.shaderStorageBufferArrayNonUniformIndexing,
        "shaderStorageBufferArrayNonUniformIndexing");
  visit(chain.mutableDescriptor.mutableDescriptorType, "mutableDescriptorType");
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                          void* pUserData) {
  using namespace chEngineSDK;
  // Select prefix depending on flags passed to the callback
  String prefix;

  CH_PARAMETER_UNUSED(messageType);
  CH_PARAMETER_UNUSED(pUserData);
  // CH_LOG_DEBUG(Vulkan, "Vulkan message type: {0}", messageType.value);

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[32m" + prefix + "\033[0m";
#endif // USING(CH_PLATFORM_WIN32)
    prefix = "VERBOSE: ";
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    prefix = "INFO: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[36m" + prefix + "\033[0m";
#endif // USING(CH_PLATFORM_WIN32)
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    prefix = "WARNING: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[33m" + prefix + "\033[0m";
#endif // USING(CH_PLATFORM_WIN32)
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    prefix = "ERROR: ";
#if USING(CH_PLATFORM_WIN32)
    prefix = "\033[31m" + prefix + "\033[0m";
#endif // USING(CH_PLATFORM_WIN32)
  }

  // Display message to default output (console/logcat)
  chEngineSDK::StringStream debugMessage;
  if (pCallbackData->pMessageIdName) {
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "]["
                 << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
  }
  else {
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber
                 << "] : " << pCallbackData->pMessage;
  }

#if defined(__ANDROID__)
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    LOGE("%s", debugMessage.str().c_str());
  }
  else {
    LOGD("%s", debugMessage.str().c_str());
  }
#else
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    CH_LOG_ERROR(Vulkan, debugMessage.str());
  }
  else {
    CH_LOG_DEBUG(Vulkan, debugMessage.str());
  }
  fflush(stdout);
#endif

  // The return value of this callback controls whether the Vulkan call that caused the
  // validation message will be aborted or not We return VK_FALSE as we DON'T want Vulkan calls
  // that cause a validation message to abort If you instead want to have calls abort, pass in
  // VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT
  return VK_FALSE;
}
} // namespace chVulkanAPIHelpers
using namespace chVulkanAPIHelpers;

namespace chEngineSDK {

struct VulkanContextData {
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  uint32 graphicsQueueFamilyIndex;
  VkQueue graphicsQueue;
};

/*
 */
VulkanAPI::~VulkanAPI()
{
  VulkanData& data = *m_vulkanData;
  if (data.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(data.device);
    destroyFrames();
    m_uploader.destroy();
    // Frees the pending bindless indexes too, so it runs before the heap is destroyed.
    m_deletionQueue.destroy();
    m_bindlessHeap.destroy();
  }

  if (m_allocator != nullptr) {
    vmaDestroyAllocator(m_allocator);
    m_allocator = nullptr;
  }

  if (data.device != VK_NULL_HANDLE) {
    vkDestroyDevice(data.device, nullptr);
    data.device = VK_NULL_HANDLE;
  }

  if (data.instance == VK_NULL_HANDLE) {
    return;
  }

  // Destroyed after the device, so the validation layers can still report the objects
  // that were never released.
  if (data.debugMessenger != VK_NULL_HANDLE) {
    auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(data.instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyMessenger) {
      destroyMessenger(data.instance, data.debugMessenger, nullptr);
    }
    data.debugMessenger = VK_NULL_HANDLE;
  }

  vkDestroyInstance(data.instance, nullptr);
  data.instance = VK_NULL_HANDLE;
}

/*
 */
void
VulkanAPI::initialize(const GraphicsAPIInfo& graphicsAPIInfo) {
  CH_LOG_DEBUG(Vulkan, "Initializing Vulkan API");

  // Initialize Vulkan instance, physical device, and logical device.
  createInstance(graphicsAPIInfo);

  if (graphicsAPIInfo.enableValidationLayer) {
    setupDebugMessenger(graphicsAPIInfo);
  }

  if (!pickPhysicalDevice()) {
    CH_EXCEPT(VulkanErrorException,
              "No GPU supports Vulkan 1.3 with the features the engine needs; the warnings "
              "above list what each GPU is missing.");
  }

  createLogicalDevice();
  createAllocator();
  createFrames();

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
uint64
VulkanAPI::getPlatformWindowFlags() const
{
#if USING(CH_DISPLAY_SDL3)
  // SDL only makes Vulkan surfaces on windows created with this flag.
  return SDL_WINDOW_VULKAN;
#else
  return 0;
#endif // USING(CH_DISPLAY_SDL3)
}

/*
 */
NODISCARD SPtr<ISwapChain>
VulkanAPI::createSwapChain(const SwapChainDesc& desc)
{
  return chMakeShared<VulkanSwapChain>(m_vulkanData->instance, m_vulkanData->device,
                                       m_vulkanData->physicalDevice,
                                       m_graphicsQueueFamilyIndex, m_presentQueueFamilyIndex,
                                       desc);
}

/*
 */
NODISCARD SPtr<IBuffer>
VulkanAPI::createBuffer(const BufferCreateInfo& createInfo) {
  return chMakeShared<VulkanBuffer>(m_allocator, createInfo);
}

/*
 */
NODISCARD SPtr<ITexture>
VulkanAPI::createTexture(const TextureCreateInfo& createInfo) {
  return chMakeShared<VulkanTexture>(m_vulkanData->device, m_allocator, createInfo);
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
VulkanAPI::createGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
  return chMakeShared<VulkanPipeline>(m_vulkanData->device, m_bindlessHeap.getPipelineLayout(),
                                      desc);
}

/*
 */
SPtr<ISampler>
VulkanAPI::createSampler(const SamplerCreateInfo& createInfo) {
  return chMakeShared<VulkanSampler>(m_vulkanData->device, createInfo);
}

/*
 */
ICommandList&
VulkanAPI::beginFrame()
{
  FrameData& frame = m_frames[m_frameIndex];

  const VkSemaphore timeline = m_deletionQueue.getTimeline();
  const VkSemaphoreWaitInfo waitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                                     .pNext = nullptr,
                                     .flags = 0,
                                     .semaphoreCount = 1,
                                     .pSemaphores = &timeline,
                                     .pValues = &frame.submitValue};
  VK_CHECK(vkWaitSemaphores(m_vulkanData->device, &waitInfo, UINT64_MAX));

  VK_CHECK(vkResetCommandPool(m_vulkanData->device, frame.commandPool, 0));
  frame.commandList->begin();
  return *frame.commandList;
}

/*
 */
void
VulkanAPI::endFrame()
{
  FrameData& frame = m_frames[m_frameIndex];
  frame.commandList->end();

  Array<VkSemaphoreSubmitInfo, MAX_FRAME_SWAP_CHAINS> waits{};
  // One more slot for the deletion queue timeline.
  Array<VkSemaphoreSubmitInfo, MAX_FRAME_SWAP_CHAINS + 1> signals{};
  for (uint32 i = 0; i < m_frameSwapChainCount; ++i) {
    const VulkanSwapChain& swapChain = *m_frameSwapChains[i];
    // Only drawing into the image waits for it; the work before runs in the meantime.
    waits[i] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = swapChain.getAcquireSemaphore(),
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .deviceIndex = 0};
    signals[i] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                  .pNext = nullptr,
                  .semaphore = swapChain.getPresentSemaphore(),
                  .value = 0,
                  .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                  .deviceIndex = 0};
  }

  frame.submitValue = m_deletionQueue.nextSubmitValue();
  signals[m_frameSwapChainCount] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                    .pNext = nullptr,
                                    .semaphore = m_deletionQueue.getTimeline(),
                                    .value = frame.submitValue,
                                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                    .deviceIndex = 0};

  // Copies recorded since the last frame run first, so this frame already sees their data.
  Array<VkCommandBufferSubmitInfo, 2> commandBufferInfos{};
  uint32 commandBufferCount = 0;
  const VkCommandBuffer uploadCommands = m_uploader.endRecording(frame.submitValue);
  if (uploadCommands != VK_NULL_HANDLE) {
    commandBufferInfos[commandBufferCount++] = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = uploadCommands,
        .deviceMask = 0};
  }
  commandBufferInfos[commandBufferCount++] = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .pNext = nullptr,
      .commandBuffer = frame.commandList->getHandle(),
      .deviceMask = 0};

  const VkSubmitInfo2 submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                 .pNext = nullptr,
                                 .flags = 0,
                                 .waitSemaphoreInfoCount = m_frameSwapChainCount,
                                 .pWaitSemaphoreInfos = waits.data(),
                                 .commandBufferInfoCount = commandBufferCount,
                                 .pCommandBufferInfos = commandBufferInfos.data(),
                                 .signalSemaphoreInfoCount = m_frameSwapChainCount + 1,
                                 .pSignalSemaphoreInfos = signals.data()};
  VK_CHECK(vkQueueSubmit2(m_graphicsQueueHandle, 1, &submitInfo, VK_NULL_HANDLE));

  m_frameSwapChainCount = 0;
  m_frameIndex = (m_frameIndex + 1) % GraphicsLimits::MAX_FRAMES_IN_FLIGHT;
  m_deletionQueue.collect();
}

/*
 */
void
VulkanAPI::addFrameSwapChain(const VulkanSwapChain& swapChain)
{
  CH_ASSERT(m_frameSwapChainCount < MAX_FRAME_SWAP_CHAINS);
  m_frameSwapChains[m_frameSwapChainCount++] = &swapChain;
}

/*
 */
void
VulkanAPI::createFrames()
{
  // Each pool is reset whole at the start of its frame, so its buffers are short lived.
  const VkCommandPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                         .pNext = nullptr,
                                         .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                         .queueFamilyIndex = m_graphicsQueueFamilyIndex};

  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    FrameData& frame = m_frames[i];
    VK_CHECK(vkCreateCommandPool(m_vulkanData->device, &poolInfo, nullptr,
                                 &frame.commandPool));
    frame.commandList = chMakeUnique<VulkanCommandList>(m_vulkanData->device,
                                                        frame.commandPool);

    const String poolName = StringUtils::format("Frame Command Pool {0}", i);
    setDebugName(VK_OBJECT_TYPE_COMMAND_POOL, frame.commandPool, poolName.c_str());
    const String listName = StringUtils::format("Frame Command List {0}", i);
    setDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, frame.commandList->getHandle(),
                 listName.c_str());
  }
}

/*
 */
void
VulkanAPI::destroyFrames()
{
  for (FrameData& frame : m_frames) {
    // The command buffer goes back to its pool, so it is freed first.
    frame.commandList.reset();
    if (frame.commandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(m_vulkanData->device, frame.commandPool, nullptr);
      frame.commandPool = VK_NULL_HANDLE;
    }
  }
}

/*
 */
void
VulkanAPI::createInstance(const GraphicsAPIInfo& graphicsAPIInfo) {
  CH_LOG_DEBUG(Vulkan, "Creating Vulkan instance");

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pNext = nullptr,
                            .pApplicationName = "chEngine Application",
                            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                            .pEngineName = "chEngine",
                            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                            .apiVersion = VK_API_VERSION_1_3};

  Vector<const ANSICHAR*> extensions;

  // SDL creates the surface, so it knows which surface extensions the window system needs
  // (Win32, X11, Wayland...), and no platform header has to reach this module.
#if USING(CH_DISPLAY_SDL3)
  uint32 count = 0;
  const ANSICHAR* const* exts = SDL_Vulkan_GetInstanceExtensions(&count);
  if (!exts) {
    CH_EXCEPT(VulkanErrorException, "Failed to get Vulkan instance extensions from SDL");
  }
  extensions.assign(exts, exts + count);
#else
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#endif // USING(CH_DISPLAY_SDL3)

  if (graphicsAPIInfo.enableValidationLayer) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32>(VALIDATION_LAYERS.size()),
      .ppEnabledLayerNames = VALIDATION_LAYERS.data(),
      .enabledExtensionCount = static_cast<uint32>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (graphicsAPIInfo.enableValidationLayer) {
    if (!checkValidationLayerSupport()) {
      CH_EXCEPT(VulkanErrorException, "Validation layers requested but not available");
    }

    createInfo.enabledLayerCount = static_cast<uint32>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
  }
  else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_vulkanData->instance));
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

  Algorithm::sort(devices,
                  [](const VkPhysicalDevice& a, const VkPhysicalDevice& b) {
                    VkPhysicalDeviceProperties propertiesA;
                    vkGetPhysicalDeviceProperties(a, &propertiesA);
                    VkPhysicalDeviceProperties propertiesB;
                    vkGetPhysicalDeviceProperties(b, &propertiesB);

                    int32 scoreA = 0;
                    int32 scoreB = 0;

                    switch (propertiesA.deviceType) {
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                      scoreA = 4;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                      scoreA = 3;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                      scoreA = 2;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                      scoreA = 1;
                      break;
                    default:
                      scoreA = 0;
                    }

                    switch (propertiesB.deviceType) {
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                      scoreB = 4;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                      scoreB = 3;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                      scoreB = 2;
                      break;
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                      scoreB = 1;
                      break;
                    default:
                      scoreB = 0;
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
  CH_LOG_INFO(Vulkan, "Total GPU Memory: [{0} MB]", totalMemory / (1024 * 1024));

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
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  Set<String> requiredExtensions(chVulkanAPIHelpers::DEVICE_EXTENSIONS.begin(),
                                 chVulkanAPIHelpers::DEVICE_EXTENSIONS.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  if (!requiredExtensions.empty()) {
    CH_LOG_WARNING(Vulkan, "{0} is missing the device extension {1}",
                   properties.deviceName, *requiredExtensions.begin());
    return false;
  }

  if (properties.apiVersion < VK_API_VERSION_1_3) {
    CH_LOG_WARNING(Vulkan, "{0} supports Vulkan {1}.{2}, the engine needs 1.3",
                   properties.deviceName, VK_API_VERSION_MAJOR(properties.apiVersion),
                   VK_API_VERSION_MINOR(properties.apiVersion));
    return false;
  }

  DeviceFeatureChain supported;
  vkGetPhysicalDeviceFeatures2(device, &supported.core);

  bool hasAllFeatures = true;
  visitRequiredFeatures(supported, [&](VkBool32 feature, const ANSICHAR* name) {
    if (feature != VK_TRUE) {
      CH_LOG_WARNING(Vulkan, "{0} is missing the feature {1}", properties.deviceName, name);
      hasAllFeatures = false;
    }
  });
  return hasAllFeatures;
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

  auto graphicsQueueFamily =
      findQueueFamily(m_vulkanData->physicalDevice, VK_QUEUE_GRAPHICS_BIT);

  if (!graphicsQueueFamily.has_value()) {
    CH_EXCEPT(VulkanErrorException, "Failed to find a suitable queue family");
  }

  m_graphicsQueueFamilyIndex = *graphicsQueueFamily;
  // For now we are using the same queue for graphics and present
  m_presentQueueFamilyIndex = *graphicsQueueFamily;

  // auto computeQueueFamily = findQueueFamily(m_vulkanData->physicalDevice,
  // VK_QUEUE_COMPUTE_BIT); auto transferQueueFamily =
  // findQueueFamily(m_vulkanData->physicalDevice, VK_QUEUE_TRANSFER_BIT);

  // Set<uint32> uniqueQueueFamilies = { m_vulkanData->graphicsQueueFamilyIndex };

  // Queue creation info, for now we are using only one queue
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                          .pNext = nullptr,
                                          .flags = 0,
                                          .queueFamilyIndex = m_graphicsQueueFamilyIndex,
                                          .queueCount = 1,
                                          .pQueuePriorities = &queuePriority};

  DeviceFeatureChain enabledFeatures;
  visitRequiredFeatures(enabledFeatures,
                        [](VkBool32& feature, const ANSICHAR*) { feature = VK_TRUE; });

  VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                .pNext = &enabledFeatures.core,
                                .flags = 0,
                                .queueCreateInfoCount = 1,
                                .pQueueCreateInfos = &queueCreateInfo,
                                .enabledLayerCount = 0,
                                .ppEnabledLayerNames = nullptr,
                                .enabledExtensionCount =
                                    static_cast<uint32>(DEVICE_EXTENSIONS.size()),
                                .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
                                .pEnabledFeatures = nullptr};

  VK_CHECK(vkCreateDevice(m_vulkanData->physicalDevice, &createInfo, nullptr,
                          &m_vulkanData->device));

  vkGetDeviceQueue(m_vulkanData->device, m_graphicsQueueFamilyIndex, 0,
                   &m_graphicsQueueHandle);
  setDebugName(VK_OBJECT_TYPE_QUEUE, m_graphicsQueueHandle, "Graphics Queue");
}

/*
 */
void
VulkanAPI::createAllocator()
{
  VmaAllocatorCreateInfo createInfo{};
  createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  createInfo.physicalDevice = m_vulkanData->physicalDevice;
  createInfo.device = m_vulkanData->device;
  createInfo.instance = m_vulkanData->instance;
  VK_CHECK(vmaCreateAllocator(&createInfo, &m_allocator));

  m_bindlessHeap.initialize(m_vulkanData->device);
  m_deletionQueue.initialize(m_vulkanData->device, m_allocator, &m_bindlessHeap);
  setDebugName(VK_OBJECT_TYPE_SEMAPHORE, m_deletionQueue.getTimeline(),
               "Deletion Queue Timeline");
  m_uploader.initialize(m_vulkanData->device, m_allocator, m_graphicsQueueFamilyIndex,
                        &m_deletionQueue);
}

/*
 */
void
VulkanAPI::setDebugNameHandle(VkObjectType type, uint64 handle, const ANSICHAR* name) const
{
  if (m_setDebugUtilsObjectName == nullptr || handle == 0) {
    return;
  }

  VkDebugUtilsObjectNameInfoEXT nameInfo{.sType =
                                             VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                         .pNext = nullptr,
                                         .objectType = type,
                                         .objectHandle = handle,
                                         .pObjectName = name};
  m_setDebugUtilsObjectName(m_vulkanData->device, &nameInfo);
}

/*
 */
void
VulkanAPI::setupDebugMessenger(const GraphicsAPIInfo& graphicsAPIInfo) {
  if (!graphicsAPIInfo.enableValidationLayer) {
    return;
  }

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      m_vulkanData->instance, "vkCreateDebugUtilsMessengerEXT");

  if (func == nullptr) {
    CH_EXCEPT(VulkanErrorException, "Failed to load debug messenger extension");
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugUtilsMessageCallback,
      .pUserData = nullptr};

  VK_CHECK(func(m_vulkanData->instance, &createInfo, nullptr, &m_vulkanData->debugMessenger));

  m_setDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
      vkGetInstanceProcAddr(m_vulkanData->instance, "vkSetDebugUtilsObjectNameEXT"));
}

/*
 */
bool
VulkanAPI::checkValidationLayerSupport() const {
  uint32 layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  Vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const ANSICHAR* layerName : VALIDATION_LAYERS) {
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
VulkanAPI::waitIdle()
{
  const VkResult result = vkDeviceWaitIdle(m_vulkanData->device);
  if (result != VK_SUCCESS) {
    CH_LOG_ERROR(Vulkan, "vkDeviceWaitIdle failed: {0}", result);
    return;
  }

  // Every submit has finished, but objects released after the last one may still be used
  // by commands recorded and not submitted yet (staging buffers of pending uploads, the
  // open frame), so they wait for the next submit like always.
  m_deletionQueue.collect();
}

/*
 */
VulkanAPI&
g_vulkanAPI() {
  return reinterpret_cast<VulkanAPI&>(IGraphicsAPI::instance());
}

} // namespace chEngineSDK
