/************************************************************************/
/**
 * @file chIGraphicsAPI.h
 * @author AccelMR
 * @date 2025/04/07
 * @brief
 *  Interface for the graphics API. This is the base class for all graphics
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chModule.h"


namespace chEngineSDK {

struct GraphicsAPIInfo {
  WeakPtr<DisplaySurface> weakDisplaySurface;
  uint32 width = 1920;
  uint32 height = 1080;
  bool enableValidationLayer = true;
};

class CH_CORE_EXPORT IGraphicsAPI : public Module<IGraphicsAPI> {
  public:
  IGraphicsAPI() = default;
  virtual ~IGraphicsAPI() = default;

  virtual void
  initialize(const GraphicsAPIInfo& graphicsAPIInfo) = 0;

  NODISCARD virtual String
  getAdapterName() const = 0;

  NODISCARD virtual SPtr<ISwapChain>
  createSwapChain(uint32 width, uint32 height, bool vsync = false) = 0;

  NODISCARD virtual SPtr<IBuffer>
  createBuffer(const BufferCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<ITexture>
  createTexture(const TextureCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<ICommandPool>
  createCommandPool(QueueType queueType, bool transient = false) = 0;

  NODISCARD virtual SPtr<IFence>
  createFence(bool signaled = false) = 0;

  NODISCARD virtual SPtr<ISemaphore>
  createSemaphore() = 0;

  NODISCARD virtual SPtr<IShader>
  createShader(const ShaderCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IPipeline>
  createPipeline(const PipelineCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IRenderPass>
  createRenderPass(const RenderPassCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IFrameBuffer>
  createFrameBuffer(const FrameBufferCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<ICommandQueue>
  getQueue(QueueType queueType) = 0;

  NODISCARD virtual SPtr<ISampler>
  createSampler(const SamplerCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IDescriptorSetLayout>
  createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IDescriptorPool>
  createDescriptorPool(const DescriptorPoolCreateInfo& createInfo) = 0;

  virtual void
  updateDescriptorSets(const Vector<WriteDescriptorSet>& writeDescriptorSets) = 0;

  virtual void
  waitIdle() = 0;

  /**
  * @brief Executes a function with the given name and arguments.
  *        This is a pure virtual function that must be implemented by derived classes.
  * @param functionName The name of the function to execute.
  * @param args The arguments to pass to the function.
  * @return The result of the function execution as an Any type.
  */
  virtual Any
  execute(const String& functionName, const Vector<Any>& args = {}) = 0;

  /*
  * @brief Executes a function with the given name and arguments.
  *        This is a convenience method that converts the arguments to Any type.
  * @param functionName The name of the function to execute.
  * @param args The arguments to pass to the function.
  * @return The result of the function execution as an Any type.
  */
  template<typename... Args>
  Any
  execute(const String& functionName, Args&&... args) {
    return execute(functionName, {Any(std::forward<Args>(args))...});
  }
};

} // namespace chEngineSDK
