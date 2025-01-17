/************************************************************************/
/**
 * @file chDX12SwapChain.cpp
 * @author AccelMR
 * @date 2022/11/17
 * 
 *  DirectX 12 specific implementation for chimera engine.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12SwapChain.h"

#include "chDX12GraphicsModule.h"
#include "chGPUResourceDescriptors.h"
#include "chDX12Translator.h"
#include "chScreen.h"
#include "chDX12Texture.h"

namespace chEngineSDK{
using namespace chGPUDesc;

/*
*/
DX12SwapChain::~DX12SwapChain() {
  SafeRelease(m_swapchain);
}

/*
*/
void
DX12SwapChain::init(const SwapChainDesc& desc) {
  GraphicsModuleDX12& rhi = g_DX12GraphicsModule();
  DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
  swapchainDesc.BufferCount = desc.frameCount;
  swapchainDesc.Width = desc.width;
  swapchainDesc.Height = desc.height;
  swapchainDesc.Format = DX12Translator::get(desc.format);
  swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchainDesc.SwapEffect = DX12Translator::get(desc.effect);
  swapchainDesc.SampleDesc.Count = 1;

  HWND hwnd = static_cast<HWND>(rhi.m_screen->getPlatformHandler());
  
  IDXGISwapChain1* swapchain1 = nullptr;
  ThrowIfFailed(rhi.m_factory->CreateSwapChainForHwnd(rhi.m_commandQueue,
                                                      hwnd,
                                                      &swapchainDesc,
                                                      nullptr,
                                                      nullptr,
                                                      &swapchain1));

  HRESULT swapchainSupport = swapchain1->QueryInterface(__uuidof(IDXGISwapChain3),
                                                       (void **)&swapchain1);
  if (SUCCEEDED(swapchainSupport)) {
    m_swapchain = (IDXGISwapChain3*)swapchain1;
  }
  createResources();
}

/*
*/
void
DX12SwapChain::createResources() {
  m_renderTargets.resize(m_frameCount);
  GraphicsModuleDX12& rhi = g_DX12GraphicsModule();
  auto* device = rhi.getDevice();

  // Describe and create a render target view (RTV) descriptor heap.
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = m_frameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

  m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

  // Create a RTV for each frame.
  for (UINT n = 0; n < m_frameCount; n++) {
    ThrowIfFailed(m_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
    device->CreateRenderTargetView(m_renderTargets[n], nullptr, rtvHandle);
    rtvHandle.Offset(1, m_rtvDescriptorSize);
  }
}

/*
*/
void
DX12SwapChain::_internalResize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) {
  m_swapchain->ResizeBuffers(frameCount, width, height, 
                             DX12Translator::get(fromat), 0);
  g_DX12GraphicsModule().m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();//TODO: we probably should not be doing this hehehox
  createResources();
}

/*
*/
void
DX12SwapChain::_internalPresent(int32 syncInterval, int32 flags) {
  ThrowIfFailed(m_swapchain->Present(syncInterval, flags));
}

/*
*/
uint32
DX12SwapChain::_internalGetCurrentFrameIndex() const {
  return m_swapchain->GetCurrentBackBufferIndex();
}

/*
*/
SPtr<Texture>
DX12SwapChain::getCurrentRenderTarget() const {
    return ch_shared_ptr_new<DX12Texture>(m_renderTargets[g_DX12GraphicsModule().m_frameIndex],
                                          m_rtvHeap);
}

}


