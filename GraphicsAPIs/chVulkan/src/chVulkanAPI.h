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

namespace chEngineSDK {

struct VulkanData {
  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
};

class VulkanAPI : public IGraphicsAPI {
 public:
  VulkanAPI() : m_vulkanData(chMakeUnique<VulkanData>()) {}
  ~VulkanAPI() override = default;

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

  void
  waitForDeviceIdle();

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

  UniquePtr<VulkanData> m_vulkanData;

  SPtr<ICommandQueue> m_graphicsQueue;
  uint32 m_graphicsQueueFamilyIndex = 0;

  SPtr<ICommandQueue> m_presentQueue;
  uint32 m_presentQueueFamilyIndex = 0;

  Map<String, Function<Any(const Vector<Any>&)>> m_functionMap;
};

CH_CORE_EXPORT VulkanAPI& g_vulkanAPI();

} // namespace chEngineSDK
