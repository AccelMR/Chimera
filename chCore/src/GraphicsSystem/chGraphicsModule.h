/************************************************************************/
/**
 * @file chGraphicsModule.h
 * @author AccelMR
 * @date 2022/09/13
 *   Chimera render hardware interface.
 *  Abstract implementation of a render hardware interface.
 */
 /************************************************************************/
 #pragma once

 /************************************************************************/
 /*
  * Implementation
  */                                                                     
 /************************************************************************/
 #include "chPrerequisitesCore.h"

 #include "chModule.h"
 #include "chGPUResourceDescriptors.h"

 #include "chGPUCommandBuffer.h"
 #include "chGPUFramebuffer.h"
 #include "chGPUPipelineState.h"
 #include "chSwapChain.h"

namespace chEngineSDK{
using namespace chGPUDesc;

class CH_CORE_EXPORT GraphicsModule: public Module<GraphicsModule>
{
 public:

  GraphicsModule() = default;
  virtual ~GraphicsModule() = default;

  /**
   * @brief Initializes the graphics module.
   * @param screen The screen to be used.
   */
  void
  initialize(const SPtr<Screen>& screen);

  /**
   * @brief Resizes the swap chain.
   * @param width The new width.
   * @param height The new height.
   */
  void
  OnResize(uint32 width, uint32 height);

  /**
   * 
   * @brief Begins the command recording.
   * @param commandBufferType The type of command buffer.
   */
  SPtr<GPUCommandBuffer>
  createCommandBuffer(COMMAND_BUFFER_TYPES commandBufferType);

  /**
   * @brief Creates a GPU pipeline state.
   */
  SPtr<GPUPipelineState> 
  createPipelineState(const PipelineStateDesc& pipelineStateDesc);

  /**
   * @brief Executes the command buffers.
   */
  void
  executeCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers);

  /**
   * @brief Moves to the next frame.
   */
  void
  moveToNextFrame();

  /**
   * @brief Waits for the GPU.
   */
  void
  waitGPU();

  /**
   * @brief Returns the swap chain.
   */
  SPtr<SwapChain>
  getSwapChain(); 

  /**
   * @brief Resets the swap chain allocator.
   */
  void
  resetSwapChainAllocator();

  SPtr<Fence>
  createFence();

  void
  syncGPU(const WeakPtr<Fence> fence, uint64 value);

  SPtr<RenderPass>
  createRenderPass(const RenderPassDesc& renderPassDesc);

  SPtr<Framebuffer>
  createFramebuffer(const FramebufferDesc& framebufferDesc);

protected:
  virtual void
  _internalInit(WeakPtr<Screen> screen) = 0;

  virtual SPtr<GPUCommandBuffer>
  _internalCreateGPUCommandBuffer() = 0;

  virtual void
  _internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) = 0;

  virtual SPtr<GPUPipelineState>
  _internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) = 0;

  virtual void
  _internalOnResize(uint32 width, uint32 height) = 0;

  virtual SPtr<SwapChain>
  _internalGetSwapChain() = 0;

  virtual void
  _internalResetSwapChainAllocator() = 0;

  virtual SPtr<Fence>
  _internalCreateFence() = 0;

  virtual void
  _internalSyncGPU(const WeakPtr<Fence> fence, uint64 value) = 0;

  virtual SPtr<RenderPass>
  _internalCreateRenderPass(const RenderPassDesc& renderPassDesc) = 0;

  virtual SPtr<Framebuffer>
  _internalCreateFramebuffer(const FramebufferDesc& framebufferDesc) = 0;

 protected:
  SPtr<SwapChain> m_swapChain;
};
}
