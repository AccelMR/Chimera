/************************************************************************/
/**
 * @file chDX12SwapChain.h
 * @author AccelMR
 * @date 2022/11/17
 *  DirectX 12 specific implementation for chimera engine.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include "chSwapChain.h"


namespace chEngineSDK {
class DX12SwapChain final: public SwapChain {
 public:
 /*
  *   Default constructor
  */
  DX12SwapChain() = default;

 /*
  *   Default destructor
  */
  ~DX12SwapChain();


 protected:
  friend class GraphicsModuleDX12;
  friend class DX12GPUCommandBuffer;

  void 
  init(const chGPUDesc::SwapChainDesc& desc) override;

  void
  createResources();

  void
  _internalResize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) override;

  void
  _internalPresent(int32 syncInterval, int32 flags);

  uint32 
  _internalGetCurrentFrameIndex() const override;

  SPtr<Texture>
  getCurrentRenderTarget() const override;

 protected:
  IDXGISwapChain3* m_swapchain;

  Vector<ID3D12Resource *> m_renderTargets;
  ID3D12DescriptorHeap *m_rtvHeap;
  uint32 m_rtvDescriptorSize;
};
}

