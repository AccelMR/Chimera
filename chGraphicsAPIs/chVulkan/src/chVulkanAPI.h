/************************************************************************/
/**
 * @file chVulkanAPI.h
 * @author AccelMR
 * @date 2025/04/07
 * @brief
 * Vulkan API implementation of the graphics API interface.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

#include "chIGraphicsAPI.h"
#include "chVulkanDeletionQueue.h"

namespace chEngineSDK {

struct VulkanData {
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
};

class VulkanAPI : public IGraphicsAPI {
 public:
  VulkanAPI() : m_vulkanData(chMakeUnique<VulkanData>()) {}
  ~VulkanAPI() override;

  void
  initialize(const GraphicsAPIInfo& graphicsAPIInfo) override;

  NODISCARD String
  getAdapterName() const override;

  NODISCARD SPtr<ISwapChain>
  createSwapChain(uint32 width, uint32 height, bool vsync = false) override;

  NODISCARD SPtr<IBuffer>
  createBuffer(const BufferCreateInfo& createInfo) override;

  NODISCARD SPtr<ITexture>
  createTexture(const TextureCreateInfo& createInfo) override;

  NODISCARD SPtr<ICommandPool>
  createCommandPool(QueueType queueType, bool transient = false) override;

  NODISCARD SPtr<IFence>
  createFence(bool signaled = false) override;

  NODISCARD SPtr<ISemaphore>
  createSemaphore() override;

  NODISCARD SPtr<IShader>
  createShader(const ShaderCreateInfo& createInfo) override;

  NODISCARD SPtr<IPipeline>
  createPipeline(const PipelineCreateInfo& createInfo) override;

  NODISCARD SPtr<IRenderPass>
  createRenderPass(const RenderPassCreateInfo& createInfo) override;

  NODISCARD SPtr<IFrameBuffer>
  createFrameBuffer(const FrameBufferCreateInfo& createInfo) override;

  NODISCARD SPtr<ICommandQueue>
  getQueue(QueueType queueType) override;

  NODISCARD virtual SPtr<ISampler>
  createSampler(const SamplerCreateInfo& createInfo) override;

  NODISCARD SPtr<IDescriptorSetLayout>
  createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) override;

  NODISCARD SPtr<IDescriptorPool>
  createDescriptorPool(const DescriptorPoolCreateInfo& createInfo) override;

  void
  updateDescriptorSets(const Vector<WriteDescriptorSet>& writeDescriptorSets) override;

  void
  waitIdle() override;

  FORCEINLINE VkDevice
  getDevice() const {
    return m_vulkanData->device;
  }

  FORCEINLINE VkPhysicalDevice
  getPhysicalDevice() const {
    return m_vulkanData->physicalDevice;
  }

  FORCEINLINE VkInstance
  getInstance() const {
    return m_vulkanData->instance;
  }

  FORCEINLINE uint32
  getGraphicsQueueFamilyIndex() const {
    return m_graphicsQueueFamilyIndex;
  }

  NODISCARD FORCEINLINE VkQueue
  getGraphicsQueueHandle() const
  {
    return m_graphicsQueueHandle;
  }

  NODISCARD FORCEINLINE VmaAllocator
  getAllocator() const
  {
    return m_allocator;
  }

  NODISCARD FORCEINLINE VulkanDeletionQueue&
  getDeletionQueue()
  {
    return m_deletionQueue;
  }

  /**
   * Names an object for the validation messages and for RenderDoc. Does nothing when the
   * debug utils extension is off.
   */
  template<typename HandleType>
  void
  setDebugName(VkObjectType type, HandleType handle, const ANSICHAR* name) const
  {
    setDebugNameHandle(type, reinterpret_cast<uint64>(handle), name);
  }

  virtual Any
  execute(const String& functionName, const Vector<Any>& args = {}) override;

 private:

  void
  createInstance(const GraphicsAPIInfo& graphicsAPIInfo);

  NODISCARD bool
  pickPhysicalDevice();

  NODISCARD bool
  isDeviceSuitable(VkPhysicalDevice device) const;

  NODISCARD Optional<uint32>
  findQueueFamily(VkPhysicalDevice device, VkQueueFlags queueFlags) const;

  void
  createLogicalDevice();

  void
  setupDebugMessenger(const GraphicsAPIInfo& graphicsAPIInfo);

  bool
  checkValidationLayerSupport() const;

  bool
  createSurface(WeakPtr<DisplaySurface> display);

  void
  initializeFunctionMap();

  void
  createAllocator();

  void
  setDebugNameHandle(VkObjectType type, uint64 handle, const ANSICHAR* name) const;

  UniquePtr<VulkanData> m_vulkanData;

  VmaAllocator m_allocator = nullptr;
  VulkanDeletionQueue m_deletionQueue;
  PFN_vkSetDebugUtilsObjectNameEXT m_setDebugUtilsObjectName = nullptr;

  SPtr<ICommandQueue> m_graphicsQueue;
  VkQueue m_graphicsQueueHandle = VK_NULL_HANDLE;
  uint32 m_graphicsQueueFamilyIndex = 0;

  SPtr<ICommandQueue> m_presentQueue;
  uint32 m_presentQueueFamilyIndex = 0;

  Map<String, Function<Any(const Vector<Any>&)>> m_functionMap;
};

VulkanAPI& g_vulkanAPI();

} // namespace chEngineSDK
