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
struct SwapChainDesc;

struct GraphicsAPIInfo {
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

  /**
   * The swap chain makes its own surface on desc.window, so every window can have one.
   */
  NODISCARD virtual SPtr<ISwapChain>
  createSwapChain(const SwapChainDesc& desc) = 0;

  NODISCARD virtual SPtr<IBuffer>
  createBuffer(const BufferCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<ITexture>
  createTexture(const TextureCreateInfo& createInfo) = 0;

  NODISCARD virtual SPtr<IShader>
  createShader(const ShaderCreateInfo& createInfo) = 0;

  /**
   * Builds a pipeline every time; use PipelineCache to reuse one with the same description.
   */
  NODISCARD virtual SPtr<IPipeline>
  createGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;

  NODISCARD virtual SPtr<ISampler>
  createSampler(const SamplerCreateInfo& createInfo) = 0;

  /**
   * Waits until the GPU has finished the last frame that used the same slot, then returns
   * the command list of the frame, open and ready to record. Every beginFrame needs its
   * endFrame, also when the frame draws nothing.
   */
  NODISCARD virtual ICommandList&
  beginFrame() = 0;

  /**
   * Submits the frame. It waits for the images acquired from swap chains during the frame
   * and lets them be presented afterwards.
   */
  virtual void
  endFrame() = 0;

  /**
   * Slot of the current frame, below GraphicsLimits::MAX_FRAMES_IN_FLIGHT. Data the CPU
   * writes every frame needs one copy per slot, because the GPU may still read the last one.
   */
  NODISCARD virtual uint32
  getFrameIndex() const = 0;

  virtual void
  waitIdle() = 0;
};

} // namespace chEngineSDK
