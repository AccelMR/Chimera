/************************************************************************/
/**
 * @file chGraphicsModuleDX12.h
 * @author AccelMR
 * @date 2022/09/13
 *   Render hardware implementation specific for Directx12.
 */
 /************************************************************************/
 #pragma once

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
#include "chPrerequisitesDX12.h"

#include <chGraphicsModule.h>

namespace chEngineSDK{
class GraphicsModuleDX12 final: public GraphicsModule
{
 public:
 /*
  *   Default constructor
  */
  GraphicsModuleDX12();
  GraphicsModuleDX12(const SPtr<Screen> _screen);
  ~GraphicsModuleDX12();

//   FORCEINLINE ID3D12CommandAllocator *const
//   getAllocator(SIZE_T id) {
//     return m_commandAllocators[id];
//   }

  FORCEINLINE ID3D12Device* const
  getDevice() {
    return m_device;
  }

 protected:
  friend class DX12GPUCommandBuffer;
  friend class DX12SwapChain;

  void
  _init();
  
  void
  _setupSwapchain(uint32 width, uint32 height);

  void 
  onStartUp() override;

  void 
  onShutDown() override; 

  SPtr<GPUCommandBuffer>
  _internalCreateGPUCommandBuffer() override;

  void 
  _internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) override;

  void 
  _internalPresent(int32, int32) override;

  void 
  _internalMoveToNextFrame() override;

  void 
  _internalWaitGPU() override;

  SPtr<GPUPipelineState> 
  _internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) override;

  SPtr<SwapChain> 
  _internalGetSwapChain() override;

  void
  _internalResetSwapChainAllocator() override;

  void 
  createFence() override;

  void 
  _internalOnResize( uint32 width, uint32 height ) override;

 protected:

  //Keep for resize
  SPtr<Screen> m_screen;

  SIZE_T m_dedicatedVideoMemory{};
  SIZE_T m_dedicatedSystemMemory{};
  SIZE_T m_sharedSystemMemory{};

  // Initialization
  IDXGIFactory4* m_factory{};
  IDXGIAdapter1* m_adapter{};

  // Current Frame
  UINT m_currentBuffer{};

  ID3D12PipelineState* m_pipelineState{};
  ID3D12CommandQueue* m_commandQueue{};

  //IDXGISwapChain3* m_swapchain{};
  SPtr<DX12SwapChain> m_swapChain;
  ID3D12Device* m_device{};

  //Array<SPtr<Texture>, FrameCount> m_renderTargets;

  // Sync
  UINT m_frameIndex{};
  HANDLE m_fenceEvent{};
  ID3D12Fence* m_fence{};
  UINT64 m_fenceValues[2];

#if USING(CH_DEBUG_MODE)
  ID3D12Debug1* m_debugController{};
  ID3D12DebugDevice* m_DebugDevice{};
#endif //#if USING(CH_DEBUG_MODE)

  //Map<SIZE_T, ID3D12CommandAllocator*> m_commandAllocators;

  Vector<ID3D12CommandAllocator*> m_frameAlloc;

};

GraphicsModuleDX12&
g_DX12GraphicsModule();
}
