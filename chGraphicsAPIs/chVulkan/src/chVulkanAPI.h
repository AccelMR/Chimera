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
#include "chVulkanBindlessHeap.h"
#include "chVulkanCommandList.h"
#include "chVulkanDeletionQueue.h"
#include "chVulkanUploader.h"

namespace chEngineSDK {
class VulkanSwapChain;

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

  NODISCARD SPtr<IShader>
  createShader(const ShaderCreateInfo& createInfo) override;

  NODISCARD SPtr<IPipeline>
  createGraphicsPipeline(const GraphicsPipelineDesc& desc) override;

  NODISCARD SPtr<ISampler>
  createSampler(const SamplerCreateInfo& createInfo) override;

  NODISCARD ICommandList&
  beginFrame() override;

  void
  endFrame() override;

  NODISCARD uint32
  getFrameIndex() const override
  {
    return m_frameIndex;
  }

  /**
   * Called by a swap chain that acquired an image in the current frame, so endFrame waits
   * for the image and signals the semaphore its present waits on.
   */
  void
  addFrameSwapChain(const VulkanSwapChain& swapChain);

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

  NODISCARD FORCEINLINE VulkanUploader&
  getUploader()
  {
    return m_uploader;
  }

  NODISCARD FORCEINLINE VulkanBindlessHeap&
  getBindlessHeap()
  {
    return m_bindlessHeap;
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
  createAllocator();

  void
  createFrames();

  void
  destroyFrames();

  void
  setDebugNameHandle(VkObjectType type, uint64 handle, const ANSICHAR* name) const;

  struct FrameData
  {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    UniquePtr<VulkanCommandList> commandList;
    // Value of the deletion queue timeline once the GPU finishes the last frame of the slot.
    uint64 submitValue = 0;
  };

  // The main window plus the editor's floating windows.
  static constexpr uint32 MAX_FRAME_SWAP_CHAINS = 8;

  UniquePtr<VulkanData> m_vulkanData;

  VmaAllocator m_allocator = nullptr;
  VulkanBindlessHeap m_bindlessHeap;
  VulkanDeletionQueue m_deletionQueue;
  VulkanUploader m_uploader;
  PFN_vkSetDebugUtilsObjectNameEXT m_setDebugUtilsObjectName = nullptr;

  VkQueue m_graphicsQueueHandle = VK_NULL_HANDLE;
  uint32 m_graphicsQueueFamilyIndex = 0;
  uint32 m_presentQueueFamilyIndex = 0;

  Array<FrameData, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_frames;
  uint32 m_frameIndex = 0;
  // Swap chains that acquired an image in the current frame.
  Array<const VulkanSwapChain*, MAX_FRAME_SWAP_CHAINS> m_frameSwapChains{};
  uint32 m_frameSwapChainCount = 0;
};

VulkanAPI& g_vulkanAPI();

} // namespace chEngineSDK
